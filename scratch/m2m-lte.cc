/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/epc-helper.h"
#include "ns3/config-store.h"
#include "ns3/buildings-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

int main(int argc, char *argv[]) {
	uint16_t numberOfNodes = 1;
	double distance = 60.0;
	double simTime = 1.1;
	double interPacketInterval = 100;

	CommandLine cmd;
	cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("distance", "Distance between eNBs [m]", distance);
	cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	cmd.Parse(argc, argv);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

	// Parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper>();
	lteHelper->SetEpcHelper(epcHelper);
	Ptr<Node> pgw = epcHelper->GetPgwNode();

	// Uncomment to enable logging
//  lteHelper->EnableLogComponents ();
	LogComponentEnable("M2mMacScheduler", LOG_LEVEL_ALL);

	// Default scheduler is PF
	lteHelper->SetSchedulerType("ns3::M2mMacScheduler");

	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create(1);
	Ptr<Node> remoteHost = remoteHostContainer.Get(0);
	InternetStackHelper internet;
	internet.Install(remoteHostContainer);

	// Create the Internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
	NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
	// interface 0 is localhost, 1 is the p2p device
	Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(
			remoteHost->GetObject<Ipv4>());
	remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

	// Create Nodes: eNodeB and UE
	NodeContainer enbNodes;
	NodeContainer ueNodes;
	enbNodes.Create(1);
	ueNodes.Create(numberOfNodes);

	// Install Mobility Model
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
	for (uint16_t i = 0; i < numberOfNodes; i++) {
		positionAlloc->Add(Vector(distance * i, 0, 0));
	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(positionAlloc);
	mobility.Install(enbNodes);
	mobility.Install(ueNodes);

	// Install LTE Devices to the nodes
	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
	NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

	// Install the IP stack on the UEs
	internet.Install(ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
		Ptr<Node> ueNode = ueNodes.Get(u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
				ueNode->GetObject<Ipv4>());
		ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
	}

	// Attach one UE per eNodeB
	for (uint16_t i = 0; i < numberOfNodes; i++) {
		lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
		// side effect: the default EPS bearer will be activated
	}

	// Activate a data radio bearer
	EpsBearer bearer = EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
	Ptr<EpcTft> tft = Create<EpcTft> ();
	EpcTft::PacketFilter pf;
	pf.remoteAddress = remoteHostAddr;
	tft->Add (pf);
	lteHelper->ActivateDedicatedEpsBearer(ueLteDevs, bearer, tft);
	//	lteHelper->EnableTraces();

	// Install and start applications on UEs and remote host
	uint16_t ulPort = 2000;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;
	for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
		PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
				InetSocketAddress(Ipv4Address::GetAny(), ulPort));
		serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

		UdpClientHelper ulClient(remoteHostAddr, ulPort);
		ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
		ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
		ulClient.SetAttribute ("PacketSize", UintegerValue (1024));

		clientApps.Add(ulClient.Install(ueNodes.Get(u)));
	}
	serverApps.Start(Seconds(0.01));
	clientApps.Start(Seconds(0.01));
	lteHelper->EnableTraces();

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	// GtkConfigStore config;
	// config.ConfigureAttributes ();

	Simulator::Destroy();
	return 0;
}
