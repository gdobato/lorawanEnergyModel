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

#include "lora-consumption-model.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraConsumptionModel");

NS_OBJECT_ENSURE_REGISTERED (LoraConsumptionModel);

TypeId
LoraConsumptionModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoraConsumptionModel")
    .SetParent<Object> ()
    .SetGroupName ("Lora")
  ;
  return tid;
}

LoraConsumptionModel::LoraConsumptionModel()
{
}

LoraConsumptionModel::~LoraConsumptionModel()
{
}


NS_OBJECT_ENSURE_REGISTERED (InterpolatedLoraConsumptionModel);

TypeId
InterpolatedLoraConsumptionModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::InterpolatedLoraConsumptionModel")
    .SetParent<LoraConsumptionModel> ()
    .SetGroupName ("Lora")
    .AddConstructor<InterpolatedLoraConsumptionModel> ();

  return tid;
}

InterpolatedLoraConsumptionModel::InterpolatedLoraConsumptionModel ()
{
  NS_LOG_FUNCTION (this);
}

InterpolatedLoraConsumptionModel::~InterpolatedLoraConsumptionModel()
{
  NS_LOG_FUNCTION (this);
}



double
InterpolatedLoraConsumptionModel::CalcTxCurrent (double power_dBm) const
{
  NS_LOG_FUNCTION (this << power_dBm);
  //Collect data from datasheet
  std::vector<double> power_dBm_lookup_table =      {7.0, 13.0, 17.0, 20.0 };
  std::vector<double> curruent_ma_lookup_table =   {18.0, 28.0, 90.0, 125.0};

  //Size of look up table elements
  int n_elements = power_dBm_lookup_table.size();

  //values which limits the value to be interpolated
  double current_ma_L, current_ma_R, power_dBm_L, power_dBm_R;

  //value of current result of interpolation
  double current_ma_interpolated;

  //Index
  int index = 0;

  NS_ASSERT ( power_dBm < power_dBm_lookup_table[n_elements - 1] || power_dBm  > power_dBm_lookup_table[0] );

  while ( power_dBm >power_dBm_lookup_table[index +1] )
  {
    index++;
  }

  //Prepare terms of the formula (TFM chapter 5)
  current_ma_L = curruent_ma_lookup_table[index];
  power_dBm_L  =  power_dBm_lookup_table[index];

  current_ma_R =curruent_ma_lookup_table[index +1];
  power_dBm_R  =  power_dBm_lookup_table[index +1];

  current_ma_interpolated = current_ma_L + (current_ma_R - current_ma_L)/(power_dBm_R - power_dBm_L) * (power_dBm - power_dBm_L);

  NS_LOG_DEBUG ("Input Power: "<< power_dBm << "dBm - Interpolated Current: " << current_ma_interpolated << " mA");

  return (current_ma_interpolated / 1000);

}

} // namespace ns3

