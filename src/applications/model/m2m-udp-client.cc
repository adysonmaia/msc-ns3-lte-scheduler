/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "seq-ts-header.h"
#include "m2m-tag.h"
#include "m2m-udp-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("M2mUdpClientApplication");
NS_OBJECT_ENSURE_REGISTERED(M2mUdpClient);

TypeId M2mUdpClient::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::M2mUdpEchoClient").SetParent<Application>().AddConstructor<M2mUdpClient>().AddAttribute(
					"MaxPackets", "The maximum number of packets the application will send",
					UintegerValue(100), MakeUintegerAccessor(&M2mUdpClient::m_count),
					MakeUintegerChecker<uint32_t>()).AddAttribute("Interval",
					"The time to wait between packets", TimeValue(Seconds(1.0)),
					MakeTimeAccessor(&M2mUdpClient::m_interval), MakeTimeChecker()).AddAttribute(
					"RandomInterval", "An additional random time to wait between packets",
					RandomVariableValue(ConstantVariable(0.0)),
					MakeRandomVariableAccessor(&M2mUdpClient::m_randInterval), MakeRandomVariableChecker()).AddAttribute(
					"CoefficientOfRandomInterval",
					"The coefficient of the additional random time to wait between packets", DoubleValue(1.0),
					MakeDoubleAccessor(&M2mUdpClient::m_coefRandInterval), MakeDoubleChecker<double>()).AddAttribute(
					"RemoteAddress", "The destination Address of the outbound packets", AddressValue(),
					MakeAddressAccessor(&M2mUdpClient::m_peerAddress), MakeAddressChecker()).AddAttribute(
					"RemotePort", "The destination port of the outbound packets", UintegerValue(0),
					MakeUintegerAccessor(&M2mUdpClient::m_peerPort), MakeUintegerChecker<uint16_t>()).AddAttribute(
					"PacketSize",
					"Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
					UintegerValue(1024), MakeUintegerAccessor(&M2mUdpClient::m_size),
					MakeUintegerChecker<uint32_t>(12, 1500)).AddAttribute("MaxPacketDelay", "max delay",
					UintegerValue(0), MakeUintegerAccessor(&M2mUdpClient::m_maxDelay),
					MakeUintegerChecker<uint32_t>());
	return tid;
}

TypeId M2mUdpClient::GetInstanceTypeId(void) const {
	return GetTypeId();
}

M2mUdpClient::M2mUdpClient() {
	NS_LOG_FUNCTION (this);
	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId();
}

M2mUdpClient::~M2mUdpClient() {
	NS_LOG_FUNCTION (this);
	m_socket = 0;
}

void M2mUdpClient::SetRemote(Address ip, uint16_t port) {
	NS_LOG_FUNCTION (this << ip << port);
	m_peerAddress = ip;
	m_peerPort = port;
}

void M2mUdpClient::SetRemote(Ipv4Address ip, uint16_t port) {
	NS_LOG_FUNCTION (this << ip << port);
	m_peerAddress = Address(ip);
	m_peerPort = port;
}

void M2mUdpClient::SetRemote(Ipv6Address ip, uint16_t port) {
	NS_LOG_FUNCTION (this << ip << port);
	m_peerAddress = Address(ip);
	m_peerPort = port;
}

void M2mUdpClient::DoDispose(void) {
	NS_LOG_FUNCTION (this);
	Application::DoDispose();
}

void M2mUdpClient::StartApplication(void) {
	NS_LOG_FUNCTION (this);

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
			m_socket->Bind();
			m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
		} else if (Ipv6Address::IsMatchingType(m_peerAddress) == true) {
			m_socket->Bind6();
			m_socket->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
		}
	}

	m_socket->SetRecvCallback(MakeCallback(&M2mUdpClient::HandleRead, this));

	ScheduleTransmit(Seconds(m_coefRandInterval * m_randInterval.GetValue()));
}

void M2mUdpClient::StopApplication() {
	NS_LOG_FUNCTION (this);

	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket = 0;
	}

	Simulator::Cancel(m_sendEvent);
}

void M2mUdpClient::ScheduleTransmit(Time dt) {
	NS_LOG_FUNCTION (this << dt);
	m_sendEvent = Simulator::Schedule(dt, &M2mUdpClient::Send, this);
}

void M2mUdpClient::Send(void) {
	NS_LOG_FUNCTION(this);
	NS_ASSERT(m_sendEvent.IsExpired());
	SeqTsHeader seqTs;
	seqTs.SetSeq(m_sent);
	Ptr<Packet> p = Create<Packet>(m_size - (8 + 4)); // 8+4 : the size of the seqTs header
	p->AddHeader(seqTs);
	M2mTag m2mTag(m_maxDelay);
	p->AddByteTag(m2mTag);

	std::stringstream peerAddressStringStream;
	if (Ipv4Address::IsMatchingType(m_peerAddress)) {
		peerAddressStringStream << Ipv4Address::ConvertFrom(m_peerAddress);
	} else if (Ipv6Address::IsMatchingType(m_peerAddress)) {
		peerAddressStringStream << Ipv6Address::ConvertFrom(m_peerAddress);
	}

	if ((m_socket->Send(p)) >= 0) {
		++m_sent;
		NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
				<< peerAddressStringStream.str () << " Uid: "
				<< p->GetUid () << " Time: "
				<< (Simulator::Now ()).GetSeconds ());

	} else {
		NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
				<< peerAddressStringStream.str ());
	}

	if (m_sent < m_count) {
		ScheduleTransmit(m_interval + Seconds(m_coefRandInterval * m_randInterval.GetValue()));
	}
}

void M2mUdpClient::HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort ());
		} else if (Inet6SocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
					Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
					Inet6SocketAddress::ConvertFrom (from).GetPort ());
		}
	}
}

} // Namespace ns3
