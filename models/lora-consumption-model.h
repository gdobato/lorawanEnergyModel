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

#ifndef LORA_CONSUMPTION_MODEL_H
#define LORA_CONSUMPTION_MODEL_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \ingroup energy
 *
 * \brief Modelize the consumption as a function of the transmit power
 *
 */
class LoraConsumptionModel : public Object
{
public:

  static TypeId GetTypeId (void);

  LoraConsumptionModel ();
  virtual ~LoraConsumptionModel ();

  virtual double CalcTxCurrent (double txPowerDbm) const = 0;
};


class InterpolatedLoraConsumptionModel : public LoraConsumptionModel
{
public:
  static TypeId GetTypeId (void);

  InterpolatedLoraConsumptionModel ();
  virtual ~InterpolatedLoraConsumptionModel ();

  double CalcTxCurrent (double txPowerDbm) const;


private:
  double m_txCurrent;
};

} // namespace ns3

#endif /* LORA_CONSUMPTION_MODEL_H */
