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

#include "lora-stats-helper.h"
#include "ns3/lora-net-device.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/node-list.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/config.h"
#include "ns3/file-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/lora-energy-source.h"
#include "ns3/lora-radio-energy-model.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/energy-source.h"
#include "ns3/buildings-module.h"
#include <iomanip>
#include <fstream>
#include <algorithm>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraStatsHelper");

LoraStatsHelper::LoraStatsHelper()
{
  m_prevTimeStamp = std::time (0);
  m_minutes = 0;
}

LoraStatsHelper::~LoraStatsHelper()
{
}

void LoraStatsHelper::NodeInformation (std::string fileName, NodeContainer endDevices, NodeContainer gateways)
{
  const char * name = fileName.c_str();
  std::ofstream nodeInformationFile;
  nodeInformationFile.open(name);
  NS_ASSERT(nodeInformationFile.is_open() == true);

  NS_LOG_DEBUG ("Collecting Node Information");
  //Print column info
  nodeInformationFile << "#Dev"          << " "
                      << "nodeId"        << " "
                      << "x"             << " "
                      << "y"             << " "
                      << "z"             << " "
                      << "SF"            << " "
                      << "DR"            << " "
                      << "ConsEnergy"    << " "
                      << "RemEnergy"     << " "
                      << std::endl;

  //End Devices Information
  for (NodeContainer::Iterator i = endDevices.Begin(); i!=endDevices.End(); i++)
    {
      Ptr<Node> node = *i;
      uint nodeId = node->GetId();

      //Get mobility info
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      NS_ASSERT (mobility != NULL);
      Vector position =  mobility->GetPosition ();

      //Get energy info
      Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer>();
      NS_ASSERT (energySourceContainer != NULL);
      Ptr<LoraEnergySource> loraEnergySource = DynamicCast<LoraEnergySource>(energySourceContainer->Get(0));
      NS_ASSERT (loraEnergySource != NULL);
      double remainingEnergyJ = loraEnergySource->GetRemainingEnergy();
      DeviceEnergyModelContainer deviceEnergyModelContainer = loraEnergySource->FindDeviceEnergyModels("ns3::LoraRadioEnergyModel");
      Ptr<LoraRadioEnergyModel> loraRadioEnergyModel = DynamicCast<LoraRadioEnergyModel>(deviceEnergyModelContainer.Get(0));
      NS_ASSERT (loraRadioEnergyModel != NULL);
      double consumedEnergyJ   = loraRadioEnergyModel->GetTotalEnergyConsumption();

      //Get lora-protocol info
      Ptr<NetDevice> netDevice = node->GetDevice(0);
      NS_ASSERT(netDevice != NULL);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice>();
      NS_ASSERT(loraNetDevice != NULL);
      Ptr<EndDeviceLoraMac> edMac= loraNetDevice->GetMac()->GetObject<EndDeviceLoraMac>();
      NS_ASSERT(edMac != NULL);
      uint  dataRate = edMac->GetDataRate();
      uint  spreadingFactor = edMac->GetSfFromDataRate(dataRate);
      //Print Info
      nodeInformationFile << "ED"             << " "
                          << nodeId           << " "
                          << position.x       << " "
                          << position.y       << " "
                          << position.z       << " "
                          << spreadingFactor  << " "
                          << dataRate         << " "
                          << consumedEnergyJ  << " "
                          << remainingEnergyJ << " "
                          << std::endl;
    }

  // Gateways Information
  for (NodeContainer::Iterator i = gateways.Begin (); i != gateways.End (); ++i)
    {
      Ptr<Node> node = *i;
      uint nodeId = node->GetId();

      //Get mobility info
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      NS_ASSERT (mobility != NULL);
      Vector position =  mobility->GetPosition ();

      //Print Info
      nodeInformationFile << "GW"             << " "
                          << nodeId           << " "
                          << position.x       << " "
                          << position.y       << " "
                          << position.z       << " "
                          << std::endl;
    }
}

void LoraStatsHelper::EnergyInformation (std::string fileName, NodeContainer endDevices)
{
  const char * name = fileName.c_str();
  std::ofstream energyInformationFile;
  energyInformationFile.open(name);

  NS_ASSERT(energyInformationFile.is_open() == true);

  NS_LOG_DEBUG ("Collecting Node Energy Information");
  //Print column info
  energyInformationFile << "#nodeId"                << " "
                        << "VoltageV"               << " "
                        << "totalTxS"               << " "
                        << "totalRxS"               << " "
                        << "totalStandbyS"          << " "
                        << "totalSleepS"            << " "
                        << "txCurrentA"             << " "
                        << "rxCurrentA"             << " "
                        << "standbyCurrentA"        << " "
                        << "sleepCurrentA"          << " "
                        << "txConsumedEnergy"       << " "
                        << "rxConsumedEnergy"       << " "
                        << "standbyConsumedEnergy"  << " "
                        << "sleepConsumedEnergy"    << " "
                        << "totalConsumedEnergy"    << " "
                        << "initialEnergyJ"         << " "
                        << "remainingEnergyJ"       << " "
                        << "SF"                     << " "
                        << std::endl;

  // Node common Information
  for (NodeContainer::Iterator i = endDevices.Begin (); i != endDevices.End (); ++i)
    {
      Ptr<Node> node = *i;
      uint nodeId = node->GetId();

      //Energy Source info
      Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer>();
      NS_ASSERT (energySourceContainer != NULL);
      Ptr<LoraEnergySource> loraEnergySource = DynamicCast<LoraEnergySource>(energySourceContainer->Get(0));
      NS_ASSERT (loraEnergySource != NULL);
      double remainingEnergyJ = loraEnergySource->GetRemainingEnergy();
      double initialEnergyJ   = loraEnergySource->GetInitialEnergy();
      double voltageV         = loraEnergySource->GetSupplyVoltage();

      //Energy Device info
      DeviceEnergyModelContainer deviceEnergyModelContainer = loraEnergySource->FindDeviceEnergyModels("ns3::LoraRadioEnergyModel");
      Ptr<LoraRadioEnergyModel> loraRadioEnergyModel = DynamicCast<LoraRadioEnergyModel>(deviceEnergyModelContainer.Get(0));
      NS_ASSERT (loraRadioEnergyModel != NULL);
      double totalTxS               = loraRadioEnergyModel->GetTotalTxTime().GetSeconds();
      double totalRxS               = loraRadioEnergyModel->GetTotalRxTime().GetSeconds();
      double totalStandbyS          = loraRadioEnergyModel->GetTotalStandbyTime().GetSeconds();
      double totalSleepS            = loraRadioEnergyModel->GetTotalSleepTime().GetSeconds();
      double txCurrentA             = loraRadioEnergyModel->GetTxCurrentA();
      double rxCurrentA             = loraRadioEnergyModel->GetRxCurrentA();
      double standbyCurrentA        = loraRadioEnergyModel->GetStandbyCurrentA();
      double sleepCurrentA          = loraRadioEnergyModel->GetSleepCurrentA();
      double txConsumedEnergyJ      = loraRadioEnergyModel->GetTxEnergyConsumption();
      double rxConsumedEnergyJ      = loraRadioEnergyModel->GetRxEnergyConsumption();
      double standbyConsumedEnergyJ = loraRadioEnergyModel->GetStandbyEnergyConsumption();
      double sleepConsumedEnergyJ   = loraRadioEnergyModel->GetSleepEnergyConsumption();
      double totalConsumedEnergyJ   = loraRadioEnergyModel->GetTotalEnergyConsumption();

      //Spreading Factor
      Ptr<NetDevice> netDevice = node->GetDevice(0);
      NS_ASSERT(netDevice != NULL);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice>();
      NS_ASSERT(loraNetDevice != NULL);
      Ptr<EndDeviceLoraMac> edMac= loraNetDevice->GetMac()->GetObject<EndDeviceLoraMac>();
      NS_ASSERT(edMac != NULL);
      uint  dataRate = edMac->GetDataRate();
      uint  spreadingFactor = edMac->GetSfFromDataRate(dataRate);

      //Print energy information
      energyInformationFile << nodeId                 << " "
                            << voltageV               << " "
                            << totalTxS               << " "
                            << totalRxS               << " "
                            << totalStandbyS          << " "
                            << totalSleepS            << " "
                            << txCurrentA             << " "
                            << rxCurrentA             << " "
                            << standbyCurrentA        << " "
                            << sleepCurrentA          << " "
                            << txConsumedEnergyJ      << " "
                            << rxConsumedEnergyJ      << " "
                            << standbyConsumedEnergyJ << " "
                            << sleepConsumedEnergyJ   << " "
                            << totalConsumedEnergyJ   << " "
                            << initialEnergyJ         << " "
                            << remainingEnergyJ       << " "
                            << spreadingFactor        << " "
                            << std::endl;
    }
}

void LoraStatsHelper::NodePosition(std::string fileName)
{
  const char * name = fileName.c_str();
  std::ofstream nodeInformationFile;
  nodeInformationFile.open(name);

  NS_ASSERT(nodeInformationFile.is_open() == true);

  NS_LOG_DEBUG ("Collecting Node positions");

  // Node common Information
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      uint nodeId = node->GetId();

      //Get mobility info
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      NS_ASSERT (mobility != NULL);
      Vector position =  mobility->GetPosition ();

      nodeInformationFile << "Node"     << " "
                          << nodeId     << " "
                          << position.x << " "
                          << position.y << " "
                          << position.z << " "
                          << std::endl;
    }

}


void LoraStatsHelper::Buildings2dInformation(std::string fileName)
{
  uint buildingIdx = 0;
  const char * name = fileName.c_str();
  std::ofstream buildingsInformationFile;
  buildingsInformationFile.open(name, std::ios_base::out | std::ios_base::trunc);
  NS_ASSERT(buildingsInformationFile.is_open() == true);

  NS_LOG_DEBUG ("Collecting 2D buildings Information");

  for (BuildingList::Iterator i = BuildingList::Begin (); i != BuildingList::End (); ++i)
    {
      ++buildingIdx;
      Ptr<Building> building = *i;
      Box coordinates = building->GetBoundaries ();
      buildingsInformationFile << " set object " << buildingIdx
                               << " rect from "  << coordinates.xMin  << "," << coordinates.yMin
                               << " to "         << coordinates.xMax  << "," << coordinates.yMax
                               << " front fs empty " //transparent to make node visibles
                               << std::endl;
    }
}


void LoraStatsHelper::Buildings3dInformation(std::string fileName)
{
  uint buildingIdx = 0;
  const char * name = fileName.c_str();
  std::ofstream buildingsInformationFile;
  buildingsInformationFile.open(name, std::ios_base::out | std::ios_base::trunc);
  NS_ASSERT(buildingsInformationFile.is_open() == true);

  NS_LOG_DEBUG ("Collecting 3D buildings Information");

  for (BuildingList::Iterator i = BuildingList::Begin (); i != BuildingList::End (); ++i)
    {
      ++buildingIdx;
      Ptr<Building> building = *i;
      Box coordinates = building->GetBoundaries ();
                               //Surface 1
      buildingsInformationFile << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << std::endl << std::endl
                               //Surface 2
                               << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << std::endl << std::endl
                               //Surface 3
                               << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << std::endl << std::endl
                               //Surface 4
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << std::endl << std::endl
                               //Surface 5
                               << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << coordinates.xMin << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMin << " " << coordinates.zMin << std::endl
                               << std::endl << std::endl
                               //Surface 6
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << coordinates.xMin << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMax << std::endl
                               << coordinates.xMax << " "  << coordinates.yMax << " " << coordinates.zMin << std::endl
                               << std::endl << std::endl;
    }
}

void LoraStatsHelper::GnuPlot2dScript (std::string scriptName, std::string dataName, bool labels)
{
  const char * script      = scriptName.c_str();
  const char * data        = dataName.c_str();
  std::ofstream scriptFile;
  scriptFile.open(script);
  NS_ASSERT(scriptFile.is_open() == true);
  std::vector<double> x,y;
  int xMax,xMin,yMax,yMin;

  //Nodes coordinates
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
     {
       Ptr<Node> node = *i;
       Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
       NS_ASSERT (mobility != NULL);
       Vector position =  mobility->GetPosition ();
       x.push_back(position.x);
       y.push_back(position.y);
     }
  uint margin = 100;
  xMax = static_cast<uint>(*std::max_element(x.begin(), x.end()))+margin;
  xMin = static_cast<uint>(*std::min_element(x.begin(), x.end()))-margin;
  yMax = static_cast<uint>(*std::max_element(y.begin(), y.end()))+margin;
  yMin = static_cast<uint>(*std::min_element(y.begin(), y.end()))-margin;



  NS_LOG_DEBUG ("Creating GnuPlot2dScript");
  if (labels == true)
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 7 \" size 1024, 768"                        << std::endl
                 << "set output  \"2dstats-labels.png\""                                          << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "set style rect fc lt -1 fs solid 0.15 "                                      << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set xrange [" << xMin << ":" << xMax << "]"                                  << std::endl
                 << "set yrange [" << yMin << ":" << yMax << "]"                                  << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "plot inputFile using 3:4:(stringcolumn(1) eq \"ED\" ? "
                 << "(sprintf(\"%d\",$6)): "
                 << "(sprintf(\"%s (%d, %d, %d)\",stringcolumn(1),$3,$4,$5))): "
                 << "(stringcolumn(1) eq \"ED\" ? $8 :1/0) "
                 << "with labels point  pt 7 palette offset char 1,1 notitle "                   << std::endl;
    }
  else
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 7 \" size 1024, 768"                        << std::endl
                 << "set output  \"2dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "set style rect fc lt -1 fs solid 0.15 "                                      << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set xrange [" << xMin << ":" << xMax << "]"                                  << std::endl
                 << "set yrange [" << yMin << ":" << yMax << "]"                                  << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "plot inputFile using 3:4:(stringcolumn(1) eq \"ED\" ? "
                 << "(sprintf(\"\")) : "
                 << "(sprintf(\"%s (%d, %d, %d)\",stringcolumn(1),$3,$4,$5))): "
                 << "(stringcolumn(1) eq \"ED\" ? $8 :1/0) "
                 << "with labels point  pt 7 palette offset char 1,1 notitle "                   << std::endl;
    }
}


void LoraStatsHelper::GnuPlot2dScript (std::string scriptName, std::string dataName, std::string buildingsName, bool labels)
{
  const char * script      = scriptName.c_str();
  const char * data        = dataName.c_str();
  const char * buildings   = buildingsName.c_str();
  std::ofstream scriptFile;
  scriptFile.open(script);
  NS_ASSERT(scriptFile.is_open() == true);
  std::vector<double> x,y;
  int xMax,xMin,yMax,yMin;


  //Buildings coordinates
  for (BuildingList::Iterator i = BuildingList::Begin (); i != BuildingList::End (); ++i)
     {
       Ptr<Building> building = *i;
       Box coordinates = building->GetBoundaries ();
       x.push_back(coordinates.xMax);
       x.push_back(coordinates.xMin);
       y.push_back(coordinates.yMax);
       y.push_back(coordinates.yMin);
     }

  //Nodes coordinates
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
     {
       Ptr<Node> node = *i;
       Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
       NS_ASSERT (mobility != NULL);
       Vector position =  mobility->GetPosition ();
       x.push_back(position.x);
       y.push_back(position.y);
     }


  uint margin = 100;
  xMax = static_cast<uint>(*std::max_element(x.begin(), x.end()))+margin;
  xMin = static_cast<uint>(*std::min_element(x.begin(), x.end()))-margin;
  yMax = static_cast<uint>(*std::max_element(y.begin(), y.end()))+margin;
  yMin = static_cast<uint>(*std::min_element(y.begin(), y.end()))-margin;




  NS_LOG_DEBUG ("Creating GnuPlot2dScript");
  if (labels == true)
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 7 \" size 1024, 768"                        << std::endl
                 << "set output  \"2dstats-labels.png\""                                          << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "set style rect fc lt -1 fs solid 0.15 "                                      << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "load '"<< buildings <<"'"                                                    << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set xrange [" << xMin << ":" << xMax << "]"                                  << std::endl
                 << "set yrange [" << yMin << ":" << yMax << "]"                                  << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "plot inputFile using 3:4:(stringcolumn(1) eq \"ED\" ? "
                // << "(sprintf(\"%s (%d, %d, %d),SF:%d\",stringcolumn(1),$3,$4,$5,$6)): "
                 << "(sprintf(\"%d\",$6)): "
                 << "(sprintf(\"%s (%d, %d, %d)\",stringcolumn(1),$3,$4,$5))): "
                 << "(stringcolumn(1) eq \"ED\" ? $8 :1/0) "
                 << "with labels point  pt 7 palette offset char 1,1 notitle "                   << std::endl;
    }
  else
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 7 \" size 1024, 768"                        << std::endl
                 << "set output  \"2dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "set style rect fc lt -1 fs solid 0.15 "                                      << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "load '"<< buildings <<"'"                                                    << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set xrange [" << xMin << ":" << xMax << "]"                                  << std::endl
                 << "set yrange [" << yMin << ":" << yMax << "]"                                  << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "plot inputFile using 3:4:(stringcolumn(1) eq \"ED\" ? "
                 << "(sprintf(\"\")) :"
                 << "(sprintf(\"%s (%d, %d, %d)\",stringcolumn(1),$3,$4,$5))): "
                 << "(stringcolumn(1) eq \"ED\" ? $8 :1/0) "
                 << "with labels point  pt 7 palette offset char 1,1 notitle "                   << std::endl;
    }
}


void LoraStatsHelper::GnuPlot3dScript (std::string scriptName, std::string dataName, bool labels)
{
  const char * script      = scriptName.c_str();
  const char * data        = dataName.c_str();
  std::ofstream scriptFile;
  scriptFile.open(script);
  NS_ASSERT(scriptFile.is_open() == true);

  NS_LOG_DEBUG ("Creating GnuPlot3dScript");
  if (labels == true)
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 5 \" size 1024, 768"                        << std::endl
                 << "set output  \"3dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set ticslevel 0"                                                             << std::endl
                 << "set autoscale"                                                               << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "set zlabel \"z\""                                                            << std::endl
                 << "splot inputFile using 3:4:5:(sprintf(\"%s(%d,%d,%d)\",stringcolumn(1),$3,$4,$5))"
                 << "with labels point pt 2 ps 1  offset char 1,1 notitle"                        << std::endl;
    }
  else
  {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 5 \" size 1024, 768"                        << std::endl
                 << "set output  \"3dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set ticslevel 0"                                                             << std::endl
                 << "set autoscale"                                                               << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "set zlabel \"z\""                                                            << std::endl
                 << "splot inputFile using 3:4:5:(sprintf(\"\")) "
                 << "with labels point pt 2 ps 1  offset char 1,1 notitle"                        << std::endl;

  }
}


void LoraStatsHelper::GnuPlot3dScript (std::string scriptName, std::string dataName, std::string buildingsName, bool labels)
{
  const char * script      = scriptName.c_str();
  const char * data        = dataName.c_str();
  const char * buildings   = buildingsName.c_str();
  std::ofstream scriptFile;
  scriptFile.open(script);
  NS_ASSERT(scriptFile.is_open() == true);

  NS_LOG_DEBUG ("Creating GnuPlot3dScript");
  if (labels == true)
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 5 \" size 1024, 768"                        << std::endl
                 << "set output  \"3dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "buildingsFile = '" << buildings << "'"                                       << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set ticslevel 0"                                                             << std::endl
                 << "set autoscale"                                                               << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "set zlabel \"z\""                                                            << std::endl
                 << "splot buildingsFile using 1:2:3 with lines linecolor rgb \"blue\","
                 << "inputFile using 3:4:5:(sprintf(\"%s(%d,%d,%d)\",stringcolumn(1),$3,$4,$5))"
                 << "with labels point pt 2 ps 1  offset char 1,1 notitle"                        << std::endl;
    }
  else
    {
      scriptFile << "reset"                                                                       << std::endl
                 << "set term pngcairo font \" Arial, 5 \" size 1024, 768"                        << std::endl
                 << "set output  \"3dstats.png\""                                                 << std::endl
                 << "set palette defined ( 0 'web-blue', 1 'green', 2 'greenyellow',3 'yellow',"
                 << "4 'yellow', 5 'goldenrod', 6 'orange', 7 'light-red', 8 'red')"              << std::endl
                 << "inputFile = '" << data << "'"                                                << std::endl
                 << "buildingsFile = '" << buildings << "'"                                       << std::endl
                 << "set offset 1,1,1,1"                                                          << std::endl
                 << "set ticslevel 0"                                                             << std::endl
                 << "set autoscale"                                                               << std::endl
                 << "set xlabel \"x\""                                                            << std::endl
                 << "set ylabel \"y\""                                                            << std::endl
                 << "set zlabel \"z\""                                                            << std::endl
                 << "splot buildingsFile using 1:2:3 with lines linecolor rgb \"blue\","
                 << "inputFile using 3:4:5:(sprintf("")) "
                 << "with labels point pt 2 ps 1  offset char 1,1 notitle"                        << std::endl;

  }
}

void LoraStatsHelper::SchedulePrintSimulationTime (void)
{
  Simulator::Schedule (Minutes (this->m_minutes), &LoraStatsHelper::PrintSimulationTime,this);
}

void LoraStatsHelper::SetTimeStamp(uint minutes)
{
  m_minutes = minutes;
}

void LoraStatsHelper::PrintSimulationTime(void)
{
  std::cout << "Time elapsed during simulation: " << Simulator::Now ().GetHours () << " hours" << std::endl;
  std::cout << "Time elapsed from last call: " << std::time (0) - m_prevTimeStamp << " seconds" << std::endl;
  m_prevTimeStamp = std::time (0);
}


}//Namespace ns3
