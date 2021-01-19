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

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/lora-energy-source-helper.h"
#include "ns3/lora-stats-helper.h"
#include "ns3/names.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/lora-building-allocator.h"
#include "ns3/lora-energy-source.h"
#include "ns3/lora-radio-energy-model.h"
#include "ns3/device-energy-model-container.h"

#include "ns3/string.h"
#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LoraEnergyModelTest");

/*********************************************************************
 * Parameters configuration
 *********************************************************************/
/*
 * Network configuration
 */
//ED configuration
#define ED_X_COORDINATE                   0
#define ED_Y_COORDINATE                   1
#define ED_HEIGHT                        15
//GW configuration
#define GW_X_COORDINATE                   0
#define GW_Y_COORDINATE                   0
#define GW_HEIGHT                        15
//Frequency
#define FREQUENCY                     868e6
#define PATH_LOSS_EXP                  3.76
#define LOSS_REF                        8.1
/*
 * Energy Configuration
 */
#define TX_CURR_DEFAULT             43.5e-3
#define RX_CURR_DEFAULT             11.2e-3
#define STANDBY_CURR_DEFAULT         1.4e-3
#define SLEEP_CURR_DEFAULT           1.8e-6

#define TX_POWER_DEFAULT               14.0

/*
 * Simulation configuration
 */
#define START_SIMULATION_TIME             0
#define STOP_SIMULATION_TIME            5.5
/*
 *  Statistics configuration
 */
#define LABELS                         true

/*********************************************************************
 * Main Program - Test Suite for Lora Energy Model
 *********************************************************************/

int main (int argc, char *argv[])
{
  /*********************************************************************
   * Enable Log Components
   *********************************************************************/
  LogComponentEnable ("LoraEnergyModelTest"  , LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraPhy"     , LOG_LEVEL_DEBUG);
  //LogComponentEnable ("LoraConsumptionModel" , LOG_LEVEL_DEBUG);
  LogComponentEnable ("LoraRadioEnergyModel" , LOG_LEVEL_DEBUG);
  //LogComponentEnable ("LoraEnergySource"     , LOG_LEVEL_DEBUG);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);



  /*********************************************************************
   * Create mobility model
   *********************************************************************/
  //ED mobility model
  Ptr<ListPositionAllocator> fixAllocatorEd = CreateObject<ListPositionAllocator> ();
  fixAllocatorEd->Add (Vector (ED_X_COORDINATE,ED_Y_COORDINATE,ED_HEIGHT));

  MobilityHelper MobilityEd;
  MobilityEd.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  MobilityEd.SetPositionAllocator(fixAllocatorEd);

  //GW mobility model
  Ptr<ListPositionAllocator> fixAllocatorGw = CreateObject<ListPositionAllocator> ();
  fixAllocatorGw->Add (Vector (GW_X_COORDINATE,GW_Y_COORDINATE,GW_HEIGHT)); //Single GW in center

  MobilityHelper MobilityGw;
  MobilityGw.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  MobilityGw.SetPositionAllocator(fixAllocatorGw);

  /*********************************************************************
   * Configure Lora Channel
   *********************************************************************/
  //Delay model
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  //LossModel
  Ptr<LogDistancePropagationLossModel> longDistanceLoss = CreateObject<LogDistancePropagationLossModel> ();
  longDistanceLoss->SetPathLossExponent (PATH_LOSS_EXP);
  longDistanceLoss->SetReference (1, LOSS_REF);
  //Create channel
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (longDistanceLoss, delay);


  /*********************************************************************
   * Create Lora helpers
   *********************************************************************/
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);
  LoraMacHelper macHelper = LoraMacHelper ();
  LoraHelper helper = LoraHelper ();
  LoraStatsHelper statsHelper = LoraStatsHelper();

  /*********************************************************************
   * Create and configure End Device
   *********************************************************************/
  NodeContainer endDevices;
  endDevices.Create (1);
  MobilityEd.Install(endDevices);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);

  /*********************************************************************
   * Create and configure Gateways
   *********************************************************************/
  //STAR CONFIGURATION
  NodeContainer gateways;
  gateways.Create (1);
  MobilityGw.Install (gateways);
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);


  /*********************************************************************
   *  Set spreading factors of End Devices
   *********************************************************************/
  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);


  /*********************************************************************
   *  Install Lora Energy Model
   *********************************************************************/

  LoraEnergySourceHelper loraSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source
  loraSourceHelper.Set ("LoraEnergySourceInitialEnergyJ", DoubleValue (5.55));
  loraSourceHelper.Set ("LoraEnergySupplyVoltageV", DoubleValue (3.7));

  //Attach Consumption Model
  radioEnergyHelper.SetConsumptionModel ("ns3::InterpolatedLoraConsumptionModel");

  EnergySourceContainer sources = loraSourceHelper.Install (endDevices);


  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install
      (endDevicesNetDevices, sources);


  /*********************************************************************
   *  Get Test information
   *********************************************************************/
  //Get Phy Information
  Ptr<Node> node = endDevices.Get(0);
  NS_ASSERT (node != NULL);
  Ptr<NetDevice> netDevice = node->GetDevice(0);
  NS_ASSERT (netDevice != NULL);
  Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice>();
  NS_ASSERT(loraNetDevice != NULL);
  Ptr<EndDeviceLoraPhy> edPhy = loraNetDevice->GetPhy()->GetObject<EndDeviceLoraPhy>();
  NS_ASSERT(edPhy!= NULL);

  //Get Energy and consumption Information
  Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer>();
  NS_ASSERT (energySourceContainer != NULL);
  Ptr<LoraEnergySource> loraEnergySource = DynamicCast<LoraEnergySource>(energySourceContainer->Get(0));
  NS_ASSERT (loraEnergySource != NULL);
  DeviceEnergyModelContainer deviceEnergyModelContainer = loraEnergySource->FindDeviceEnergyModels("ns3::LoraRadioEnergyModel");
  Ptr<LoraRadioEnergyModel> loraRadioEnergyModel = DynamicCast<LoraRadioEnergyModel>(deviceEnergyModelContainer.Get(0));
  NS_ASSERT (loraRadioEnergyModel != NULL);

  /*********************************************************************
   *  Test Interpolated Consumption Model
   *********************************************************************/
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 7.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 8.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 9.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 10.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 11.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 12.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 13.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 14.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 15.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 16.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 17.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 18.0 );
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 19.0);
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &LoraRadioEnergyModel::CalcTxCurrentFromModel, loraRadioEnergyModel, 20.0);

  /*********************************************************************
   *  Test Listener
   *********************************************************************/
  Simulator::Schedule (Seconds(START_SIMULATION_TIME), &EndDeviceLoraPhy::SwitchToTx,edPhy, TX_POWER_DEFAULT);
  Simulator::Schedule (Seconds(1), &EndDeviceLoraPhy::SwitchToRx, edPhy);
  Simulator::Schedule (Seconds(2.25), &EndDeviceLoraPhy::SwitchToStandby, edPhy);
  Simulator::Schedule (Seconds(3.75), &EndDeviceLoraPhy::SwitchToSleep, edPhy);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &EndDeviceLoraPhy::SwitchToStandby, edPhy);

  /*********************************************************************
   *  Test Energy Device Model
   *********************************************************************/
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTxCurrentA, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetRxCurrentA, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetStandbyCurrentA, loraRadioEnergyModel) ;
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetSleepCurrentA, loraRadioEnergyModel);

  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTotalTxTime, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTotalRxTime, loraRadioEnergyModel) ;
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTotalStandbyTime, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTotalSleepTime, loraRadioEnergyModel);

  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTxEnergyConsumption, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetRxEnergyConsumption, loraRadioEnergyModel) ;
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetStandbyEnergyConsumption, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetSleepEnergyConsumption, loraRadioEnergyModel);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraRadioEnergyModel::GetTotalEnergyConsumption, loraRadioEnergyModel);

  /*********************************************************************
   *  Test Energy Source Model
   *********************************************************************/
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraEnergySource::GetSupplyVoltage, loraEnergySource);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraEnergySource::GetInitialEnergy, loraEnergySource);
  Simulator::Schedule (Seconds(STOP_SIMULATION_TIME), &LoraEnergySource::GetRemainingEnergy, loraEnergySource);


  /*********************************************************************
   *  Start Simulation
   *********************************************************************/
  //Set Stop Time
  Simulator::Stop (Seconds (STOP_SIMULATION_TIME));

  //Run Simulation
  Simulator::Run ();

  //Collect statistics
#if 0
  statsHelper.NodeInformation("src/lorawan/deployment/energyTest-collect.dat",endDevices,gateways);
  statsHelper.EnergyInformation("src/lorawan/deployment/energyTest-energy.dat",endDevices);
  statsHelper.GnuPlot2dScript ("src/lorawan/deployment/2d-energyTest-deployment","energyTest-collect.dat",LABELS);
  statsHelper.GnuPlot3dScript ("src/lorawan/deployment/3d-energyTesst-deployment","energyTest-collect.dat",LABELS);
#endif
  NS_LOG_INFO("End of simulation");

  //Destroy simulation objects
  Simulator::Destroy ();

  return 0;
}
