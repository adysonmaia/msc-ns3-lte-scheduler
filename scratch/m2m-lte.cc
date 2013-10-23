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
#include "ns3/flow-monitor-helper.h"
#include <iostream>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

void UlSchedulingCallback(std::map<uint16_t, int> *ulTbMap, Time *startTime, std::string path,
		uint32_t frameNo, uint32_t subframeNo, uint16_t rnti, uint8_t mcs, uint16_t size) {
//	if (Simulator::Now() < *startTime) {
//		return;
//	}
	std::map<uint16_t, int>::iterator itMap = ulTbMap->find(rnti);
	if (itMap != ulTbMap->end()) {
		itMap->second += size;
	} else {
		ulTbMap->insert(std::pair<uint16_t, int>(rnti, size));
	}

//	std::cout << " Frame " << frameNo << " SubFrame " << subframeNo << " RNTI " << rnti << " MCS " << (int)mcs << " TB " << size << std::endl;
}

void ClientTxCallback(std::map<Ptr<NetDevice>, int> *txMap, Time *startTime, Ptr<const Packet> packet,
		Ptr<Ipv4> ipv4, uint32_t interface) {
//	if (Simulator::Now() < *startTime) {
//		return;
//	}
	Ptr<NetDevice> netDevice = ipv4->GetNetDevice(interface);
	std::map<Ptr<NetDevice>, int>::iterator itTx = txMap->find(netDevice);
	if (itTx != txMap->end()) {
		itTx->second++;
	} else {
		txMap->insert(std::pair<Ptr<NetDevice>, int>(netDevice, 0));
	}
}

void ClientDropCallback(std::map<Ptr<NetDevice>, int> *dropMap, Time *startTime, const Ipv4Header &header,
		Ptr<const Packet> packet, Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t interface) {
	std::cout << " Drop " << reason << std::endl;
	if (Simulator::Now() < *startTime) {
		return;
	}
	Ptr<NetDevice> netDevice = ipv4->GetNetDevice(interface);
	std::map<Ptr<NetDevice>, int>::iterator it = dropMap->find(netDevice);
	if (it != dropMap->end()) {
		it->second++;
	} else {
		dropMap->insert(std::pair<Ptr<NetDevice>, int>(netDevice, 0));
	}
}

std::vector<EpsBearer> GetAvailableM2mRegularEpsBearers(double simulationTime) {
	std::vector<EpsBearer> response;
	EpsBearer allBearers[] = { EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_1), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_2), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_3), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_4), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_5), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_6), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_7), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_8), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_9), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_10), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_11) };

	simulationTime = simulationTime * 1000; // s => ms
	for (int i = 0; i < 11; i++) {
		if (allBearers[i].GetPacketDelayBudgetMs() <= simulationTime) {
			response.push_back(allBearers[i]);
		}
	}

	return response;
}

int main(int argc, char *argv[]) {
	int scheduler = 0;
	bool enableM2m = true;
	uint16_t nM2mTrigger = 150;
	uint16_t nM2mRegular = 50;
	uint16_t nH2h = 20;
//	double simTime = 1.0;
	double simTime = 0.5;
	double minRadius = 375;
	double maxRadius = 1000;
	unsigned int bandwidth = 25; // n RB
	Time statsStartTime = Seconds(0.300);
	unsigned int packetSizeM2m = 125; // bytes
	unsigned int packetSizeH2h = 1200; // bytes
//	double interPacketM2mTrigger = 30; // s
//	double interPacketM2mTrigger = 0.025; // s
	double interPacketM2mTrigger = 0.05; // s
	double interPacketH2h = 75; // ms
	unsigned int minRBPerM2m = 3;
	unsigned int minRBPerH2h = 3;
	double minPercentRBForM2m = (double) minRBPerM2m / bandwidth;
//	double minPercentRBForM2m = 0.0;
	bool harqEnabled = false;

	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(bandwidth));
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(bandwidth));

	CommandLine cmd;
	cmd.AddValue("scheduler", "Scheduler Type [0=M2M, 1=PF, 2=RR]", scheduler);
	cmd.AddValue("nM2MTrigger", "Number of UE to M2M event driven", nM2mTrigger);
	cmd.AddValue("nM2MRegular", "Number of UE to M2M time driven", nM2mRegular);
	cmd.AddValue("nH2H", "Number of UE to H2H", nH2h);
	cmd.AddValue("minRBPerM2M", "min resource block per M2M UE", minRBPerM2m);
	cmd.AddValue("minPercentRBForM2M", "min percentage of resource blocks available for M2M UE",
			minPercentRBForM2m);
	cmd.AddValue("minRBPerH2H", "min resource block demand per H2H UE", minRBPerH2h);
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

	switch (scheduler) {
	// RR
	case 2:
		lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
		enableM2m = false;
		break;
		// PF
	case 1:
		lteHelper->SetSchedulerType("ns3::M2mMacScheduler");
		enableM2m = false;
		break;
		// M2M
	case 0:
	default:
		lteHelper->SetSchedulerType("ns3::M2mMacScheduler");
		lteHelper->SetSchedulerAttribute("MinPercentRBForM2M", DoubleValue(minPercentRBForM2m));
		lteHelper->SetSchedulerAttribute("MinRBPerM2M", UintegerValue(minRBPerM2m));
		lteHelper->SetSchedulerAttribute("MinRBPerH2H", UintegerValue(minRBPerH2h));
		enableM2m = true;
		break;
	}
	lteHelper->SetSchedulerAttribute("UlGrantMcs", UintegerValue(7));
	lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(harqEnabled));

	// Uncomment to enable logging
//	lteHelper->EnableLogComponents();
	LogComponentEnable("M2mMacScheduler", LOG_LEVEL_ALL);
//	LogComponentEnable("M2mUdpServer", LOG_LEVEL_INFO);

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
//	p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1000Gb/s")));
	p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
//	p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
	p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.0)));
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
		UdpClientHelper appHelper(remoteHostAddr, remoteH2hPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketH2h)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2h));

		clientApps.Add(appHelper.Install(ueH2hNodes.Get(u)));
	}
	M2mUdpServerHelper h2hServerHelper(remoteH2hPort);
	h2hServerHelper.Install(remoteHost);
	h2hServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerH2h.GetPacketDelayBudgetMs())));
//	h2hServerHelper.GetServer()->SetAttribute("StatsStartTime", TimeValue(statsStartTime));

	// M2M Trigger Report
	EpsBearer bearerM2mTriggerDefault = EpsBearer(EpsBearer::NGBR_M2M_TRIGGER_REPORT);
	EpsBearer bearerM2mTrigger = bearerM2mTriggerDefault;
	if (!enableM2m)
		bearerM2mTrigger = EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
	Ptr<EpcTft> tftM2mTrigger = Create<EpcTft>();
	EpcTft::PacketFilter pfM2mTrigger;
	pfM2mTrigger.remoteAddress = remoteHostAddr;
	pfM2mTrigger.remotePortStart = remoteM2mTriggerPort;
	pfM2mTrigger.remotePortEnd = remoteM2mTriggerPort;
	tftM2mTrigger->Add(pfM2mTrigger);
	lteHelper->ActivateDedicatedEpsBearer(ueM2mTriggerDevs, bearerM2mTrigger, tftM2mTrigger);
	for (uint32_t u = 0; u < ueM2mTriggerNodes.GetN(); ++u) {
		M2mUdpEchoClientHelper appHelper(remoteHostAddr, remoteM2mTriggerPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(0)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		appHelper.SetAttribute("RandomInterval",
				RandomVariableValue(ExponentialVariable(interPacketM2mTrigger)));

		clientApps.Add(appHelper.Install(ueM2mTriggerNodes.Get(u)));
	}
	M2mUdpServerHelper m2mTriggerServerHelper(remoteM2mTriggerPort);
	m2mTriggerServerHelper.Install(remoteHost);
	m2mTriggerServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerM2mTriggerDefault.GetPacketDelayBudgetMs())));
//	m2mTriggerServerHelper.GetServer()->SetAttribute("StatsStartTime", TimeValue(statsStartTime));

	// M2M Regular Report
	std::vector<EpsBearer> bearerM2mRegularList = GetAvailableM2mRegularEpsBearers(simTime);
	std::vector<NetDeviceContainer> ueM2mRegularClassDevs;
	ueM2mRegularClassDevs.resize(bearerM2mRegularList.size(), NetDeviceContainer());
	for (uint32_t u = 0; u < ueM2mRegularDevs.GetN(); u++) {
		Ptr<NetDevice> ueDevice = ueM2mRegularDevs.Get(u);
		Ptr<EpcTft> tft = Create<EpcTft>();
		EpcTft::PacketFilter pf;
		EpsBearer bearer = bearerM2mRegularList.at(u % bearerM2mRegularList.size());
		int port = remoteM2mRegularPort + (u % bearerM2mRegularList.size());
		Time interval = MilliSeconds(bearer.GetPacketDelayBudgetMs());
		if (!enableM2m) {
			bearer = EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
		}
		pf.remoteAddress = remoteHostAddr;
		pf.remotePortStart = port;
		pf.remotePortEnd = port;
		tft->Add(pf);
		lteHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);
		UdpClientHelper appHelper(remoteHostAddr, port);
		appHelper.SetAttribute("Interval", TimeValue(interval));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		clientApps.Add(appHelper.Install(ueDevice->GetNode()));
		ueM2mRegularClassDevs.at(u % bearerM2mRegularList.size()).Add(ueDevice);
	}
	std::vector<M2mUdpServerHelper> m2mRegularServerHelperList;
	for (unsigned int i = 0; i < bearerM2mRegularList.size(); i++) {
		int port = remoteM2mRegularPort + i;
		M2mUdpServerHelper m2mRegularServerHelper(port);
		m2mRegularServerHelper.Install(remoteHost);
		m2mRegularServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
				TimeValue(MilliSeconds(bearerM2mRegularList.at(i).GetPacketDelayBudgetMs())));
//		m2mRegularServerHelper.GetServer()->SetAttribute("StatsStartTime", TimeValue(statsStartTime));
		m2mRegularServerHelperList.push_back(m2mRegularServerHelper);
	}

	serverApps.Start(Seconds(0.01));
	clientApps.Start(Seconds(0.01));

	std::map<uint16_t, int> ulTbMap;
	Config::Connect("/NodeList/*/DeviceList/*/LteEnbMac/UlScheduling",
			MakeBoundCallback(&UlSchedulingCallback, &ulTbMap, &statsStartTime));
	std::map<Ptr<NetDevice>, int> clientTxMap;
	Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
			MakeBoundCallback(&ClientTxCallback, &clientTxMap, &statsStartTime));
	std::map<Ptr<NetDevice>, int> clientDropMap;
	Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Drop",
			MakeBoundCallback(&ClientDropCallback, &clientDropMap, &statsStartTime));

//	Simulator::Stop(Seconds(simTime - 0.0001) + statsStartTime);
	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	// GtkConfigStore config;
	// config.ConfigureAttributes ();

	std::cout << "UL - Test with " << ueAllDevs.GetN() << " user(s). Scheduler " << scheduler << std::endl;
	std::cout << "H2H with " << ueH2hDevs.GetN() << " user(s)" << std::endl;
	double avgThroughput = 0.0;
	Ptr<M2mUdpServer> serverApp = h2hServerHelper.GetServer();
	double avgDelay = serverApp->GetReceivedSumDelay().GetMilliSeconds();
	double avgLostDelay = serverApp->GetLostSumDelay().GetMilliSeconds();
	int clientSumPacketTx = 0;
	for (unsigned int i = 0; i < ueH2hDevs.GetN(); i++) {
		uint16_t rnti = ueH2hDevs.Get(i)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
		int tb = 0;
		std::map<uint16_t, int>::iterator itTb = ulTbMap.find(rnti);
		if (itTb != ulTbMap.end())
			tb = itTb->second;
		avgThroughput += (8 * tb) / (simTime * 1024 * 1024); // to mbps

		std::map<Ptr<NetDevice>, int>::iterator itClientTx = clientTxMap.find(ueH2hDevs.Get(i));
		if (itClientTx != clientTxMap.end()) {
			clientSumPacketTx += itClientTx->second;
		}

//		std::cout << "\tRNTI " << rnti << " tb " << tb << "  thr " << (double) tb / simTime
//				<< std::endl;
	}
	std::cout << "\tAvg Throughput: " << (ueH2hDevs.GetN() > 0 ? avgThroughput / ueH2hDevs.GetN() : 0.0)
			<< " mbps" << std::endl;
	std::cout << "\tServer rx packets: " << serverApp->GetReceivedPackets() << std::endl;
	std::cout << "\tServer rx bytes: " << serverApp->GetReceivedBytes() << std::endl;
	std::cout << "\tServer lost packets: " << serverApp->GetLostPackets() << std::endl;
	std::cout << "\tServer avg delay: "
			<< ((serverApp->GetReceivedPackets() > 0) ? avgDelay / serverApp->GetReceivedPackets() : 0.0)
			<< " ms" << std::endl;
	std::cout << "\tServer avg packet lost delay: "
			<< ((serverApp->GetLostPackets() > 0) ? avgLostDelay / serverApp->GetLostPackets() : 0.0) << " ms"
			<< std::endl;
	std::cout << "\tClient tx packets: " << clientSumPacketTx << std::endl;

	std::cout << "\nM2M Trigger with " << ueM2mTriggerDevs.GetN() << " user(s)" << std::endl;
	avgThroughput = 0.0;
	serverApp = m2mTriggerServerHelper.GetServer();
	avgDelay = serverApp->GetReceivedSumDelay().GetMilliSeconds();
	avgLostDelay = serverApp->GetLostSumDelay().GetMilliSeconds();
	clientSumPacketTx = 0;
	for (unsigned int i = 0; i < ueM2mTriggerDevs.GetN(); i++) {
		uint16_t rnti = ueM2mTriggerDevs.Get(i)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
		int tb = 0;
		std::map<uint16_t, int>::iterator it = ulTbMap.find(rnti);
		if (it != ulTbMap.end())
			tb = it->second;
		avgThroughput += (8 * tb) / (simTime * 1024 * 1024); // to mbps

		std::map<Ptr<NetDevice>, int>::iterator itClientTx = clientTxMap.find(ueM2mTriggerDevs.Get(i));
		if (itClientTx != clientTxMap.end()) {
			clientSumPacketTx += itClientTx->second;
		}

//		std::cout << "\tRNTI " << rnti << " tb " << tb << "  thr " << (double) tb / simTime
//				<< std::endl;
	}
	std::cout << "\tAvg Throughput: "
			<< (ueM2mTriggerDevs.GetN() > 0 ? avgThroughput / ueM2mTriggerDevs.GetN() : 0.0) << " mbps"
			<< std::endl;
	std::cout << "\tServer rx packets: " << serverApp->GetReceivedPackets() << std::endl;
	std::cout << "\tServer rx bytes: " << serverApp->GetReceivedBytes() << std::endl;
	std::cout << "\tServer lost packets: " << serverApp->GetLostPackets() << std::endl;
	std::cout << "\tServer avg delay: "
			<< ((serverApp->GetReceivedPackets() > 0) ? avgDelay / serverApp->GetReceivedPackets() : 0.0)
			<< " ms" << std::endl;
	std::cout << "\tServer avg packet lost delay: "
			<< ((serverApp->GetLostPackets() > 0) ? avgLostDelay / serverApp->GetLostPackets() : 0.0) << " ms"
			<< std::endl;
	std::cout << "\tClient tx packets: " << clientSumPacketTx << std::endl;

	std::cout << "\nM2M Regular with " << ueM2mRegularDevs.GetN() << " user(s)" << std::endl;
	avgThroughput = 0.0;
	for (unsigned int i = 0; i < ueM2mRegularDevs.GetN(); i++) {
		uint16_t rnti = ueM2mRegularDevs.Get(i)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
		int tb = 0;
		std::map<uint16_t, int>::iterator it = ulTbMap.find(rnti);
		if (it != ulTbMap.end())
			tb = it->second;
		avgThroughput += (8 * tb) / (simTime * 1024 * 1024); // to mbps

//		std::cout << "\tRNTI " << rnti << " tb " << tb << "  thr " << (double) tb / simTime
//				<< std::endl;
	}
	std::cout << "\tAvg Throughput: "
			<< (ueM2mRegularDevs.GetN() > 0 ? avgThroughput / ueM2mRegularDevs.GetN() : 0.0) << " mbps"
			<< std::endl;
	for (unsigned int i = 0; i < bearerM2mRegularList.size(); i++) {
		serverApp = m2mRegularServerHelperList.at(i).GetServer();
		avgDelay = serverApp->GetReceivedSumDelay().GetMilliSeconds();
		avgLostDelay = serverApp->GetLostSumDelay().GetMilliSeconds();
		clientSumPacketTx = 0;

		std::cout << "\tQoS delay: " << serverApp->GetMaxPacketDelay().GetMilliSeconds() << " ms";
		std::cout << " with " << ueM2mRegularClassDevs.at(i).GetN() << " user(s)" << std::endl;
		std::cout << "\t\tServer rx packets: " << serverApp->GetReceivedPackets() << std::endl;
		std::cout << "\t\tServer rx bytes: " << serverApp->GetReceivedBytes() << std::endl;
		std::cout << "\t\tServer lost packets: " << serverApp->GetLostPackets() << std::endl;
		std::cout << "\t\tServer avg delay: "
				<< ((serverApp->GetReceivedPackets() > 0) ? avgDelay / serverApp->GetReceivedPackets() : 0.0)
				<< " ms" << std::endl;
		std::cout << "\t\tServer avg packet lost delay: "
				<< ((serverApp->GetLostPackets() > 0) ? avgLostDelay / serverApp->GetLostPackets() : 0.0)
				<< " ms" << std::endl;

		NetDeviceContainer netDev = ueM2mRegularClassDevs.at(i);
		NetDeviceContainer::Iterator itNetDev;
		std::map<Ptr<NetDevice>, int>::iterator itClientTx;
		for (itNetDev = netDev.Begin(); itNetDev != netDev.End(); itNetDev++) {
			itClientTx = clientTxMap.find(*itNetDev);
			if (itClientTx != clientTxMap.end()) {
				clientSumPacketTx += itClientTx->second;
			}
		}
		std::cout << "\t\tClient tx packets: " << clientSumPacketTx << std::endl;
	}

	Simulator::Destroy();
	return 0;
}
