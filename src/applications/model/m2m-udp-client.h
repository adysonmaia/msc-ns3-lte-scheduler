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

#ifndef M2M_UDP_ECHO_CLIENT_H
#define M2M_UDP_ECHO_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup udpecho
 * \brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class M2mUdpClient: public Application {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;

	M2mUdpClient();

	virtual ~M2mUdpClient();

	/**
	 * \param ip destination ipv4 address
	 * \param port destination port
	 */
	void SetRemote(Address ip, uint16_t port);
	void SetRemote(Ipv4Address ip, uint16_t port);
	void SetRemote(Ipv6Address ip, uint16_t port);
protected:
	virtual void DoDispose(void);

private:

	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ScheduleTransmit(Time dt);
	void Send(void);

	void HandleRead(Ptr<Socket> socket);

	uint32_t m_count;
	Time m_interval;
	RandomVariable m_randInterval;
	double m_coefRandInterval;
	uint32_t m_size;
	uint32_t m_maxDelay;

	uint32_t m_sent;
	Ptr<Socket> m_socket;
	Address m_peerAddress;
	uint16_t m_peerPort;
	EventId m_sendEvent;
};

} // namespace ns3

#endif /* M2M_UDP_ECHO_CLIENT_H */
