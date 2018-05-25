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
#include "ns3/string.h"
#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LoraOpenArea");

/*********************************************************************
 * Parameters configuration
 *********************************************************************/
/*
 * Scenario dimensions
 */
#define SCENARIO_SIDE                 20000

/*
 * Network configuration
 */
//Topology
#define STAR                              1
#define STAR_OF_STARS                     2
#define TOPOLOGY                          2
//Number of Nodes
#define N_EDS                           300
//GWs and EDs height
#define GATEWAY_HEIGHT                   15
#define ENDDEVICE_HEIGHT                1.5
//Ed Sending-Frequency Model
#define SINGLE_PERIOD                     1
#define RANDOM_PERIOD                     2
#define REPORT_MODEL          SINGLE_PERIOD
#define REPORT_PERIOD                  3600
//Frequency
#define FREQUENCY                     868e6
#define PATH_LOSS_EXP                  3.76
#define LOSS_REF                        8.1
/*
 * Energy Model Configuration
 */
//Voltage Supply in volts
#define VOLTAGE                         3.7
//Initial Energy of the battery in Joules
#define INITIAL_ENERGY                 3610

/*
 * Simulation configuration
 */
#define SIMULATION_TIME                3610

/*
 *  Statistics configuration
 */
#define LABELS                         true

/*
 *  Auto-configured parameteres, do not change it!
 */
#if TOPOLOGY == STAR
  #define N_GWS                           1
#elif TOPOLOGY == STAR_OF_STARS
  #define N_GWS                           3
#else
  #error  "Topology not supported"
#endif

#if REPORT_MODEL == SINGLE_PERIOD
  #define ED_APP_PERIOD        REPORT_PERIOD
#elif REPORT_MODEL == RANDOM_PERIOD
  #define ED_APP_PERIOD                   0
#else
  #error  "Report model not supported"
#endif


/*********************************************************************
 * Auxiliar functions
 *********************************************************************/


/*********************************************************************
 * Main Program - Set Urban Area Scenario
 *********************************************************************/

int main (int argc, char *argv[])
{
  /*********************************************************************
   * Enable Log Components
   *********************************************************************/
  LogComponentEnable ("LoraOpenArea", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraStatsHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
  LogComponentEnable ("HybridBuildingsPropagationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraConsumptionModel", LOG_LEVEL_ALL);
  LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);



  /*********************************************************************
   * Create mobility models
   *********************************************************************/
  //Ed mobility model
  std::ostringstream  uX, uY,uZ;
  uX << "ns3::UniformRandomVariable"  << "[Min="  << -SCENARIO_SIDE/2    << "|Max=" << SCENARIO_SIDE/2    << "]";
  uY << "ns3::UniformRandomVariable"  << "[Min="  << -SCENARIO_SIDE/2    << "|Max=" << SCENARIO_SIDE/2    << "]";
  uZ << "ns3::UniformRandomVariable"  << "[Min="  << ENDDEVICE_HEIGHT    << "|Max=" << ENDDEVICE_HEIGHT   << "]";

  Ptr<RandomBoxPositionAllocator>randomBoxAllocatorEd =  CreateObject<RandomBoxPositionAllocator> ();
  randomBoxAllocatorEd->SetAttribute("X",StringValue (uX.str())),
  randomBoxAllocatorEd->SetAttribute("Y",StringValue (uY.str())),
  randomBoxAllocatorEd->SetAttribute("Z",StringValue (uZ.str()));

  MobilityHelper mobilityEd;
  mobilityEd.SetPositionAllocator(randomBoxAllocatorEd);
  mobilityEd.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  //GW mobility model
  Ptr<ListPositionAllocator> fixAllocatorGw = CreateObject<ListPositionAllocator> ();
  //STAR CONFIGURATION
#if TOPOLOGY == STAR
  fixAllocatorGw->Add (Vector (0,0,GATEWAY_HEIGHT)); //Single GW in center
  //STAR OF STARS CONFIGURATION
#else
  fixAllocatorGw->Add (Vector (0,(SCENARIO_SIDE/4),GATEWAY_HEIGHT)); //Places 3GW uniform (aprox.)
  fixAllocatorGw->Add (Vector (SCENARIO_SIDE/4, -SCENARIO_SIDE/4,GATEWAY_HEIGHT));
  fixAllocatorGw->Add (Vector (-SCENARIO_SIDE/4,-SCENARIO_SIDE/4,GATEWAY_HEIGHT));
#endif
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
   * Create and configure End Devices
   *********************************************************************/
  NodeContainer endDevices;
  endDevices.Create (N_EDS);
  mobilityEd.Install(endDevices);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);


  /*********************************************************************
   * Create and configure Gateways
   *********************************************************************/
  //STAR CONFIGURATION
  NodeContainer gateways;
  gateways.Create (N_GWS);
  MobilityGw.Install (gateways);
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);


  /*********************************************************************
   *  Set spreading factors of End Devices
   *********************************************************************/
   macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);


  /*********************************************************************
   *  Install Application on End Devices
   *********************************************************************/
  Time stopReporting = Seconds (SIMULATION_TIME);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (ED_APP_PERIOD));
  ApplicationContainer appContainer = appHelper.Install (endDevices);
  appContainer.Start (Seconds (0));
  appContainer.Stop (stopReporting);

  /*********************************************************************
   *  Install Lora Energy Model on End Devices
   *********************************************************************/

  LoraEnergySourceHelper loraSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source
  loraSourceHelper.Set ("LoraEnergySourceInitialEnergyJ", DoubleValue (INITIAL_ENERGY));
  loraSourceHelper.Set ("LoraEnergySupplyVoltageV", DoubleValue (VOLTAGE));
  radioEnergyHelper.SetConsumptionModel ("ns3::InterpolatedLoraConsumptionModel");

  // install source on EDs' nodes
  EnergySourceContainer sources = loraSourceHelper.Install (endDevices);
  Names::Add ("/Names/EnergySource", sources.Get (0));


  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install
      (endDevicesNetDevices, sources);


  /*********************************************************************
   *  Start Simulation
   *********************************************************************/
  //Set Stop Time
  Simulator::Stop (Seconds (SIMULATION_TIME));

  //Run Simulation
  Simulator::Run ();

  //Collect statistics
  statsHelper.NodeInformation("src/lorawan/deployment/open-collect.dat",endDevices,gateways);
  statsHelper.EnergyInformation("src/lorawan/deployment/open-energy.dat",endDevices);
  statsHelper.GnuPlot2dScript ("src/lorawan/deployment/2d-open-deployment","open-collect.dat",false);
  statsHelper.GnuPlot3dScript ("src/lorawan/deployment/3d-open-deployment","open-collect.dat",false);
  statsHelper.GnuPlot2dScript ("src/lorawan/deployment/2d-open-deployment-labels","open-collect.dat",true);
  statsHelper.GnuPlot3dScript ("src/lorawan/deployment/3d-open-deployment-labels","open-collect.dat",true);

  NS_LOG_INFO("End of simulation");

  //Destroy simulation objects
  Simulator::Destroy ();


  return 0;
}

