/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#ifndef M2M_UDP_SERVER_HELPER_H
#define M2M_UDP_SERVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/m2m-udp-server.h"
namespace ns3 {
/**
 * \brief Create a server application which waits for input udp packets
 *        and uses the information carried into their payload to compute
 *        delay and to determine if some packets are lost.
 */
class M2mUdpServerHelper
{
public:
  /**
   * Create M2mUdpServerHelper which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   */
  M2mUdpServerHelper ();

  /**
   * Create M2mUdpServerHelper which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   * \param port The port the server will wait on for incoming packets
   */
  M2mUdpServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one udp server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);
  Ptr<M2mUdpServer> GetServer (void);
private:
  ObjectFactory m_factory;
  Ptr<M2mUdpServer> m_server;
};

} // namespace ns3

#endif /* M2M_UDP_SERVER_HELPER_H */
