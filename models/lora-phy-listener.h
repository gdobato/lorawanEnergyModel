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

#ifndef LORA_PHY_LISTENER_H
#define LORA_PHY_LISTENER_H

#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief Monitor operation of PHY-Lora
 */
class LoraPhyListener
{
public:
  virtual ~LoraPhyListener ()
  {
  };

  //Notify start of reception
  virtual void NotifyRxStart (void) = 0;

  //Notify start of transmission
  virtual void NotifyTxStart (double txPowerDbm) = 0;

  //Notify entry of sleep mode
  virtual void NotifySleep (void) = 0;

  //Notify entry of Standby mode
  virtual void NotifyStandby (void) = 0;
};

} //namespace ns3

#endif /* LORA_PHY_LISTENER_H */
