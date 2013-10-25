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
#include <fstream>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

struct StatsTools_s {
	std::string type;
	NetDeviceContainer *ptr_netDevContainer;
	ApplicationContainer *ptr_appContainer;
	enum EpsBearer::Qci qci;
};

void UlSchedulingCallback(std::map<uint16_t, unsigned long> *ulTbMap, Time *startTime, std::string path,
		uint32_t frameNo, uint32_t subframeNo, uint16_t rnti, uint8_t mcs, uint16_t size) {
//	if (Simulator::Now() < *startTime) {
//		return;
//	}
	std::map<uint16_t, unsigned long>::iterator itMap = ulTbMap->find(rnti);
	if (itMap != ulTbMap->end()) {
		itMap->second += size;
	} else {
		ulTbMap->insert(std::pair<uint16_t, unsigned long>(rnti, size));
	}

//	std::cout << " Frame " << frameNo << " SubFrame " << subframeNo << " RNTI " << rnti << " MCS " << (int)mcs << " TB " << size << std::endl;
}

void ClientTxCallback(std::map<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> > *txMap,
		Time *startTime, Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
//	if (Simulator::Now() < *startTime) {
//		return;
//	}
	Ptr<NetDevice> netDevice = ipv4->GetNetDevice(interface);
	std::map<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> >::iterator itTx = txMap->find(netDevice);
	if (itTx != txMap->end()) {
		itTx->second.first++;
		itTx->second.second += packet->GetSize();
	} else {
		std::pair<unsigned int, unsigned long> value(1, packet->GetSize());
		txMap->insert(std::pair<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> >(netDevice, value));
	}
}

std::vector<EpsBearer> GetAvailableM2mRegularEpsBearers(double simulationTime, int minCqiIndex = 0, int maxCqiIndex = 11) {
	std::vector<EpsBearer> response;
	EpsBearer allBearers[] = { EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_1), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_2), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_3), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_4), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_5), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_6), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_7), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_8), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_9), EpsBearer(
			EpsBearer::NGBR_M2M_REGULAR_REPORT_10), EpsBearer(EpsBearer::NGBR_M2M_REGULAR_REPORT_11) };

	simulationTime = simulationTime * 1000; // s => ms
	for (int i = minCqiIndex; i < maxCqiIndex; i++) {
		if (allBearers[i].GetPacketDelayBudgetMs() < simulationTime) {
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
	double simTime = 1.0;
	double minRadius = 375;
	double maxRadius = 1000;
	unsigned int bandwidth = 25; // n RB
	Time statsStartTime = Seconds(0.300);
	unsigned int packetSizeM2m = 125; // bytes
	unsigned int packetSizeH2h = 1200; // bytes
	double interPacketM2mTrigger = 30; // s
//	double interPacketM2mTrigger = 0.025; // s
	double interPacketH2h = 75; // ms
	unsigned int minRBPerM2m = 3;
//	unsigned int minRBPerM2m = bandwidth;
	unsigned int minRBPerH2h = 3;
	double minPercentRBForM2m = (double) minRBPerM2m / bandwidth;
	bool harqEnabled = true;
	int currentExecution = 0;
	int minM2mRegularCqi = 0, maxM2mRegularCqi = 11;

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
	cmd.AddValue("nExec", "number of current execution", currentExecution);
	cmd.AddValue("intervalM2MTrigger", "interval (exponencial mean) between packet transmission for m2m Trigger", interPacketM2mTrigger);
	cmd.AddValue("minM2MRegularCqi", "min cqi index for m2m regular [0, 11]", minM2mRegularCqi);
	cmd.AddValue("maxM2MRegularCqi", "max cqi index for m2m regular [0, 11]", maxM2mRegularCqi);

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
//	LogComponentEnable("M2mMacScheduler", LOG_LEVEL_ALL);
//	LogComponentEnable("M2mUdpServer", LOG_LEVEL_INFO);
//	LogComponentEnable("M2mUdpClientApplication", LOG_LEVEL_INFO);
//	LogComponentEnable("Queue", LOG_LEVEL_ALL);

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
	std::vector<NetDeviceContainer> ueM2mRegularQciDevs;
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
	p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.0)));
	NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
//	p2ph.EnablePcapAll("m2m-p2p", true);
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
	ApplicationContainer allClientApps;
	ApplicationContainer allServerApps;
	ApplicationContainer h2hServerApps;
	ApplicationContainer m2mServerApps;
	ApplicationContainer m2mTriggerServerApps;
	ApplicationContainer m2mRegularServerApps;
	std::vector<ApplicationContainer> m2mRegularQciServerApps;
	std::map<Ptr<Node>, EpsBearer> ueBearerMap;

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
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds((int)interPacketH2h)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2h));

		Ptr<Node> node = ueH2hNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerH2h));
	}
	M2mUdpServerHelper h2hServerHelper(remoteH2hPort);
	h2hServerApps.Add(h2hServerHelper.Install(remoteHost));
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
		M2mUdpClientHelper appHelper(remoteHostAddr, remoteM2mTriggerPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds(0)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		appHelper.SetAttribute("RandomInterval",
				RandomVariableValue(ExponentialVariable(interPacketM2mTrigger)));

		Ptr<Node> node = ueM2mTriggerNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerM2mTriggerDefault));
	}
	M2mUdpServerHelper m2mTriggerServerHelper(remoteM2mTriggerPort);
	m2mTriggerServerApps.Add(m2mTriggerServerHelper.Install(remoteHost));
	m2mServerApps.Add(m2mTriggerServerApps);
	m2mTriggerServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerM2mTriggerDefault.GetPacketDelayBudgetMs())));
//	m2mTriggerServerHelper.GetServer()->SetAttribute("StatsStartTime", TimeValue(statsStartTime));

	// M2M Regular Report
	std::vector<EpsBearer> bearerM2mRegularList = GetAvailableM2mRegularEpsBearers(simTime, minM2mRegularCqi, maxM2mRegularCqi);
	ueM2mRegularQciDevs.resize(bearerM2mRegularList.size(), NetDeviceContainer());
	for (uint32_t u = 0; u < ueM2mRegularDevs.GetN(); u++) {
		Ptr<NetDevice> ueDevice = ueM2mRegularDevs.Get(u);
		Ptr<EpcTft> tft = Create<EpcTft>();
		EpcTft::PacketFilter pf;
		EpsBearer bearer = bearerM2mRegularList.at(u % bearerM2mRegularList.size());
		int port = remoteM2mRegularPort + (u % bearerM2mRegularList.size());
		Time interval = MilliSeconds(bearer.GetPacketDelayBudgetMs());
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(ueDevice->GetNode(), bearer));
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
		allClientApps.Add(appHelper.Install(ueDevice->GetNode()));
		ueM2mRegularQciDevs.at(u % bearerM2mRegularList.size()).Add(ueDevice);
	}
	for (unsigned int i = 0; i < bearerM2mRegularList.size(); i++) {
		int port = remoteM2mRegularPort + i;
		M2mUdpServerHelper m2mRegularServerHelper(port);
		m2mRegularQciServerApps.push_back(m2mRegularServerHelper.Install(remoteHost));
		m2mRegularServerApps.Add(m2mRegularQciServerApps.at(i));
		m2mServerApps.Add(m2mRegularServerApps);
		m2mRegularServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
				TimeValue(MilliSeconds(bearerM2mRegularList.at(i).GetPacketDelayBudgetMs())));
//		m2mRegularServerHelper.GetServer()->SetAttribute("StatsStartTime", TimeValue(statsStartTime));
	}

	allServerApps.Add(h2hServerApps);
	allServerApps.Add(m2mServerApps);

	allServerApps.Start(Seconds(0.01));
	allClientApps.Start(Seconds(0.01));

	allClientApps.Stop(Seconds(simTime - 0.05));

	std::map<uint16_t, unsigned long> ulTbMap;
	Config::Connect("/NodeList/*/DeviceList/*/LteEnbMac/UlScheduling",
			MakeBoundCallback(&UlSchedulingCallback, &ulTbMap, &statsStartTime));
	std::map<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> > clientTxMap;
	Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
			MakeBoundCallback(&ClientTxCallback, &clientTxMap, &statsStartTime));

//	Simulator::Stop(Seconds(simTime - 0.0001) + statsStartTime);
	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	// GtkConfigStore config;
	// config.ConfigureAttributes ();

	std::vector<StatsTools_s> statsToolsList;
	StatsTools_s toolAll, toolH2h, toolM2m, toolM2mTrigger, toolM2mRegular;
	toolAll.type = "ALL";
	toolAll.qci = EpsBearer::Qci(0);
	toolAll.ptr_appContainer = &allServerApps;
	toolAll.ptr_netDevContainer = &ueAllDevs;
	statsToolsList.push_back(toolAll);

	toolH2h.type = "H2H";
	toolH2h.qci = bearerH2h.qci;
	toolH2h.ptr_appContainer = &h2hServerApps;
	toolH2h.ptr_netDevContainer = &ueH2hDevs;
	statsToolsList.push_back(toolH2h);

	toolM2m.type = "M2M";
	toolM2m.qci = EpsBearer::Qci(0);
	toolM2m.ptr_appContainer = &m2mServerApps;
	toolM2m.ptr_netDevContainer = &ueM2mDevs;
	statsToolsList.push_back(toolM2m);

	toolM2mTrigger.type = "M2M Trigger";
	toolM2mTrigger.qci = bearerM2mTriggerDefault.qci;
	toolM2mTrigger.ptr_appContainer = &m2mTriggerServerApps;
	toolM2mTrigger.ptr_netDevContainer = &ueM2mTriggerDevs;
	statsToolsList.push_back(toolM2mTrigger);

	toolM2mRegular.type = "M2M Regular All";
	toolM2mRegular.qci = EpsBearer::Qci(0);
	toolM2mRegular.ptr_appContainer = &m2mRegularServerApps;
	toolM2mRegular.ptr_netDevContainer = &ueM2mRegularDevs;
	statsToolsList.push_back(toolM2mRegular);

	for (unsigned int i = 0; i < ueM2mRegularQciDevs.size(); i++) {
		StatsTools_s tool;
		tool.type = "M2M Regular";
		tool.qci = bearerM2mRegularList.at(i).qci;
		tool.ptr_appContainer = &(m2mRegularQciServerApps.at(i));
		tool.ptr_netDevContainer = &(ueM2mRegularQciDevs.at(i));
		statsToolsList.push_back(tool);
	}

	std::ostringstream ossGeral, ossUe;
	ossGeral << "m2m-stats-geral-s(" << scheduler << ")-h2h(" << ueH2hNodes.GetN() << ")-m2mT("
			<< ueM2mTriggerNodes.GetN() << ")-m2mR(" << ueM2mRegularNodes.GetN() << ")-" << currentExecution
			<< ".csv";
	std::ofstream statsGeralFile(ossGeral.str().c_str(), std::ios::out);
	statsGeralFile
			<< "Type; Size; Sim Time (s); Qci; TB (KiB); Avg TB; Throughput TB (kbps); Fairness TB; Tx Packets; Tx (KiB); "
			<< " Throughput Tx (kbps); Fairness Tx; "
			<< "Rx Packets; Rx (KiB); Rx Packets > Delay; Rx (KiB) > Max Delay; Packets Lost; "
			<< " Avg Delay (ms); Avg Delay > Max Delay (ms)\n";

	ossUe << "m2m-stats-device-s(" << scheduler << ")-h2h(" << ueH2hNodes.GetN() << ")-m2mT("
			<< ueM2mTriggerNodes.GetN() << ")-m2mR(" << ueM2mRegularNodes.GetN() << ")-" << currentExecution
			<< ".csv";
	std::ofstream statsUeFile(ossUe.str().c_str(), std::ios::out);
	statsUeFile << "RNTI;QCI index;TB bytes;Tx Packets;Tx bytes\n";

	for (std::vector<StatsTools_s>::iterator it = statsToolsList.begin(); it != statsToolsList.end(); it++) {
		NetDeviceContainer *ptrNetDevCont = it->ptr_netDevContainer;
		NetDeviceContainer::Iterator itNetDev;
		ApplicationContainer *ptrAppCont = it->ptr_appContainer;
		ApplicationContainer::Iterator itApp;
		int qciIndex = it->qci;
		unsigned int classSize = 0;
		long double tbBytes = 0;
		double avgTb = 0.0;
		double throughputTb = 0.0;
		double fairnessTb = 0.0;
		unsigned int txPackets = 0;
		long double txBytes = 0;
		double avgTx = 0.0;
		double throughputTx = 0.0;
		double fairnessTx = 0.0;
		unsigned int rxPackets = 0;
		long double rxBytes = 0;
		unsigned int rxPktExceedDelay = 0;
		unsigned long rxBytesExceedDelay = 0;
		unsigned int packetLost = 0;
		double avgDelay = 0.0;
		double avgExceedDelay = 0.0;

		for (itNetDev = ptrNetDevCont->Begin(); itNetDev != ptrNetDevCont->End(); itNetDev++) {
			uint16_t rnti = (*itNetDev)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
			std::map<uint16_t, unsigned long>::iterator it = ulTbMap.find(rnti);
			unsigned int devTbBytes = 0;
			unsigned int devTxPackets = 0;
			unsigned int devTxBytes = 0;
			if (it != ulTbMap.end()) {
				devTbBytes = it->second;
			}
			std::map<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> >::iterator itClientTx =
					clientTxMap.find(*itNetDev);
			if (itClientTx != clientTxMap.end()) {
				devTxPackets = itClientTx->second.first;
				devTxBytes = itClientTx->second.second;
			}

			tbBytes += devTbBytes / 1024; // bytes => kbytes
			txBytes += devTxBytes / 1024;
			fairnessTb += (devTbBytes / 1024) * (devTbBytes / 1024); // bytes => kbytes
			txPackets += devTxPackets;
			fairnessTx += (devTxBytes / 1024) * (devTxBytes / 1024);
		}
		int delayCount = 0;
		int delayExceedCount = 0;
		for (itApp = ptrAppCont->Begin(); itApp != ptrAppCont->End(); itApp++) {
			Ptr<M2mUdpServer> server = DynamicCast<M2mUdpServer>(*itApp);
			rxPackets += server->GetReceivedPackets() + server->GetLostPackets();
			rxBytes += (server->GetReceivedBytes() + server->GetLostBytes()) / 1024;
			rxPktExceedDelay += server->GetLostPackets();
			rxBytesExceedDelay += server->GetLostBytes() / 1024;
			if (server->GetReceivedPackets() > 0) {
				avgDelay += server->GetReceivedSumDelay().GetMilliSeconds() / server->GetReceivedPackets();
				delayCount++;
			}
			if (server->GetLostPackets() > 0) {
				avgExceedDelay += server->GetLostSumDelay().GetMilliSeconds() / server->GetLostPackets();
				delayExceedCount++;
			}
		}
		classSize = ptrNetDevCont->GetN();
		if (classSize > 0) {
			avgTb = tbBytes / classSize;
			avgTx = txBytes / classSize;
			throughputTb = 8 * avgTb / simTime;
			throughputTx = 8 * avgTx / simTime;
			if (fairnessTb != 0.0)
				// Convert tbBytes to kbytes
				fairnessTb = (tbBytes * tbBytes) / (fairnessTb * classSize); // Jain's fairness index
			if (fairnessTx != 0.0)
				fairnessTx = (txBytes * txBytes) / (fairnessTx * classSize); // Jain's fairness index
			fairnessTb = std::max(0.0, std::min(1.0, fairnessTb));
			fairnessTx = std::max(0.0, std::min(1.0, fairnessTx));
		}
		if (txPackets > rxPackets)
			packetLost = txPackets - rxPackets;
		if (delayCount > 0)
			avgDelay /= delayCount;
		if (delayExceedCount > 0)
			avgExceedDelay /= delayExceedCount;

		statsGeralFile << it->type << ";" << classSize << ";" << simTime << ";" << qciIndex << ";" << tbBytes
				<< ";" << avgTb << ";" << throughputTb << ";" << fairnessTb << ";" << txPackets << ";"
				<< txBytes << ";" << throughputTx << ";" << fairnessTx << ";" << rxPackets << ";" << rxBytes
				<< ";" << rxPktExceedDelay << ";" << rxBytesExceedDelay << ";" << packetLost << ";"
				<< avgDelay << ";" << avgExceedDelay << "\n";
	}

	for (NetDeviceContainer::Iterator itNetDev = ueAllDevs.Begin(); itNetDev != ueAllDevs.End(); itNetDev++) {
		uint16_t rnti = (*itNetDev)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
		std::map<uint16_t, unsigned long>::iterator it = ulTbMap.find(rnti);

		unsigned long devTbBytes = 0;
		int devQciIndex = 0;
		unsigned int devTxPackets = 0;
		unsigned long devTxBytes = 0;

		if (it != ulTbMap.end()) {
			devTbBytes = it->second;
		}
		std::map<Ptr<NetDevice>, std::pair<unsigned int, unsigned long> >::iterator itClientTx =
				clientTxMap.find(*itNetDev);
		if (itClientTx != clientTxMap.end()) {
			devTxPackets += itClientTx->second.first;
			devTxBytes += itClientTx->second.second;
		}
		std::map<Ptr<Node>, EpsBearer>::iterator itBearer = ueBearerMap.find((*itNetDev)->GetNode());
		if (itBearer != ueBearerMap.end()) {
			devQciIndex = itBearer->second.qci;
		}

		statsUeFile << rnti << ";" << devQciIndex << ";" << devTbBytes << ";" << devTxPackets << ";"
				<< devTxBytes << "\n";
	}

	Simulator::Destroy();
	statsGeralFile.close();
	statsUeFile.close();

	return 0;
}
