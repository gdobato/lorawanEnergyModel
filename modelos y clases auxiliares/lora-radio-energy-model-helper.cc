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

#include "lora-radio-energy-model-helper.h"
#include "ns3/lora-net-device.h"
#include "ns3/lora-consumption-model.h"
#include "ns3/end-device-lora-phy.h"

namespace ns3 {

LoraRadioEnergyModelHelper::LoraRadioEnergyModelHelper ()
{
  m_energyModel.SetTypeId ("ns3::LoraRadioEnergyModel");
  //Nullify Callbacks
  m_energyDepletionCB.Nullify();
  m_energyRechargedCB.Nullify();
  m_energyChangedCB.Nullify();
}

LoraRadioEnergyModelHelper::~LoraRadioEnergyModelHelper ()
{
}

void
LoraRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_energyModel.Set (name, v);
}

void
LoraRadioEnergyModelHelper::RegisterEnergyDepletionCB (LoraRadioEnergyModel::LoraEnergyDepletionCB cb)
{
  NS_ASSERT (!cb.IsNull ());
  m_energyDepletionCB = cb;
}

void
LoraRadioEnergyModelHelper::RegisterEnergyRechargedCB (LoraRadioEnergyModel::LoraEnergyRechargedCB cb)
{
  NS_ASSERT (!cb.IsNull ());
  m_energyRechargedCB = cb;
}

void
LoraRadioEnergyModelHelper::RegisterEnergyChangedCB (LoraRadioEnergyModel::LoraEnergyChangedCB cb)
{
  NS_ASSERT (!cb.IsNull ());
  m_energyChangedCB = cb;
}


void
LoraRadioEnergyModelHelper::SetConsumptionModel (std::string name,
                                                 std::string n0, const AttributeValue& v0,
                                                 std::string n1, const AttributeValue& v1,
                                                 std::string n2, const AttributeValue& v2,
                                                 std::string n3, const AttributeValue& v3,
                                                 std::string n4, const AttributeValue& v4)
{
  ObjectFactory factory;
  factory.SetTypeId (name);
  factory.Set (n0, v0);
  factory.Set (n1, v1);
  factory.Set (n2, v2);
  factory.Set (n3, v3);
  factory.Set (n4, v4);
  m_consumptionModel = factory;
}

Ptr<DeviceEnergyModel>
LoraRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device,
                                       Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);
  //Check correct device
  std::string deviceName = device->GetInstanceTypeId ().GetName ();
  if (deviceName.compare ("ns3::LoraNetDevice") != 0)
    {
      NS_FATAL_ERROR ("NetDevice type is not LoraNetDevice!");
    }
  //SetEnergy Source and add model in energy source
  Ptr<Node> node = device->GetNode ();
  Ptr<LoraRadioEnergyModel> model = m_energyModel.Create ()->GetObject<LoraRadioEnergyModel> ();
  NS_ASSERT (model != NULL);
  model->SetEnergySource (source);
  source->AppendDeviceEnergyModel (model);

  //Register PhyListener
  Ptr<LoraNetDevice> loraDevice = device->GetObject<LoraNetDevice> ();
  Ptr<EndDeviceLoraPhy> loraPhy = loraDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
  loraPhy->RegisterListener (model->GetPhyListener ());

  //Register Energy-handling callbacks
  if (!m_energyDepletionCB.IsNull())
    {
      model->RegisterEnergyDepletionCB(m_energyDepletionCB);
    }
  if (!m_energyRechargedCB.IsNull())
    {
      model->RegisterEnergyRechargedCB(m_energyRechargedCB);
    }
  if (!m_energyChangedCB.IsNull())
    {
      model->RegisterEnergyChangedCB(m_energyChangedCB);
    }

  //Set Consumption Model
  if (m_consumptionModel.GetTypeId ().GetUid ())
    {
      Ptr<LoraConsumptionModel> consumption = m_consumptionModel.Create<LoraConsumptionModel> ();
      model->SetConsumptionModel (consumption);
    }
  return model;
}

} // namespace ns3
