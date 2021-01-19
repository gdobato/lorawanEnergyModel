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

#ifndef LORA_STATS_HELPER_H
#define LORA_STATS_HELPER_H

#include "ns3/node-container.h"
#include "ns3/buildings-module.h"
#include <ctime>

namespace ns3 {

class LoraStatsHelper
{
    /*
    *  Class to collect data and generate gnuplot scripts used in the Master Thesis 
    *  "Integración de un modelo de energía en la simulación de redes LoRaWAN en NS-3"
    */
public:

  LoraStatsHelper();
  ~LoraStatsHelper();

  void NodePosition (std::string fileName);
  void EnergyInformation (std::string fileName, NodeContainer endDevices);
  void NodeInformation (std::string fileName, NodeContainer endDevices, NodeContainer gateways);

  void Buildings2dInformation(std::string fileName);
  void Buildings3dInformation(std::string fileName);

  void GnuPlot2dScript (std::string scriptName, std::string dataName, bool labels);
  void GnuPlot3dScript (std::string scriptName, std::string dataName, bool labels);
  void GnuPlot2dScript (std::string scriptName, std::string dataName, std::string buildingsName, bool labels);
  void GnuPlot3dScript (std::string scriptName, std::string dataName, std::string buildingsName, bool labels);

  void SchedulePrintSimulationTime (void);
  void SetTimeStamp(uint minutes);

private:

  void PrintSimulationTime(void);

  time_t m_prevTimeStamp;
  uint   m_minutes;
};

} //namespace ns3

#endif /* LORA_STATS_HELPER_H */

