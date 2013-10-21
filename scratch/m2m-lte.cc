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
#include <iostream>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

std::vector<EpsBearer> GetAvailableM2mRegularEpsBearers(double simulationTime) {
	std::vector<EpsBearer> response;
	EpsBearer allBearers[] = { EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_1), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_2), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_3), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_4), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_5), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_6), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_7), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_8), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_9), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_10), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_11) };

	simulationTime = simulationTime * 1000; // s => ms
	response.push_back(allBearers[0]);
	for (int i = 1; i < 11; i++) {
		if (allBearers[i].GetPacketDelayBudgetMs() <= simulationTime) {
			response.push_back(allBearers[i]);
		}
	}

	return response;
}

int main(int argc, char *argv[]) {
//	uint16_t nM2mTrigger = 50;
//	uint16_t nM2mRegular = 100;
	uint16_t nM2mTrigger = 1;
	uint16_t nM2mRegular = 1;
	uint16_t nH2h = 5;
	double simTime = 1.0;
	double minRadius = 0;
	double maxRadius = 500;
	unsigned int bandwidth = 25; // n RB
	double statsStartTime = 0.300;
	int packetSizeM2m = 125; // bytes
	int packetSizeH2h = 1200; // bytes
	double interPacketM2mTrigger = 30; // s
	double interPacketH2h = 75; // ms

	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(bandwidth));
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(bandwidth));

	CommandLine cmd;
	cmd.AddValue("nM2MTrigger", "Number of eNodeBs to M2M event driven", nM2mTrigger);
	cmd.AddValue("nM2MRegular", "Number of eNodeBs to M2M time driven", nM2mRegular);
	cmd.AddValue("nH2H", "Number of eNodeBs to H2H", nH2h);
	cmd.AddValue("cellRadius", "Radius [m] of cell", maxRadius);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.Parse(argc, argv);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

	// Parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper>();
	lteHelper->SetEpcHelper(epcHelper);
	Ptr<Node> pgw = epcHelper->GetPgwNode();

	// Default scheduler is PF
	lteHelper->SetSchedulerType("ns3::M2mMacScheduler");

	// Uncomment to enable logging
//	lteHelper->EnableLogComponents();
	LogComponentEnable("M2mMacScheduler", LOG_LEVEL_ALL);

	// Create Nodes: eNodeB and UE
	NodeContainer enbNodes;
	NodeContainer ueH2hNodes;
	NodeContainer ueM2mTriggerNodes;
	NodeContainer ueM2mRegularNodes;
	NodeContainer ueM2MNodes, ueAllNodes;
	enbNodes.Create(1);
	ueH2hNodes.Create(nH2h);
	ueM2mTriggerNodes.Create(nM2mTrigger);
	ueM2mRegularNodes.Create(nM2mRegular);
	ueM2MNodes.Add(ueM2mTriggerNodes);
	ueM2MNodes.Add(ueM2mRegularNodes);
	ueAllNodes.Add(ueH2hNodes);
	ueAllNodes.Add(ueM2MNodes);

	// Install Mobility Model
	Ptr<UniformRandomVariable> uniformRandomVar = CreateObject<UniformRandomVariable>();
	uniformRandomVar->SetAttribute("Min", DoubleValue(minRadius));
	uniformRandomVar->SetAttribute("Max", DoubleValue(maxRadius));
	Ptr<RandomDiscPositionAllocator> posAllocator = CreateObject<RandomDiscPositionAllocator>();
	posAllocator->SetX(0.0);
	posAllocator->SetY(0.0);
	posAllocator->SetRho(uniformRandomVar);
	MobilityHelper ueMobility;
	ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	ueMobility.SetPositionAllocator(posAllocator);
	ueMobility.Install(ueAllNodes);

	MobilityHelper enbMobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
	positionAlloc->Add(Vector(0, 0, 0));
	enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	enbMobility.SetPositionAllocator(positionAlloc);
	enbMobility.Install(enbNodes);

	// Install LTE Devices to the nodes
	NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice(enbNodes);
	NetDeviceContainer ueH2hDevs = lteHelper->InstallUeDevice(ueH2hNodes);
	NetDeviceContainer ueM2mTriggerDevs = lteHelper->InstallUeDevice(ueM2mTriggerNodes);
	NetDeviceContainer ueM2mRegularDevs = lteHelper->InstallUeDevice(ueM2mRegularNodes);
	NetDeviceContainer ueM2mDevs, ueAllDevs;
	ueM2mDevs.Add(ueM2mTriggerDevs);
	ueM2mDevs.Add(ueM2mRegularDevs);
	ueAllDevs.Add(ueH2hDevs);
	ueAllDevs.Add(ueM2mDevs);

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

	// Install the IP stack on the UEs
	internet.Install(ueAllNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address(ueAllDevs);
	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueAllNodes.GetN(); ++u) {
		Ptr<Node> ueNode = ueAllNodes.Get(u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
				ueNode->GetObject<Ipv4>());
		ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
	}

	// Attach one UE per eNodeB
	for (uint16_t i = 0; i < ueAllDevs.GetN(); i++) {
		lteHelper->Attach(ueAllDevs.Get(i), enbDevs.Get(0));
	}

	// Install and start applications on UEs and remote host
	uint16_t remoteH2hPort = 2000;
	uint16_t remoteM2mTriggerPort = 2001;
	uint16_t remoteM2mRegularPort = 2002;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;

	// H2H

	EpsBearer bearerH2h = EpsBearer(EpsBearer::GBR_CONV_VIDEO);
	Ptr<EpcTft> tftH2h = Create<EpcTft>();
	EpcTft::PacketFilter pfH2h;
	pfH2h.remoteAddress = remoteHostAddr;
	pfH2h.remotePortStart = remoteH2hPort;
	pfH2h.remotePortEnd = remoteH2hPort;
	tftH2h->Add(pfH2h);
	lteHelper->ActivateDedicatedEpsBearer(ueH2hDevs, bearerH2h, tftH2h);
	for (uint32_t u = 0; u < ueH2hNodes.GetN(); ++u) {
		PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
				InetSocketAddress(Ipv4Address::GetAny(), remoteH2hPort));
		serverApps.Add(packetSinkHelper.Install(remoteHost));

		// TODO definir
		UdpClientHelper appHelper(remoteHostAddr, remoteH2hPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketH2h)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2h));

		clientApps.Add(appHelper.Install(ueH2hNodes.Get(u)));
	}

	// M2M Trigger Report

	EpsBearer bearerM2mTrigger = EpsBearer(EpsBearer::NGBR_M2M_TRIGGER_REPORT);
	Ptr<EpcTft> tftM2mTrigger = Create<EpcTft>();
	EpcTft::PacketFilter pfM2mTrigger;
	pfM2mTrigger.remoteAddress = remoteHostAddr;
	pfM2mTrigger.remotePortStart = remoteM2mTriggerPort;
	pfM2mTrigger.remotePortEnd = remoteM2mTriggerPort;
	tftM2mTrigger->Add(pfM2mTrigger);
	lteHelper->ActivateDedicatedEpsBearer(ueM2mTriggerDevs, bearerM2mTrigger, tftM2mTrigger);
	for (uint32_t u = 0; u < ueM2mTriggerNodes.GetN(); ++u) {
		PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
				InetSocketAddress(Ipv4Address::GetAny(), remoteM2mTriggerPort));
		serverApps.Add(packetSinkHelper.Install(remoteHost));

		M2mUdpEchoClientHelper appHelper(remoteHostAddr, remoteM2mTriggerPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(0)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		appHelper.SetAttribute("RandomInterval", RandomVariableValue(ExponentialVariable(interPacketM2mTrigger)));

		clientApps.Add(appHelper.Install(ueM2mTriggerNodes.Get(u)));
	}

	// M2M Regular Report

	std::vector<EpsBearer> bearerM2mRegularList = GetAvailableM2mRegularEpsBearers(simTime);
	for (uint32_t u = 0; u < ueM2mRegularDevs.GetN(); u++) {
		Ptr<NetDevice> ueDevice = ueM2mRegularDevs.Get(u);
		Ptr<EpcTft> tft = Create<EpcTft>();
		EpcTft::PacketFilter pf;
		EpsBearer bearer = bearerM2mRegularList.at(u % bearerM2mRegularList.size());
		int port = remoteM2mRegularPort + (u % bearerM2mRegularList.size());
		pf.remoteAddress = remoteHostAddr;
		pf.remotePortStart = port;
		pf.remotePortEnd = port;
		tft->Add(pf);
		lteHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);
		UdpClientHelper appHelper(remoteHostAddr, port);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(bearer.GetPacketDelayBudgetMs())));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		clientApps.Add(appHelper.Install(ueDevice->GetNode()));
	}
	for (unsigned int i = 0; i < bearerM2mRegularList.size(); i++) {
		int port = remoteM2mRegularPort + i;
		PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
				InetSocketAddress(Ipv4Address::GetAny(), port));
		serverApps.Add(packetSinkHelper.Install(remoteHost));
	}

	serverApps.Start(Seconds(0.01));
	clientApps.Start(Seconds(0.01));

//	lteHelper->EnablePdcpTraces();
	lteHelper->EnableRlcTraces();
	lteHelper->EnableUlMacTraces();
	Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
	rlcStats->SetAttribute("StartTime", TimeValue(Seconds(statsStartTime)));
	rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(simTime)));

	Simulator::Stop(Seconds(simTime + statsStartTime - 0.0001));
	Simulator::Run();

	// GtkConfigStore config;
	// config.ConfigureAttributes ();

	std::cout << "UL - Test with " << ueAllDevs.GetN() << " user(s)" << std::endl;
	for (unsigned int i = 0; i < ueH2hDevs.GetN(); i++) {
		// get the imsi
		uint64_t imsi = ueH2hDevs.Get(i)->GetObject<LteUeNetDevice>()->GetImsi();
		// get the lcId
		uint8_t lcId = 4;
		uint64_t dataTx = rlcStats->GetUlTxData(imsi, lcId);
		std::cout << "\tH2H User " << i << " imsi " << imsi << " bytes txed " << dataTx
				<< "  thr " << (double) dataTx / simTime << std::endl;
	}
	for (unsigned int i = 0; i < ueM2mDevs.GetN(); i++) {
		// get the imsi
		uint64_t imsi = ueM2mDevs.Get(i)->GetObject<LteUeNetDevice>()->GetImsi();
		// get the lcId
		uint8_t lcId = 4;
		uint64_t dataTx = rlcStats->GetUlTxData(imsi, lcId);
		std::cout << "\tM2M User " << i << " imsi " << imsi << " bytes txed " << dataTx
				<< "  thr " << (double) dataTx / simTime << std::endl;
	}

	Simulator::Destroy();
	return 0;
}
