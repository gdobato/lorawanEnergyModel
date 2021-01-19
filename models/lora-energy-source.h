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

#ifndef LORA_ENERGY_SOURCE_H
#define LORA_ENERGY_SOURCE_H

#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/energy-source.h"

namespace ns3 {

/**
 * \ingroup energy
 *
 * class based in BasicEnergySource  which models a linear energy sourcea (TFM chapter 5)
 *
 */
class LoraEnergySource : public EnergySource
{
public:
  static TypeId GetTypeId (void);
  LoraEnergySource ();
  virtual ~LoraEnergySource ();


  virtual double GetInitialEnergy (void) const;
  virtual double GetInitialCharge (void) const;

  virtual double GetSupplyVoltage (void) const;

  virtual double GetRemainingEnergy (void);
  virtual double GetRemainingCharge (void);


  virtual double GetEnergyFraction (void);



  virtual void UpdateEnergySource (void);

  void SetInitialEnergy (double initialEnergyJ);
  void SetInitialCharge (double initialChargeC);

  void SetSupplyVoltage (double supplyVoltageV);

  void SetEnergyUpdateInterval (Time interval);

  Time GetEnergyUpdateInterval (void) const;


private:

  void DoInitialize (void);
  void DoDispose (void);
  void HandleEnergyDrainedEvent (void);
  void HandleEnergyRechargedEvent (void);
  void CalculateRemaining(void);
  void CalculateConsumedEnergy(void);
  void CalculateConsumedCharge(void);

private:
  //initial energy, in Joules
  double m_initialEnergyJ;  
  //initial charge in mAh
  double m_initialChargemAh;  
  //suply voltage  (volts)
  double m_supplyVoltageV;     
  //Thresholds  
  double m_lowBatteryTh;                 
  double m_highBatteryTh; 
  //depleted flag  
  bool m_depleted;                       
                                        
  // remaining energy, in Joules
  TracedValue<double> m_remainingEnergyJ; 
  // remaining charge, in  mAh
  TracedValue<double> m_remainingChargemAh; 
  // Internal variables
  EventId m_energyUpdateEvent;            
  Time m_lastUpdateTime;                 
  Time m_energyUpdateInterval;         


};

} // namespace ns3

#endif /* LORA_ENERGY_SOURCE_H */
