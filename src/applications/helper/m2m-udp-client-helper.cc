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
#include "m2m-udp-client-helper.h"
#include "ns3/udp-echo-client.h"
#include "ns3/m2m-udp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

M2mUdpClientHelper::M2mUdpClientHelper(Address address, uint16_t port) {
	m_factory.SetTypeId(M2mUdpClient::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(address));
	SetAttribute("RemotePort", UintegerValue(port));
}

M2mUdpClientHelper::M2mUdpClientHelper(Ipv4Address address, uint16_t port) {
	m_factory.SetTypeId(M2mUdpClient::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(Address(address)));
	SetAttribute("RemotePort", UintegerValue(port));
}

M2mUdpClientHelper::M2mUdpClientHelper(Ipv6Address address, uint16_t port) {
	m_factory.SetTypeId(M2mUdpClient::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(Address(address)));
	SetAttribute("RemotePort", UintegerValue(port));
}

void M2mUdpClientHelper::SetAttribute(std::string name, const AttributeValue &value) {
	m_factory.Set(name, value);
}

ApplicationContainer M2mUdpClientHelper::Install(NodeContainer c) const {
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
		Ptr<Node> node = *i;
		Ptr<M2mUdpClient> client = m_factory.Create<M2mUdpClient>();
		node->AddApplication(client);
		apps.Add(client);
	}
	return apps;
}

} // namespace ns3
