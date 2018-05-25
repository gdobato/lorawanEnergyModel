/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "lora-energy-source.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/lora-utils.h"


//Implementaton based on BasicEnergySource 
//Only default atributtes are modified and some test interface added.

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraEnergySource");

NS_OBJECT_ENSURE_REGISTERED (LoraEnergySource);

TypeId
LoraEnergySource::GetTypeId (void)
{

  static TypeId tid = TypeId ("ns3::LoraEnergySource")
    .SetParent<EnergySource> ()
    .SetGroupName ("Energy")
    .AddConstructor<LoraEnergySource> ()
    .AddAttribute ("LoraEnergySourceInitialEnergyJ",
                   "Initial energy stored in lora energy source.",
                   DoubleValue (5.55),  
                   MakeDoubleAccessor (&LoraEnergySource::SetInitialEnergy,
                                       &LoraEnergySource::GetInitialEnergy),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LoraEnergySourceInitialChargemAh",
                   "Initial charge stored in lora energy source (mAh)",
                   DoubleValue (1500), 
                   MakeDoubleAccessor (&LoraEnergySource::SetInitialCharge,
                                       &LoraEnergySource::GetInitialCharge),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LoraEnergySupplyVoltageV",
                   "Initial supply voltage for basic energy source.",
                   DoubleValue (3.7),
                   MakeDoubleAccessor (&LoraEnergySource::SetSupplyVoltage,
                                       &LoraEnergySource::GetSupplyVoltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LoraEnergyLowBatteryThreshold",
                   "Low battery threshold for basic energy source.",
                   DoubleValue (0.10), 
                   MakeDoubleAccessor (&LoraEnergySource::m_lowBatteryTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LoraEnergyHighBatteryThreshold",
                   "High battery threshold for basic energy source.",
                   DoubleValue (0.15),
                   MakeDoubleAccessor (&LoraEnergySource::m_highBatteryTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PeriodicEnergyUpdateInterval",
                   "Time between two consecutive periodic energy updates.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&LoraEnergySource::SetEnergyUpdateInterval,
                                     &LoraEnergySource::GetEnergyUpdateInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("RemainingEnergy",
                     "Remaining energy at LoraEnergySource.",
                     MakeTraceSourceAccessor (&LoraEnergySource::m_remainingEnergyJ),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("RemainingCharge",
                     "Remaining energy at LoraEnergySource.",
                     MakeTraceSourceAccessor (&LoraEnergySource::m_remainingChargemAh),
                     "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

LoraEnergySource::LoraEnergySource ()
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Seconds (0.0);
  m_depleted = false;
}

LoraEnergySource::~LoraEnergySource ()
{
  NS_LOG_FUNCTION (this);
}

void
LoraEnergySource::SetInitialEnergy (double initialEnergyJ)
{
  NS_LOG_FUNCTION (this << initialEnergyJ);
  NS_ASSERT (initialEnergyJ >= 0);
  m_initialEnergyJ = initialEnergyJ;
  m_remainingEnergyJ = m_initialEnergyJ;

}

void
LoraEnergySource::SetInitialCharge (double initialChargemAh)
{
  NS_LOG_FUNCTION (this << initialChargemAh);
  NS_ASSERT (initialChargemAh >= 0);
  m_initialChargemAh = initialChargemAh;
  m_remainingChargemAh = m_initialChargemAh;
}

void
LoraEnergySource::SetSupplyVoltage (double supplyVoltageV)
{
  NS_LOG_FUNCTION (this << supplyVoltageV);
  m_supplyVoltageV = supplyVoltageV;
}

void
LoraEnergySource::SetEnergyUpdateInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_energyUpdateInterval = interval;
}

Time
LoraEnergySource::GetEnergyUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_energyUpdateInterval;
}

double
LoraEnergySource::GetSupplyVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Supply Voltage: " << m_supplyVoltageV << " V");
  return m_supplyVoltageV;
}

double
LoraEnergySource::GetInitialEnergy (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Initial Energy: " << m_initialEnergyJ << " J");
  return m_initialEnergyJ;
}

double
LoraEnergySource::GetInitialCharge (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialChargemAh;
}

double
LoraEnergySource::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();
  NS_LOG_DEBUG ("Remaining Energy: " << m_remainingEnergyJ << " J");
  return m_remainingEnergyJ;
}

double
LoraEnergySource::GetRemainingCharge (void)
{
  NS_LOG_FUNCTION (this);

  UpdateEnergySource ();
  return m_remainingChargemAh;
}


double
LoraEnergySource::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);

  UpdateEnergySource ();
  return m_remainingEnergyJ / m_initialEnergyJ;
}

void
LoraEnergySource::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoraEnergySource:Updating remaining energy.");


  if (Simulator::IsFinished ())
    {
      return;
    }

  m_energyUpdateEvent.Cancel ();

  double remainingEnergy = m_remainingEnergyJ;
  CalculateRemaining();

  m_lastUpdateTime = Simulator::Now ();

  if (!m_depleted && m_remainingEnergyJ <= m_lowBatteryTh * m_initialEnergyJ)
  {
    m_depleted = true;
    HandleEnergyDrainedEvent ();
  }
  else if (m_depleted && m_remainingEnergyJ > m_highBatteryTh * m_initialEnergyJ)
  {
    m_depleted = false;
    HandleEnergyRechargedEvent ();
  }
  else if (m_remainingEnergyJ != remainingEnergy)
  {
    NotifyEnergyChanged ();
  }

  m_energyUpdateEvent = Simulator::Schedule (m_energyUpdateInterval,
                                             &LoraEnergySource::UpdateEnergySource,
                                             this);
}


void
LoraEnergySource::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();
}

void
LoraEnergySource::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BreakDeviceEnergyModelRefCycle ();  
}

void
LoraEnergySource::HandleEnergyDrainedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoraEnergySource:Energy depleted!");
  NotifyEnergyDrained (); 
}

void
LoraEnergySource::HandleEnergyRechargedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoraEnergySource:Energy recharged!");
  NotifyEnergyRecharged (); 
}

void
LoraEnergySource::CalculateRemaining(void)
{
  NS_LOG_FUNCTION (this);
  double totalCurrentA = CalculateTotalCurrent ();
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ());
 
  double energyToDecreaseJ = (totalCurrentA * m_supplyVoltageV * duration.GetNanoSeconds ()) / 1e9;
  if(m_remainingEnergyJ <= energyToDecreaseJ)
  {
    m_remainingEnergyJ = 0.0;
  }
  m_remainingEnergyJ -= energyToDecreaseJ;
  NS_LOG_DEBUG ("LoraEnergySource:Remaining energy = " << m_remainingEnergyJ);
  m_remainingChargemAh = (m_remainingEnergyJ/m_supplyVoltageV)*1000;
  NS_LOG_DEBUG ("LoraEnergySource:Remaining charge = " << m_remainingChargemAh);
}






} // namespace ns3
