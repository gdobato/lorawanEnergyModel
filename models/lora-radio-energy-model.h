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

#ifndef LORA_RADIO_ENERGY_MODEL_H
#define LORA_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/lora-consumption-model.h"
#include "ns3/lora-phy-listener.h"

namespace ns3 {
/**
 * \ingroup energy
 *
 * Derivated class of LoraPhyListener to monitor the state of Lora
 * transceiver (SX1272 ). Implementation only covers model for end device
 * (SX1272), but it can be used in future implementations for gateway SX1301
 * if considered.
 *
 */
class LoraEnergyPhyListener : public LoraPhyListener
{
public:

  //Callback type to notify Tx consumption (interpolated -consumption model)
  typedef Callback<void, double> NotifyTxConsumptionCB;

  LoraEnergyPhyListener ();
  virtual ~LoraEnergyPhyListener ();

  //Register Callback after a state transition
  void RegisterNotifyTransitionCB (DeviceEnergyModel::ChangeStateCallback cb);

  //Register SetUpdateTxCurrentCallback
  void RegisterNotifyTxConsumptionCB (NotifyTxConsumptionCB cb);

  //Notify start of transmission/reception/standby/sleep
  void NotifyTxStart (double txPowerDbm);
  void NotifyRxStart (void);
  void NotifySleep   (void);
  void NotifyStandby (void);

private:

  //Calback to inform about a transition in operation mode of Lora transceiver
  //TX-RX-STANDBY-SLEEP
  DeviceEnergyModel::ChangeStateCallback m_changeStateCB;
  //Callback to inform about the current consumption in TX mode of Lora transceiver
  NotifyTxConsumptionCB m_notifyTxConsumptionCB;
};


/**
 * \ingroup energy
 * \brief A LoRa radio energy model based on WifiRadioEnergyModel
 *  (explained in TFM chapter 5)
 *
 */
class LoraRadioEnergyModel : public DeviceEnergyModel
{
public:

  //Callback types to be registered for energy handling
  typedef Callback<void> LoraEnergyDepletionCB;
  typedef Callback<void> LoraEnergyRechargedCB;
  typedef Callback<void> LoraEnergyChangedCB;

  static TypeId GetTypeId (void);
  LoraRadioEnergyModel ();
  virtual ~LoraRadioEnergyModel ();

  //Connect EnergySource
  void SetEnergySource (Ptr<EnergySource> source);
  //Connect Consumption model
  void SetConsumptionModel (Ptr<LoraConsumptionModel> model);

  //Get Energy Consumption in different operation modes
  double GetTxEnergyConsumption (void) const;
  double GetRxEnergyConsumption (void) const;
  double GetStandbyEnergyConsumption (void) const;
  double GetSleepEnergyConsumption (void) const;
  double GetTotalEnergyConsumption (void) const;

  //Get operation times on different modes
  Time GetTotalTxTime(void) const;
  Time GetTotalRxTime(void) const;
  Time GetTotalStandbyTime(void) const;
  Time GetTotalSleepTime(void) const;

  //Get current in different operation modes
  //TX-RX-STANDBY-SLEEP
  double GetTxCurrentA (void) const;
  double GetRxCurrentA (void) const;
  double GetStandbyCurrentA (void) const;
  double GetSleepCurrentA (void) const;

  //Set current in different operation modes
  //TX-RX-STANDBY-SLEEP
  void SetTxCurrentA (double txCurrentA);
  void SetRxCurrentA (double rxCurrentA);
  void SetStandbyCurrentA (double standbyCurrentA);
  void SetSleepCurrentA (double sleepCurrentA);

  //Get Current State of Lora-PHY
  EndDeviceLoraPhy::State GetCurrentState (void) const;

  void RegisterEnergyDepletionCB (LoraEnergyDepletionCB cb);
  void RegisterEnergyRechargedCB (LoraEnergyRechargedCB cb);
  void RegisterEnergyChangedCB (LoraEnergyChangedCB cb);

  //Calculate and set Tx current according to used consumption model
  void CalcTxCurrentFromModel (double txPowerDbm);

  //Methods inherited from the base class to handle energy depletion, recharged and charged
  void ChangeState (int newState);
  void HandleEnergyDepletion (void);
  void HandleEnergyRecharged (void);
  void HandleEnergyChanged (void);

  //Get listener to monitor Lora-PHY operation
  LoraEnergyPhyListener * GetPhyListener (void);

private:
  void DoDispose (void);
  double DoGetCurrentA (void) const;
  void SetLoraPhyState (const EndDeviceLoraPhy::State state);

  //Lora-Phy listener
  LoraEnergyPhyListener *m_loraEnergyPhyListener;
  //Energy Source used
  Ptr<EnergySource> m_source;
  //Consumption Model used
  Ptr<LoraConsumptionModel> m_consumptionModel;

  // Private variable to handle current in different modes
  double  m_txCurrentA;
  double  m_rxCurrentA;
  double  m_standbyCurrentA;
  double  m_sleepCurrentA;

  //Traced values to track energy consumption in different operation modes
  TracedValue<double> m_totalEnergyConsumption;
  TracedValue<double> m_txEnergyConsumption;
  TracedValue<double> m_rxEnergyConsumption;
  TracedValue<double> m_standbyEnergyConsumption;
  TracedValue<double> m_sleepEnergyConsumption;

  //Variables to handle states
  EndDeviceLoraPhy::State m_currentState;
  Time m_lastStampTime;
  bool m_energyDepleted;

  //Variables to handle time in different operation modes
  Time m_totalTxTime;
  Time m_totalRxTime;
  Time m_totalStandbyTime;
  Time m_totalSleepTime;

  //Callbacks to handle state of energy source
  LoraEnergyDepletionCB m_energyDepletionCB;
  LoraEnergyRechargedCB m_energyRechargedCB;
  LoraEnergyChangedCB m_energyChangedCB;

};

} // namespace ns3

#endif /* LORA_RADIO_ENERGY_MODEL_H */
