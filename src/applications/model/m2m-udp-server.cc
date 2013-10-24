/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "seq-ts-header.h"
#include "m2m-udp-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("M2mUdpServer");
NS_OBJECT_ENSURE_REGISTERED(M2mUdpServer);

TypeId M2mUdpServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::M2mUdpServer").SetParent<Application>().AddConstructor<M2mUdpServer>().AddAttribute(
					"Port", "Port on which we listen for incoming packets.", UintegerValue(100),
					MakeUintegerAccessor(&M2mUdpServer::m_port), MakeUintegerChecker<uint16_t>()).AddAttribute(
					"MaxPacketDelay", "mac packet delay to compute packet loss", TimeValue(Seconds(0.0)),
					MakeTimeAccessor(&M2mUdpServer::m_maxDelay), MakeTimeChecker()).AddAttribute(
					"StatsStartTime", "time for init the stats", TimeValue(Seconds(0.0)),
					MakeTimeAccessor(&M2mUdpServer::m_statsStart), MakeTimeChecker());
	return tid;
}

M2mUdpServer::M2mUdpServer() :
		m_rxPackets(0), m_rxSize(0), m_lostPackets(0), m_lostSize(0), m_lostDelay(Time(0.0)), m_rxDelay(
				Time(0.0)) {
}

M2mUdpServer::~M2mUdpServer() {
}

Time M2mUdpServer::GetMaxPacketDelay() const {
	return m_maxDelay;
}

uint32_t M2mUdpServer::GetLostPackets() const {
	return m_lostPackets;
}

uint64_t M2mUdpServer::GetLostBytes() const {
	return m_lostSize;
}

uint32_t M2mUdpServer::GetReceivedPackets() const {
	return m_rxPackets;
}

uint64_t M2mUdpServer::GetReceivedBytes() const {
	return m_rxSize;
}

Time M2mUdpServer::GetReceivedSumDelay() const {
	return m_rxDelay;
}

Time M2mUdpServer::GetLostSumDelay() const {
	return m_lostDelay;
}

void M2mUdpServer::DoDispose(void) {
	NS_LOG_FUNCTION (this);
	Application::DoDispose();
}

void M2mUdpServer::StartApplication(void) {
	NS_LOG_FUNCTION (this);

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
		m_socket->Bind(local);
	}

	m_socket->SetRecvCallback(MakeCallback(&M2mUdpServer::HandleRead, this));

	if (m_socket6 == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket6 = Socket::CreateSocket(GetNode(), tid);
		Inet6SocketAddress local = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
		m_socket6->Bind(local);
	}

	m_socket6->SetRecvCallback(MakeCallback(&M2mUdpServer::HandleRead, this));

}

void M2mUdpServer::StopApplication(void) {
	NS_LOG_FUNCTION (this);

	if (m_socket != 0) {
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}
}

void M2mUdpServer::HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) {
		if (packet->GetSize() > 0) {
			uint32_t size = packet->GetSize();
			SeqTsHeader seqTs;
			packet->RemoveHeader(seqTs);
			Time timeTx = seqTs.GetTs();
			if (timeTx < m_statsStart)
				return;
			Time delayTime = Simulator::Now() - timeTx;
			NS_LOG_INFO("From " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " tx time " << timeTx.GetMilliSeconds()
					<< " rx time " << Simulator::Now().GetMilliSeconds()
					<< " delay " << delayTime.GetMilliSeconds() << " max delay " << m_maxDelay.GetMilliSeconds());
			if (delayTime > m_maxDelay && m_maxDelay.GetDouble() > 0.0) {
				m_lostPackets++;
				m_lostSize += size;
				m_lostDelay += delayTime;
			} else {
				m_rxPackets++;
				m_rxSize += size;
				m_rxDelay += delayTime;
			}
		}
	}
}

} // Namespace ns3
