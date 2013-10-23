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
#include "m2m-udp-server-helper.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

M2mUdpServerHelper::M2mUdpServerHelper ()
{
}

M2mUdpServerHelper::M2mUdpServerHelper (uint16_t port)
{
  m_factory.SetTypeId (M2mUdpServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
M2mUdpServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
M2mUdpServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<M2mUdpServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<M2mUdpServer>
M2mUdpServerHelper::GetServer (void)
{
  return m_server;
}

} // namespace ns3
