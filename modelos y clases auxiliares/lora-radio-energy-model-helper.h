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

#ifndef LORA_RADIO_ENERGY_MODEL_HELPER_H
#define LORA_RADIO_ENERGY_MODEL_HELPER_H

#include "ns3/energy-model-helper.h"
#include "ns3/lora-radio-energy-model.h"

namespace ns3 {
/**
 * \ingroup energy
 * \brief A LoRa radio energy helper based on WifiRadioEnergyHelper
 *
 */
class LoraRadioEnergyModelHelper : public DeviceEnergyModelHelper
{
public:
  LoraRadioEnergyModelHelper ();
  ~LoraRadioEnergyModelHelper ();
  //Handle attributes
  void Set (std::string name, const AttributeValue &v);

  //Register Energy-Handling Callbacks
  void RegisterEnergyDepletionCB ( LoraRadioEnergyModel::LoraEnergyDepletionCB cb);
  void RegisterEnergyRechargedCB ( LoraRadioEnergyModel::LoraEnergyRechargedCB cb);
  void RegisterEnergyChangedCB   ( LoraRadioEnergyModel::LoraEnergyChangedCB cb);

  //Set consumption model and handle attributes
  void SetConsumptionModel (std::string name,
                            std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                            std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                            std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                            std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                            std::string n4 = "", const AttributeValue &v7 = EmptyAttributeValue ());

private:
  virtual Ptr<DeviceEnergyModel> DoInstall (Ptr<NetDevice> device,
                                            Ptr<EnergySource> source) const;

private:
  //energy source
  ObjectFactory m_energyModel;
  //consumption model
  ObjectFactory m_consumptionModel;

  //Callback types to be registered for energy handling
  //Callbacks to handle state of energy source
  LoraRadioEnergyModel::LoraEnergyDepletionCB m_energyDepletionCB;
  LoraRadioEnergyModel::LoraEnergyRechargedCB m_energyRechargedCB;
  LoraRadioEnergyModel::LoraEnergyChangedCB   m_energyChangedCB;


};

} // namespace ns3

#endif /* LORA_RADIO_ENERGY_MODEL_HELPER_H */
