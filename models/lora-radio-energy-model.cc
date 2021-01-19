/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Universitat Oberta de Catalunya
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Gabriel Dobato <gdobato@uoc.edu>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "lora-radio-energy-model.h"

#define TX_CURR_DEFAULT        43.5e-3
#define RX_CURR_DEFAULT        11.2e-3
#define STANDBY_CURR_DEFAULT   1.4e-3
#define SLEEP_CURR_DEFAULT     1.8e-6

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (LoraRadioEnergyModel);

TypeId
LoraRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoraRadioEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .SetGroupName ("Energy")
    .AddConstructor<LoraRadioEnergyModel> ()
    //For mor reference see chapter 5 - "Integración de un modelo de energía
    //en la simulación de redes LoRaWAN en NS-3"
    .AddAttribute  ("TxCurrentA",
                   "Default consumption in TX operation)",
                   DoubleValue (TX_CURR_DEFAULT),
                   MakeDoubleAccessor (&LoraRadioEnergyModel::SetTxCurrentA,
                                       &LoraRadioEnergyModel::GetTxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute  ("RxCurrentA",
                   "Default consumption in RX operation)",
                   DoubleValue (RX_CURR_DEFAULT),
                   MakeDoubleAccessor (&LoraRadioEnergyModel::SetRxCurrentA,
                                       &LoraRadioEnergyModel::GetRxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute  ("StandbyCurrentA",
                   "Default consumption in STANDBY operation)",
                   DoubleValue (STANDBY_CURR_DEFAULT),
                   MakeDoubleAccessor (&LoraRadioEnergyModel::SetStandbyCurrentA,
                                       &LoraRadioEnergyModel::GetStandbyCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute  ("SleepCurrentA",
                   "Default consumption in SLEEP operation)",
                   DoubleValue (SLEEP_CURR_DEFAULT),
                   MakeDoubleAccessor (&LoraRadioEnergyModel::SetSleepCurrentA,
                                       &LoraRadioEnergyModel::GetSleepCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute  ("ConsumptionModel", "A pointer to the attached consumption model.",
                   PointerValue (),
                   MakePointerAccessor (&LoraRadioEnergyModel::m_consumptionModel),
                   MakePointerChecker<LoraConsumptionModel> ())
    .AddTraceSource("TotalEnergyConsumption",
                    "Total energy consumption of the radio device.",
                    MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_totalEnergyConsumption),
                   "ns3::TracedValueCallback::Double")
    .AddTraceSource("TxEnergyConsumption",
                    "Energy consumption in TX mode",
                    MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_txEnergyConsumption),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("RxEnergyConsumption",
                    "Energy consumption in RX mode",
                     MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_rxEnergyConsumption),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("StandbyEnergyConsumption",
                    "Energy consumption in STANDBY mode",
                    MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_standbyEnergyConsumption),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("SleepEnergyConsumption",
                    "Energy consumption in SLEEP mode",
                    MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_sleepEnergyConsumption),
                    "ns3::TracedValueCallback::Double")

  ;
  return tid;
}

LoraRadioEnergyModel::LoraRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  //Init State
  m_currentState = EndDeviceLoraPhy::SLEEP;              //Reference to Datasheet SX1272

  //Init consumption
  m_txEnergyConsumption      = 0.0;
  m_rxEnergyConsumption      = 0.0;
  m_standbyEnergyConsumption = 0.0;
  m_sleepEnergyConsumption   = 0.0;
  m_totalEnergyConsumption   = 0.0;

  //Initialize operation time
  m_totalTxTime      = Seconds(0.0);
  m_totalRxTime      = Seconds(0.0);
  m_totalStandbyTime = Seconds(0.0);
  m_totalSleepTime   = Seconds(0.0);

  //Initialize internal state variables
  m_lastStampTime = Seconds (0.0);
  m_energyDepleted = false;

  //Nullify all elements
  m_energyDepletionCB.Nullify ();
  m_energyRechargedCB.Nullify ();
  m_energyChangedCB.Nullify ();
  m_source = NULL;

  //Init listener and attach callbacks to monitor operation state
  m_loraEnergyPhyListener = new LoraEnergyPhyListener;
  m_loraEnergyPhyListener->RegisterNotifyTransitionCB(MakeCallback (&DeviceEnergyModel::ChangeState, this));
  m_loraEnergyPhyListener->RegisterNotifyTxConsumptionCB(MakeCallback (&LoraRadioEnergyModel::CalcTxCurrentFromModel, this));

}

LoraRadioEnergyModel::~LoraRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  delete m_loraEnergyPhyListener;
}

void
LoraRadioEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
}

void
LoraRadioEnergyModel::SetConsumptionModel (Ptr<LoraConsumptionModel> model)
{
  NS_ASSERT(model != NULL);
  m_consumptionModel = model;
}

double LoraRadioEnergyModel::GetTxEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("TX Energy consumption: " << m_txEnergyConsumption << " J");
  return m_txEnergyConsumption;
}

double LoraRadioEnergyModel::GetRxEnergyConsumption (void) const
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG ("RX Energy consumption: " << m_rxEnergyConsumption << " J");
  return m_rxEnergyConsumption;
}

double LoraRadioEnergyModel::GetStandbyEnergyConsumption (void) const
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG ("STANDBY Energy consumption: " << m_standbyEnergyConsumption << " J");
  return m_standbyEnergyConsumption;
}

double LoraRadioEnergyModel::GetSleepEnergyConsumption (void) const
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG ("SLEEP Energy consumption: " << m_sleepEnergyConsumption << " J");
  return m_sleepEnergyConsumption;
}

double
LoraRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("TOTAL Energy consumption: " << m_totalEnergyConsumption << " J");
  return m_totalEnergyConsumption;
}

Time LoraRadioEnergyModel::GetTotalTxTime(void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Total time in TX mode: " << m_totalTxTime.GetSeconds() << " s");
  return m_totalTxTime;
}

Time LoraRadioEnergyModel::GetTotalRxTime(void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Total time in RX mode: " << m_totalRxTime.GetSeconds() << " s");
  return m_totalRxTime;
}

Time LoraRadioEnergyModel::GetTotalStandbyTime(void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Total time in STANDBY mode: " << m_totalStandbyTime.GetSeconds() << " s");
  return m_totalStandbyTime;
}

Time LoraRadioEnergyModel::GetTotalSleepTime(void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Total time in SLEEP mode: " << m_totalSleepTime.GetSeconds() << " s");
  return m_totalSleepTime;
}

double
LoraRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("TX mode current: " << m_txCurrentA << " A");
  return m_txCurrentA;
}

double
LoraRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("RX mode current: " << m_rxCurrentA << " A");
  return m_rxCurrentA;
}

double
LoraRadioEnergyModel::GetStandbyCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("STANDBY mode current: " << m_standbyCurrentA << " A");
  return m_standbyCurrentA;
}

double
LoraRadioEnergyModel::GetSleepCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("SLEEP mode current: " << m_sleepCurrentA << " A");
  return m_sleepCurrentA;
}

void
LoraRadioEnergyModel::SetTxCurrentA (double txCurrentA)
{
  NS_LOG_FUNCTION (this << txCurrentA);
  m_txCurrentA = txCurrentA;
}

void
LoraRadioEnergyModel::SetRxCurrentA (double rxCurrentA)
{
  NS_LOG_FUNCTION (this << rxCurrentA);
  m_rxCurrentA = rxCurrentA;
}

void
LoraRadioEnergyModel::SetStandbyCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_standbyCurrentA = idleCurrentA;
}

void
LoraRadioEnergyModel::SetSleepCurrentA (double sleepCurrentA)
{
  NS_LOG_FUNCTION (this << sleepCurrentA);
  m_sleepCurrentA = sleepCurrentA;
}

EndDeviceLoraPhy::State
LoraRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
LoraRadioEnergyModel::RegisterEnergyDepletionCB (LoraEnergyDepletionCB cb)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!cb.IsNull ());
  m_energyDepletionCB = cb;
}

void
LoraRadioEnergyModel::RegisterEnergyRechargedCB (LoraEnergyRechargedCB cb)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!cb.IsNull ());
  m_energyRechargedCB = cb;
}

void
LoraRadioEnergyModel::RegisterEnergyChangedCB (LoraEnergyChangedCB cb)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!cb.IsNull ());
  m_energyChangedCB = cb;
}

void
LoraRadioEnergyModel::CalcTxCurrentFromModel (double txPowerDbm)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT(m_consumptionModel!=NULL);
  m_txCurrentA = m_consumptionModel->CalcTxCurrent (txPowerDbm);
}

// Implementation based on WiFi model (already tested in platform)
// adaptation for Lora-PHY (according to operation states)
void
LoraRadioEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  Time duration = Simulator::Now () - m_lastStampTime;
  NS_ASSERT (duration.IsPositive ());

  //Variable to handle energy decrement
  double energyDecrement = 0.0;
  double supplyVoltage = m_source->GetSupplyVoltage ();

  //Calculate Energy and register Time elapsed and Energy consumed
  //in each operation state
  switch (m_currentState)
    {
    case EndDeviceLoraPhy::TX:
      energyDecrement = duration.GetSeconds () * m_txCurrentA * supplyVoltage;
      m_totalTxTime += duration;
      m_txEnergyConsumption += energyDecrement;
      break;
    case EndDeviceLoraPhy::RX:
      energyDecrement = duration.GetSeconds () * m_rxCurrentA * supplyVoltage;
      m_totalRxTime += duration;
      m_rxEnergyConsumption += energyDecrement;
      break;
    case EndDeviceLoraPhy::STANDBY:
      energyDecrement = duration.GetSeconds () * m_standbyCurrentA * supplyVoltage;
      m_totalStandbyTime += duration;
      m_standbyEnergyConsumption += energyDecrement;
      break;
    case EndDeviceLoraPhy::SLEEP:
      energyDecrement = duration.GetSeconds () * m_sleepCurrentA * supplyVoltage;
      m_totalSleepTime += duration;
      m_sleepEnergyConsumption += energyDecrement;
      break;
    default:
      NS_FATAL_ERROR ("Invalid Lora operation State: " << m_currentState);
    }

  // update total energy consumption
  m_totalEnergyConsumption += energyDecrement;
  // update last update time stamp
  m_lastStampTime = Simulator::Now ();
  // notify energy source
  m_source->UpdateEnergySource ();

  //If energy not depleted, change state and inform about energy consumption
  if (m_energyDepleted == false)
    {
      SetLoraPhyState (static_cast<EndDeviceLoraPhy::State>(newState));
      NS_LOG_INFO ("Energy consumption is " << m_totalEnergyConsumption << "J");
    }
}

void
LoraRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_energyDepletionCB.IsNull ())
   {
     m_energyDepletionCB ();
   }
  else
   {
     NS_LOG_INFO("Energy depletion!");
   }

  m_energyDepleted = true;
}

void
LoraRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_energyRechargedCB.IsNull ())
   {
     m_energyRechargedCB ();
   }
  else
   {
     NS_LOG_INFO("Energy recharged!");
   }
}

void
LoraRadioEnergyModel::HandleEnergyChanged (void)
{
  //No log function to avoid console overloading
  if (!m_energyChangedCB.IsNull ())
   {
     m_energyChangedCB ();
   }
}

LoraEnergyPhyListener *
LoraRadioEnergyModel::GetPhyListener (void)
{
  NS_LOG_FUNCTION (this);
  return m_loraEnergyPhyListener;
}

void
LoraRadioEnergyModel::DoDispose (void)
{
  //Nullify all elements
  NS_LOG_FUNCTION (this);
  m_energyDepletionCB.Nullify ();
  m_energyRechargedCB.Nullify ();
  m_energyChangedCB.Nullify ();
  m_source = NULL;
}

double
LoraRadioEnergyModel::DoGetCurrentA (void) const
{
  //No log function to avoid console overloading
  switch (m_currentState)
    {
    case EndDeviceLoraPhy::TX:
      return m_txCurrentA;
    case EndDeviceLoraPhy::RX:
      return m_rxCurrentA;
    case EndDeviceLoraPhy::STANDBY:
      return m_standbyCurrentA;
    case EndDeviceLoraPhy::SLEEP:
      return m_sleepCurrentA;
    default:
      NS_FATAL_ERROR ("Undefined radio state:" << m_currentState);
    }
}

void
LoraRadioEnergyModel::SetLoraPhyState (const EndDeviceLoraPhy::State state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;
  std::string stateName;
  switch (state)
    {
    case EndDeviceLoraPhy::STANDBY:
      stateName = "STANDBY";
      break;
    case EndDeviceLoraPhy::TX:
      stateName = "TX";
      break;
    case EndDeviceLoraPhy::RX:
      stateName = "RX";
      break;
    case EndDeviceLoraPhy::SLEEP:
      stateName = "SLEEP";
      break;
    }
    NS_LOG_DEBUG ("[EnergyModel] Switching to state: " << stateName << " at time = " << Simulator::Now ().GetSeconds () << " s");
}


/*
 * LoraEnergyPhyListener Implementation
 */
LoraEnergyPhyListener::LoraEnergyPhyListener ()
{
  //Nullify all used callbacks
  m_changeStateCB.Nullify ();
  m_notifyTxConsumptionCB.Nullify ();
}

LoraEnergyPhyListener::~LoraEnergyPhyListener ()
{
}

void
LoraEnergyPhyListener::RegisterNotifyTransitionCB (DeviceEnergyModel::ChangeStateCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  NS_ASSERT (!cb.IsNull ());
  m_changeStateCB = cb;
}

void
LoraEnergyPhyListener::RegisterNotifyTxConsumptionCB (NotifyTxConsumptionCB cb)
{
  NS_LOG_FUNCTION (this << &cb);
  NS_ASSERT (!cb.IsNull ());
  m_notifyTxConsumptionCB = cb;
}

void
LoraEnergyPhyListener::NotifyRxStart ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("[Listener] Notify new state: " << "RX" << " at time = " << Simulator::Now ().GetSeconds () << " s");
  NS_ASSERT (!m_changeStateCB.IsNull ());
  m_changeStateCB (EndDeviceLoraPhy::RX);
}

void
LoraEnergyPhyListener::NotifyTxStart (double txPowerDbm)
{
  NS_LOG_FUNCTION (this << txPowerDbm);
  NS_LOG_DEBUG ("[Listener] Notify new state: " << "TX" << " at time = " << Simulator::Now ().GetSeconds () << " s");

  //Update  Tx consumption
  NS_ASSERT (!m_notifyTxConsumptionCB.IsNull ());
  m_notifyTxConsumptionCB (txPowerDbm);

  NS_ASSERT (!m_changeStateCB.IsNull ());
  m_changeStateCB (EndDeviceLoraPhy::TX);

}

void
LoraEnergyPhyListener::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("[Listener] Notify new state: " << "SLEEP" << " at time = " << Simulator::Now ().GetSeconds () << " s");
  NS_ASSERT (!m_changeStateCB.IsNull ());
  m_changeStateCB (EndDeviceLoraPhy::SLEEP);
}

void
LoraEnergyPhyListener::NotifyStandby (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("[Listener] Notify new state: " << "STANDBY" << " at time = " << Simulator::Now ().GetSeconds () << " s");
  NS_ASSERT (!m_changeStateCB.IsNull ());
  m_changeStateCB (EndDeviceLoraPhy::STANDBY);
}


} // namespace ns3
