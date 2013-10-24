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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef M2M_UDP_ECHO_HELPER_H
#define M2M_UDP_ECHO_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

/**
 * \brief create an application which sends a udp packet and waits for an echo of this packet
 */
class M2mUdpClientHelper {
public:
	/**
	 * Create UdpEchoClientHelper which will make life easier for people trying
	 * to set up simulations with echos.
	 *
	 * \param ip The IP address of the remote udp echo server
	 * \param port The port number of the remote udp echo server
	 */
	M2mUdpClientHelper(Address ip, uint16_t port);
	M2mUdpClientHelper(Ipv4Address ip, uint16_t port);
	M2mUdpClientHelper(Ipv6Address ip, uint16_t port);

	/**
	 * Record an attribute to be set in each Application after it is is created.
	 *
	 * \param name the name of the attribute to set
	 * \param value the value of the attribute to set
	 */
	void SetAttribute(std::string name, const AttributeValue &value);

	/**
	 * \param c the nodes
	 *
	 * Create one udp echo client application on each of the input nodes
	 *
	 * \returns the applications created, one application per input node.
	 */
	ApplicationContainer Install(NodeContainer c) const;

private:
	ObjectFactory m_factory;
};

} // namespace ns3

#endif /* M2M_UDP_ECHO_HELPER_H */
