/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 *
 */

#ifndef M2M_UDP_SERVER_H
#define M2M_UDP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "packet-loss-counter.h"
namespace ns3 {
/**
 * \ingroup applications
 * \defgroup udpclientserver UdpClientServer
 */

/**
 * \ingroup udpclientserver
 * \class M2mUdpServer
 * \brief A Udp server. Receives UDP packets from a remote host. UDP packets
 * carry a 32bits sequence number followed by a 64bits time stamp in their
 * payloads. The application uses, the sequence number to determine if a packet
 * is lost, and the time stamp to compute the delay
 */
class M2mUdpServer: public Application {
public:
	static TypeId GetTypeId(void);
	M2mUdpServer();
	virtual ~M2mUdpServer();
	/**
	 * returns the number of lost packets
	 * \return the number of lost packets
	 */
	uint32_t GetLostPackets() const;

	/**
	 * \brief returns the number of received packets
	 * \return the number of received packets
	 */
	uint32_t GetReceivedPackets() const;

	uint64_t GetReceivedBytes() const;

	Time GetReceivedSumDelay() const;

	Time GetMaxPacketDelay() const;
protected:
	virtual void DoDispose(void);

private:

	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void HandleRead(Ptr<Socket> socket);

	uint16_t m_port;
	Ptr<Socket> m_socket;
	Ptr<Socket> m_socket6;
	Address m_local;
	uint32_t m_rxPackets;
	uint64_t m_rxSize;
	uint32_t m_lostPackets;
	Time m_maxDelay;
	Time m_rxDelay;
};

} // namespace ns3

#endif /* M2M_UDP_SERVER_H */
