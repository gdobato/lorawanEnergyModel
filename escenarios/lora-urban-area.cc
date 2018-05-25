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

NS_LOG_COMPONENT_DEFINE ("LoraUrbanArea");

/*********************************************************************
 * Parameters configuration
 *********************************************************************/
/*
 * Scenario dimensions
 */
#define SCENARIO_SIDE                  4000

/*
 * Buildings layout configuration
 */
#define X_BUILDINGLENGHT                 60
#define Y_BUILDINGLENGHT                120
#define BUILDING_HEIGHT                  40
#define DELTAX_BUILDING                  40
#define DELTAY_BUILDING                  20
#define SQAUARELAYOUT_SIDE    SCENARIO_SIDE
#define BUILDINGGRID_SEPARATION         100

/*
 * Network configuration
 */
//Topology
#define STAR                              1
#define STAR_OF_STARS                     2
#define TOPOLOGY              STAR_OF_STARS
//Number of Nodes
#define N_EDS_INDOOR                    350
#define N_EDS_OUTDOOR                   350
#define N_EDS N_EDS_INDOOR + NS_EDS_OUTDOOR
//Gateway height
#define GATEWAY_HEIGHT                   45
#define ED_OUTDOOR_HEIGHT_MIN           1.5
#define ED_OUTDOOR_HEIGHT_MAX             5
//Ed Sending-Frequency Model
#define SINGLE_PERIOD                     1
#define RANDOM_PERIOD                     2
#define REPORT_MODEL          SINGLE_PERIOD
#define REPORT_PERIOD                   360
//Frequency
#define FREQUENCY                     868e6
/*
 * Energy Model Configuration
 */
//Voltage Supply in volts
#define VOLTAGE                        3.7
//Initial Energy of the battery in Joules
#define INITIAL_ENERGY                 5.5
/*
 * Simulation configuration
 */
#define SIMULATION_TIME                3600

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
/*
 * Function to Create buldings layout
 */
void CreateBuildings(void)
{
  double xGrid   = -(SQAUARELAYOUT_SIDE/2);
  double yGrid    = -(SQAUARELAYOUT_SIDE/2);
  double gridWidth = SQAUARELAYOUT_SIDE/(X_BUILDINGLENGHT + DELTAX_BUILDING);
  uint elements = static_cast<uint>((SQAUARELAYOUT_SIDE/(Y_BUILDINGLENGHT + DELTAY_BUILDING+BUILDINGGRID_SEPARATION))*gridWidth);
  double limitY = yGrid;

  //Configure Grid
  Ptr<GridBuildingAllocator>  gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (X_BUILDINGLENGHT));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (Y_BUILDINGLENGHT));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (DELTAX_BUILDING));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (DELTAY_BUILDING));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (BUILDING_HEIGHT));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (10));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (3));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (10));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (xGrid));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (yGrid));
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));

  //Limit scope
  while (limitY < 2000)
    {
      gridBuildingAllocator->Create (elements);
      limitY += ((elements/gridWidth)*Y_BUILDINGLENGHT) + ((elements/gridWidth-1)*DELTAY_BUILDING) + BUILDINGGRID_SEPARATION;
    }
}


/*********************************************************************
 * Main Program - Set Urban Area Scenario
 *********************************************************************/

int main (int argc, char *argv[])
{
  /*********************************************************************
   * Enable Log Components
   *********************************************************************/
  LogComponentEnable ("LoraUrbanArea", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraStatsHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
  LogComponentEnable ("HybridBuildingsPropagationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraConsumptionModel", LOG_LEVEL_ALL);
  LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);



  /*********************************************************************
   * Create buildings layout
   *********************************************************************/
   CreateBuildings();


  /*********************************************************************
   * Create mobility models
   *********************************************************************/
  //In/Outdoor Mobility for EDs
  std::ostringstream  u;
  u << "ns3::UniformRandomVariable"  << "[Min="  << ED_OUTDOOR_HEIGHT_MIN  << "|Max=" << ED_OUTDOOR_HEIGHT_MAX << "]";

  Ptr<OutdoorPositionAllocator>outdoorAllocatorEd =  CreateObject<OutdoorPositionAllocator> ();
  outdoorAllocatorEd->SetAttribute("X",StringValue ("ns3::UniformRandomVariable[Min=-2000.0|Max=2000.0]"));
  outdoorAllocatorEd->SetAttribute("Y",StringValue ("ns3::UniformRandomVariable[Min=-2000.0|Max=2000.0]"));
  outdoorAllocatorEd->SetAttribute("Z",StringValue (u.str()));

  MobilityHelper outdoorMobilityEd;
  outdoorMobilityEd.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  outdoorMobilityEd.SetPositionAllocator(outdoorAllocatorEd);

  //Indoor allocation
  Ptr<RandomBuildingPositionAllocator> indoorAllocatorEd =  CreateObject<RandomBuildingPositionAllocator> ();

  MobilityHelper indoorMobilityEd;
  indoorMobilityEd.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  indoorMobilityEd.SetPositionAllocator(indoorAllocatorEd);


  //GW mobility model
  double locMargin = 10.0;

  //STAR CONFIGURATION
#if TOPOLOGY == STAR
  std::ostringstream  uX, uY,uZ;
  uX << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin      << "|Max=" << locMargin      << "]";
  uY << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin      << "|Max=" << locMargin      << "]";
  uZ << "ns3::UniformRandomVariable"  << "[Min="  << GATEWAY_HEIGHT  << "|Max=" << GATEWAY_HEIGHT << "]";

  Ptr<OutdoorPositionAllocator>outdoorAllocatorGw1 =  CreateObject<OutdoorPositionAllocator> ();
  outdoorAllocatorGw1->SetAttribute("X",StringValue (uX.str())),
  outdoorAllocatorGw1->SetAttribute("Y",StringValue (uY.str())),
  outdoorAllocatorGw1->SetAttribute("Z",StringValue (uZ.str()));

  MobilityHelper outdoorMobilityGw1;
  outdoorMobilityGw1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  outdoorMobilityGw1.SetPositionAllocator(outdoorAllocatorGw1);

  //STAR OF STARS CONFIGURATION
#else
  std::ostringstream  uX, uY,uZ;
  uX << "ns3::UniformRandomVariable" << "[Min=" << -locMargin     << "|Max=" << +locMargin << "]";
  uY << "ns3::UniformRandomVariable" << "[Min=" << -locMargin + SCENARIO_SIDE/4  << "|Max=" << +locMargin + SCENARIO_SIDE/4 << "]";
  uZ << "ns3::UniformRandomVariable" << "[Min=" << GATEWAY_HEIGHT << "|Max=" << GATEWAY_HEIGHT << "]";

  Ptr<OutdoorPositionAllocator>outdoorAllocatorGw1 =  CreateObject<OutdoorPositionAllocator> ();
  outdoorAllocatorGw1->SetAttribute("X",StringValue (uX.str())),
  outdoorAllocatorGw1->SetAttribute("Y",StringValue (uY.str())),
  outdoorAllocatorGw1->SetAttribute("Z",StringValue (uZ.str()));

  MobilityHelper outdoorMobilityGw1;
  outdoorMobilityGw1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  outdoorMobilityGw1.SetPositionAllocator(outdoorAllocatorGw1);

  uX.str(""); uY.str(""); uZ.str("");
  uX << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin +SCENARIO_SIDE/4 << "|Max=" << locMargin +SCENARIO_SIDE/4 << "]";
  uY << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin -SCENARIO_SIDE/4 << "|Max=" << locMargin -SCENARIO_SIDE/4 << "]";;
  uZ << "ns3::UniformRandomVariable"  << "[Min="  << GATEWAY_HEIGHT  << "|Max=" << GATEWAY_HEIGHT << "]";

  Ptr<OutdoorPositionAllocator>outdoorAllocatorGw2 =  CreateObject<OutdoorPositionAllocator> ();
  outdoorAllocatorGw2->SetAttribute("X",StringValue (uX.str())),
  outdoorAllocatorGw2->SetAttribute("Y",StringValue (uY.str())),
  outdoorAllocatorGw2->SetAttribute("Z",StringValue (uZ.str()));

  MobilityHelper outdoorMobilityGw2;
  outdoorMobilityGw2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  outdoorMobilityGw2.SetPositionAllocator(outdoorAllocatorGw2);

  uX.str(""); uY.str(""); uZ.str("");
  uX << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin -SCENARIO_SIDE/4 << "|Max=" << locMargin -SCENARIO_SIDE/4  << "]";
  uY << "ns3::UniformRandomVariable"  << "[Min="  << -locMargin -SCENARIO_SIDE/4 << "|Max=" << locMargin -SCENARIO_SIDE/4  << "]";
  uZ << "ns3::UniformRandomVariable"  << "[Min="  << GATEWAY_HEIGHT  << "|Max=" << GATEWAY_HEIGHT << "]";

  Ptr<OutdoorPositionAllocator>outdoorAllocatorGw3 =  CreateObject<OutdoorPositionAllocator> ();
  outdoorAllocatorGw3->SetAttribute("X",StringValue (uX.str())),
  outdoorAllocatorGw3->SetAttribute("Y",StringValue (uY.str())),
  outdoorAllocatorGw3->SetAttribute("Z",StringValue (uZ.str()));

  MobilityHelper outdoorMobilityGw3;
  outdoorMobilityGw3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  outdoorMobilityGw3.SetPositionAllocator(outdoorAllocatorGw3);
#endif


  /*********************************************************************
   * Configure Lora Channel
   *********************************************************************/
  //Delay model
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  //LossModel
  Ptr<HybridBuildingsPropagationLossModel> hybridLoss = CreateObject<HybridBuildingsPropagationLossModel> ();
  hybridLoss->SetAttribute("Frequency",DoubleValue(FREQUENCY));
  hybridLoss->SetAttribute("Environment",StringValue("Urban"));
  hybridLoss->SetAttribute("CitySize",StringValue("Large"));
  hybridLoss->SetAttribute("RooftopLevel",DoubleValue(BUILDING_HEIGHT));

  //Create channel
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (hybridLoss, delay);


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
  //Outdoor Eds
  NodeContainer outdoorEds;
  outdoorEds.Create (N_EDS_OUTDOOR);
  outdoorMobilityEd.Install (outdoorEds);

  //Indoor Eds
  NodeContainer indoorEds;
  indoorEds.Create (N_EDS_INDOOR);
  indoorMobilityEd.Install (indoorEds);

  NodeContainer endDevices;
  endDevices.Add(outdoorEds);
  endDevices.Add(indoorEds);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);


  /*********************************************************************
   * Create and configure Gateways
   *********************************************************************/
  //STAR CONFIGURATION
#if TOPOLOGY == STAR
  NodeContainer gateways;
  gateways.Create (N_GWS);
  outdoorMobilityGw1.Install (gateways);
#else
  //STAR OF STARS CONFIGURATION
  NodeContainer gw1;
  gw1.Create(1);
  outdoorMobilityGw1.Install(gw1);
  NodeContainer gw2;
  gw2.Create(1);
  outdoorMobilityGw2.Install(gw2);
  NodeContainer gw3;
  gw3.Create(1);
  outdoorMobilityGw3.Install(gw3);

  NodeContainer gateways;
  gateways.Add(gw1);
  gateways.Add(gw2);
  gateways.Add(gw3);
#endif

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);


  /*********************************************************************
   * Install Devices in Buildings
   *********************************************************************/
  BuildingsHelper::Install (gateways);
  BuildingsHelper::Install (endDevices);
  BuildingsHelper::MakeMobilityModelConsistent ();


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
  statsHelper.NodeInformation("src/lorawan/deployment/urban-collect.dat",endDevices,gateways);
  statsHelper.EnergyInformation("src/lorawan/deployment/urban-energy.dat",endDevices);
  statsHelper.Buildings2dInformation("src/lorawan/deployment/2dBLayout.dat");
  statsHelper.Buildings3dInformation("src/lorawan/deployment/3dBLayout.dat");
  statsHelper.GnuPlot2dScript ("src/lorawan/deployment/2d-urban-deployment-labels","urban-collect.dat", "2dBLayout.dat",true);
  statsHelper.GnuPlot2dScript ("src/lorawan/deployment/2d-urban-deployment","urban-collect.dat", "2dBLayout.dat",false);
  statsHelper.GnuPlot3dScript ("src/lorawan/deployment/3d-urban-deployment","urban-collect.dat", "3dBLayout.dat",LABELS);

  NS_LOG_INFO("End of simulation");

  //Destroy simulation objects
  Simulator::Destroy ();


  return 0;
}
