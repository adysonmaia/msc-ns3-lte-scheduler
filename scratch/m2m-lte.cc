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
#include "ns3/point-to-point-module.h"
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
	if (packet->GetSize() == 0)
		return;
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

std::vector<EpsBearer> GetAvailableM2mRegularEpsBearers(double simulationTime, int minCqiIndex = 0,
		int maxCqiIndex = 11) {
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

int GetM2mRegularEpsBearerIndex(uint32_t interval, double simulationTime, int minCqiIndex = 0,
		int maxCqiIndex = 11) {
	std::vector<EpsBearer> bearers = GetAvailableM2mRegularEpsBearers(simulationTime, minCqiIndex,
			maxCqiIndex);
	std::vector<EpsBearer>::iterator it;
	int index = 0;
	for (it = bearers.begin(); it != bearers.end(); it++) {
		if ((*it).GetPacketDelayBudgetMs() <= interval
				&& (it + 1 == bearers.end() || (*(it + 1)).GetPacketDelayBudgetMs() > interval)) {
			return index;
		}
		index++;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	int scheduler = 0;
	bool enableM2m = true;
	uint16_t nM2mTrigger = 150;
	uint16_t nM2mRegular = 50;
	uint16_t nH2h = 30;
	uint16_t nH2hVideo, nH2hVoip, nH2hFtp;
	double simTime = 1.0;
	double minRadius = 100;
	double maxRadius = 1400;
	unsigned int bandwidth = 25; // n RB
	Time statsStartTime = Seconds(0.300);
	unsigned int packetSizeM2m = 125; // bytes
	unsigned int packetSizeH2hVideo = 1200; // bytes
	unsigned int packetSizeH2hVoip = 40; // bytes
	unsigned int packetSizeH2hFtp = 256; // bytes
	double interPacketM2mTrigger = 0.05; // s
	double interPacketH2hVideo = 75; // ms
	double interPacketH2hVoip = 20; // ms
	double interPacketH2hFtp = 15.625; // ms
	unsigned int minRBPerM2m = 3;
	unsigned int minRBPerH2h = 3;
	double minPercentRBForM2m = (double) minRBPerM2m / bandwidth;
	bool harqEnabled = true;
	int currentExecution = 0;
	int minM2mRegularCqi = 0, maxM2mRegularCqi = 11;
	bool useM2mQosClass = true;
	std::string suffixStatsFile = "";

	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(bandwidth));
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(bandwidth));
	Config::SetDefault("ns3::LteEnbMac::NumberOfRaPreambles", UintegerValue(64));
	Config::SetDefault("ns3::LteEnbMac::PreambleTransMax", UintegerValue(200));

	CommandLine cmd;
	cmd.AddValue("scheduler", "Scheduler Type [0=M2M, 1=PF, 2=RR, 3=Lioumpas]", scheduler);
	cmd.AddValue("nM2MTrigger", "Number of UE to M2M event driven", nM2mTrigger);
	cmd.AddValue("nM2MRegular", "Number of UE to M2M time driven", nM2mRegular);
	cmd.AddValue("nH2H", "Number of UE to H2H", nH2h);
	cmd.AddValue("minRBPerM2M", "min resource block per M2M UE", minRBPerM2m);
	cmd.AddValue("minPercentRBForM2M", "min percentage of resource blocks available for M2M UE",
			minPercentRBForM2m);
	cmd.AddValue("minRBPerH2H", "min resource block demand per H2H UE", minRBPerH2h);
	cmd.AddValue("cellRadius", "Radius of cell", maxRadius);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("nExec", "number of current execution", currentExecution);
	cmd.AddValue("intervalM2MTrigger",
			"interval (exponencial mean) between packet transmission for m2m Trigger", interPacketM2mTrigger);
	cmd.AddValue("minM2MRegularCqi", "min cqi index for m2m regular [0, 11]", minM2mRegularCqi);
	cmd.AddValue("maxM2MRegularCqi", "max cqi index for m2m regular [0, 11]", maxM2mRegularCqi);
	cmd.AddValue("useM2MQoSClass", "use qos params from m2m class", useM2mQosClass);
	cmd.AddValue("suffixStatsFile", "", suffixStatsFile);

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
	// Lioumpas
	case 3:
		lteHelper->SetSchedulerType("ns3::M2mLioumpasMacScheduler");
		lteHelper->SetSchedulerAttribute("MinRBPerM2M", UintegerValue(minRBPerM2m));
		lteHelper->SetSchedulerAttribute("MinRBPerH2H", UintegerValue(minRBPerH2h));
		lteHelper->SetSchedulerAttribute("UseM2MQoSClass", BooleanValue(useM2mQosClass));
		enableM2m = true;
		break;
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
		lteHelper->SetSchedulerAttribute("UseM2MQoSClass", BooleanValue(useM2mQosClass));
		enableM2m = true;
		break;
	}
	lteHelper->SetSchedulerAttribute("UlGrantMcs", UintegerValue(12));
//	lteHelper->SetSchedulerAttribute("UlGrantMcs", UintegerValue(0));
	lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(harqEnabled));
	lteHelper->SetSchedulerAttribute("UlCqiFilter", EnumValue(FfMacScheduler::PUSCH_UL_CQI));

	// Uncomment to enable logging
//	lteHelper->EnableLogComponents();
//	LogComponentEnable("M2mMacScheduler", LOG_LEVEL_ALL);
//	LogComponentEnable("M2mUdpServer", LOG_LEVEL_INFO);
//	LogComponentEnable("M2mUdpClientApplication", LOG_LEVEL_INFO);
//	LogComponentEnable("LteRlcAm", LOG_LEVEL_INFO);

	RngSeedManager::SetRun(currentExecution + 1);

	nH2hVoip = nH2h / 3;
	nH2hFtp = nH2hVoip;
	nH2hVideo = nH2h - nH2hVoip - nH2hFtp;

	// Create Nodes: eNodeB and UE
	NodeContainer enbNodes;
	NodeContainer ueH2hVideoNodes, ueH2hVoipNodes, ueH2hFtpNodes;
	NodeContainer ueM2mTriggerNodes, ueM2mRegularNodes;
	NodeContainer ueH2hNodes, ueM2MNodes, ueAllNodes;
	enbNodes.Create(1);
	ueH2hVideoNodes.Create(nH2hVideo);
	ueH2hVoipNodes.Create(nH2hVoip);
	ueH2hFtpNodes.Create(nH2hFtp);
	ueH2hNodes.Add(ueH2hVideoNodes);
	ueH2hNodes.Add(ueH2hVoipNodes);
	ueH2hNodes.Add(ueH2hFtpNodes);
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

	//TODO teste
//	Ptr<MobilityModel> mobility = ueH2hNodes.Get(0)->GetObject<MobilityModel> ();
//	Vector position = mobility->GetPosition();
//	position.x = maxRadius;
//	position.y = 0.0;
//	mobility->SetPosition(position);

	// Install LTE Devices to the nodes
	NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice(enbNodes);
	NetDeviceContainer ueH2hVideoDevs = lteHelper->InstallUeDevice(ueH2hVideoNodes);
	NetDeviceContainer ueH2hVoipDevs = lteHelper->InstallUeDevice(ueH2hVoipNodes);
	NetDeviceContainer ueH2hFtpDevs = lteHelper->InstallUeDevice(ueH2hFtpNodes);
	NetDeviceContainer ueM2mTriggerDevs = lteHelper->InstallUeDevice(ueM2mTriggerNodes);
	NetDeviceContainer ueM2mRegularDevs = lteHelper->InstallUeDevice(ueM2mRegularNodes);
	NetDeviceContainer ueH2hDevs, ueM2mDevs, ueAllDevs;
	std::vector<NetDeviceContainer> ueM2mRegularQciDevs;
	ueH2hDevs.Add(ueH2hVideoDevs);
	ueH2hDevs.Add(ueH2hVoipDevs);
	ueH2hDevs.Add(ueH2hFtpDevs);
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
	uint16_t remoteH2hVideoPort = 2000;
	uint16_t remoteH2hVoipPort = 2001;
	uint16_t remoteH2hFtpPort = 2002;
	uint16_t remoteM2mTriggerPort = 2003;
	uint16_t remoteM2mRegularPort = 2004;
	ApplicationContainer allClientApps;
	ApplicationContainer allServerApps;
	ApplicationContainer h2hServerApps;
	ApplicationContainer m2mServerApps;
	ApplicationContainer h2hVideoServerApps, h2hVoipServerApps, h2hFtpServerApps;
	ApplicationContainer m2mTriggerServerApps, m2mRegularServerApps;
	std::vector<ApplicationContainer> m2mRegularQciServerApps;
	std::map<Ptr<Node>, EpsBearer> ueBearerMap;
	Ptr<EpcEnbApplication> enbApp = enbNodes.Get(0)->GetApplication(0)->GetObject<EpcEnbApplication>();
	Ptr<M2mSchedulerParam> schedulerParam = SimulationSingleton<M2mSchedulerParam>::Get();

	// H2H Video
	EpsBearer bearerH2hVideo = EpsBearer(EpsBearer::GBR_CONV_VIDEO);
	Ptr<EpcTft> tftH2hVideo = Create<EpcTft>();
	EpcTft::PacketFilter pfH2hVideo;
	pfH2hVideo.remoteAddress = remoteHostAddr;
	pfH2hVideo.remotePortStart = remoteH2hVideoPort;
	pfH2hVideo.remotePortEnd = remoteH2hVideoPort;
	tftH2hVideo->Add(pfH2hVideo);
	lteHelper->ActivateDedicatedEpsBearer(ueH2hVideoDevs, bearerH2hVideo, tftH2hVideo);
	for (uint32_t u = 0; u < ueH2hVideoNodes.GetN(); ++u) {
		M2mUdpClientHelper appHelper(remoteHostAddr, remoteH2hVideoPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds((int) interPacketH2hVideo)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2hVideo));
		appHelper.SetAttribute("CoefficientOfRandomInterval", DoubleValue(0.0));
		appHelper.SetAttribute("MaxPacketDelay", UintegerValue(bearerH2hVideo.GetPacketDelayBudgetMs()));

		Ptr<Node> node = ueH2hVideoNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerH2hVideo));
	}
	M2mUdpServerHelper h2hVideoServerHelper(remoteH2hVideoPort);
	h2hVideoServerApps.Add(h2hVideoServerHelper.Install(remoteHost));
	h2hVideoServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerH2hVideo.GetPacketDelayBudgetMs())));
	h2hServerApps.Add(h2hVideoServerApps);

	// H2H VoIP
	EpsBearer bearerH2hVoip = EpsBearer(EpsBearer::GBR_CONV_VOICE);
	Ptr<EpcTft> tftH2hVoip = Create<EpcTft>();
	EpcTft::PacketFilter pfH2hVoip;
	pfH2hVoip.remoteAddress = remoteHostAddr;
	pfH2hVoip.remotePortStart = remoteH2hVoipPort;
	pfH2hVoip.remotePortEnd = remoteH2hVoipPort;
	tftH2hVoip->Add(pfH2hVoip);
	lteHelper->ActivateDedicatedEpsBearer(ueH2hVoipDevs, bearerH2hVoip, tftH2hVoip);
	for (uint32_t u = 0; u < ueH2hVoipNodes.GetN(); ++u) {
		M2mUdpClientHelper appHelper(remoteHostAddr, remoteH2hVoipPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds((int) interPacketH2hVoip)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2hVoip));
		appHelper.SetAttribute("CoefficientOfRandomInterval", DoubleValue(0.0));
		appHelper.SetAttribute("MaxPacketDelay", UintegerValue(bearerH2hVoip.GetPacketDelayBudgetMs()));

		Ptr<Node> node = ueH2hVoipNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerH2hVoip));
	}
	M2mUdpServerHelper h2hVoipServerHelper(remoteH2hVoipPort);
	h2hVoipServerApps.Add(h2hVoipServerHelper.Install(remoteHost));
	h2hVoipServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerH2hVoip.GetPacketDelayBudgetMs())));
	h2hServerApps.Add(h2hVoipServerApps);

	// H2H FTP
	EpsBearer bearerH2hFtp = EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
	Ptr<EpcTft> tftH2hFtp = Create<EpcTft>();
	EpcTft::PacketFilter pfH2hFtp;
	pfH2hFtp.remoteAddress = remoteHostAddr;
	pfH2hFtp.remotePortStart = remoteH2hFtpPort;
	pfH2hFtp.remotePortEnd = remoteH2hFtpPort;
	tftH2hFtp->Add(pfH2hFtp);
	lteHelper->ActivateDedicatedEpsBearer(ueH2hFtpDevs, bearerH2hFtp, tftH2hFtp);
	for (uint32_t u = 0; u < ueH2hFtpNodes.GetN(); ++u) {
		M2mUdpClientHelper appHelper(remoteHostAddr, remoteH2hFtpPort);
		appHelper.SetAttribute("Interval", TimeValue(MilliSeconds((int) interPacketH2hFtp)));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeH2hFtp));
		appHelper.SetAttribute("CoefficientOfRandomInterval", DoubleValue(0.0));
		appHelper.SetAttribute("MaxPacketDelay", UintegerValue(bearerH2hFtp.GetPacketDelayBudgetMs()));

		Ptr<Node> node = ueH2hFtpNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerH2hFtp));
	}
	M2mUdpServerHelper h2hFtpServerHelper(remoteH2hFtpPort);
	h2hFtpServerApps.Add(h2hFtpServerHelper.Install(remoteHost));
	h2hFtpServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerH2hFtp.GetPacketDelayBudgetMs())));
	h2hServerApps.Add(h2hFtpServerApps);

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
		appHelper.SetAttribute("MaxPacketDelay",
				UintegerValue(bearerM2mTriggerDefault.GetPacketDelayBudgetMs()));

		Ptr<Node> node = ueM2mTriggerNodes.Get(u);
		allClientApps.Add(appHelper.Install(node));
		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(node, bearerM2mTriggerDefault));
	}
	M2mUdpServerHelper m2mTriggerServerHelper(remoteM2mTriggerPort);
	m2mTriggerServerApps.Add(m2mTriggerServerHelper.Install(remoteHost));
	m2mServerApps.Add(m2mTriggerServerApps);
	m2mTriggerServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
			TimeValue(MilliSeconds(bearerM2mTriggerDefault.GetPacketDelayBudgetMs())));

	// M2M Regular Report
	std::vector<EpsBearer> bearerM2mRegularList = GetAvailableM2mRegularEpsBearers(simTime, minM2mRegularCqi,
			maxM2mRegularCqi);
	uint32_t minM2mRegularInterval = bearerM2mRegularList[0].GetPacketDelayBudgetMs();
	uint32_t maxM2mRegularInterval =
			bearerM2mRegularList[bearerM2mRegularList.size() - 1].GetPacketDelayBudgetMs() + 50;
	maxM2mRegularInterval = std::min(maxM2mRegularInterval, static_cast<uint32_t>(1000 * simTime / 4));
	Ptr<UniformRandomVariable> m2mRegularRandom = CreateObject<UniformRandomVariable>();
	m2mRegularRandom->SetAttribute("Min", DoubleValue(minM2mRegularInterval));
	m2mRegularRandom->SetAttribute("Max", DoubleValue(maxM2mRegularInterval));
	ueM2mRegularQciDevs.resize(bearerM2mRegularList.size(), NetDeviceContainer());
	for (uint32_t u = 0; u < ueM2mRegularDevs.GetN(); u++) {
		Ptr<NetDevice> ueDevice = ueM2mRegularDevs.Get(u);
		Ptr<EpcTft> tft = Create<EpcTft>();
		EpcTft::PacketFilter pf;
		Time interval = MilliSeconds(m2mRegularRandom->GetInteger());
		int bearerIndex = GetM2mRegularEpsBearerIndex(interval.GetMilliSeconds(), simTime, minM2mRegularCqi,
				maxM2mRegularCqi);
		EpsBearer bearerDefault = bearerM2mRegularList.at(bearerIndex);
		EpsBearer bearer = bearerDefault;
		int port = remoteM2mRegularPort + bearerIndex;
		if (!enableM2m) {
			bearer = EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
		}
		pf.remoteAddress = remoteHostAddr;
		pf.remotePortStart = port;
		pf.remotePortEnd = port;
		tft->Add(pf);
		lteHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);

		M2mUdpClientHelper appHelper(remoteHostAddr, port);
		appHelper.SetAttribute("Interval", TimeValue(interval));
		appHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
		appHelper.SetAttribute("PacketSize", UintegerValue(packetSizeM2m));
		appHelper.SetAttribute("CoefficientOfRandomInterval", DoubleValue(0.0));
		appHelper.SetAttribute("MaxPacketDelay", UintegerValue(interval.GetMilliSeconds()));
		if (useM2mQosClass) {
//			appHelper.SetAttribute("MaxPacketDelay", UintegerValue(bearerDefault.GetPacketDelayBudgetMs()));
			schedulerParam->SetMaxPacketDelay(ueDevice, bearerDefault.GetPacketDelayBudgetMs());
		} else {
			schedulerParam->SetMaxPacketDelay(ueDevice, interval.GetMilliSeconds());
		}

		ueBearerMap.insert(std::pair<Ptr<Node>, EpsBearer>(ueDevice->GetNode(), bearerDefault));
		allClientApps.Add(appHelper.Install(ueDevice->GetNode()));
		ueM2mRegularQciDevs.at(bearerIndex).Add(ueDevice);
	}
	for (unsigned int i = 0; i < bearerM2mRegularList.size(); i++) {
		int port = remoteM2mRegularPort + i;
		M2mUdpServerHelper m2mRegularServerHelper(port);
		m2mRegularQciServerApps.push_back(m2mRegularServerHelper.Install(remoteHost));
		m2mRegularServerApps.Add(m2mRegularQciServerApps.at(i));
		m2mServerApps.Add(m2mRegularServerApps);
		m2mRegularServerHelper.GetServer()->SetAttribute("MaxPacketDelay",
				TimeValue(MilliSeconds(bearerM2mRegularList.at(i).GetPacketDelayBudgetMs())));
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

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	// GtkConfigStore config;
	// config.ConfigureAttributes ();

	std::vector<StatsTools_s> statsToolsList;
	StatsTools_s toolAll, toolH2h, toolM2m;
	StatsTools_s toolH2hVideo, toolH2hVoip, toolH2hFtp, toolM2mTrigger, toolM2mRegular;
	toolAll.type = "ALL";
	toolAll.qci = EpsBearer::Qci(0);
	toolAll.ptr_appContainer = &allServerApps;
	toolAll.ptr_netDevContainer = &ueAllDevs;
	statsToolsList.push_back(toolAll);

	toolH2h.type = "H2H All";
	toolH2h.qci = EpsBearer::Qci(0);
	toolH2h.ptr_appContainer = &h2hServerApps;
	toolH2h.ptr_netDevContainer = &ueH2hDevs;
	statsToolsList.push_back(toolH2h);

	toolH2hVideo.type = "H2H Video";
	toolH2hVideo.qci = bearerH2hVideo.qci;
	toolH2hVideo.ptr_appContainer = &h2hVideoServerApps;
	toolH2hVideo.ptr_netDevContainer = &ueH2hVideoDevs;
	statsToolsList.push_back(toolH2hVideo);

	toolH2hVoip.type = "H2H VoIP";
	toolH2hVoip.qci = bearerH2hVoip.qci;
	toolH2hVoip.ptr_appContainer = &h2hVoipServerApps;
	toolH2hVoip.ptr_netDevContainer = &ueH2hVoipDevs;
	statsToolsList.push_back(toolH2hVoip);

	toolH2hFtp.type = "H2H FTP";
	toolH2hFtp.qci = bearerH2hFtp.qci;
	toolH2hFtp.ptr_appContainer = &h2hFtpServerApps;
	toolH2hFtp.ptr_netDevContainer = &ueH2hFtpDevs;
	statsToolsList.push_back(toolH2hFtp);

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
	ossGeral << "m2m-stats-geral-s(" << scheduler << ")-c(" << useM2mQosClass << ")-h2h(" << ueH2hNodes.GetN()
			<< ")-m2mT(" << ueM2mTriggerNodes.GetN() << ")-m2mR(" << ueM2mRegularNodes.GetN() << ")-"
			<< currentExecution;
	if (suffixStatsFile.length() > 0) {
		ossGeral << "-" << suffixStatsFile;
	}
	ossGeral << ".csv";
	std::ofstream statsGeralFile(ossGeral.str().c_str(), std::ios::out);
	statsGeralFile
			<< "Type; Size; Sim Time (s); Qci; TB (KiB); Avg TB; Throughput TB (kbps); Fairness TB; Tx Packets; Tx (KiB); "
			<< " Throughput Tx (kbps); Fairness Tx; "
			<< "Rx Packets; Rx (KiB); Rx Packets > Delay; Rx (KiB) > Max Delay; Packets Lost; "
			<< " Avg Delay (ms); Avg Delay > Max Delay (ms)\n";

	ossUe << "m2m-stats-device-s(" << scheduler << ")-c(" << useM2mQosClass << ")-h2h(" << ueH2hNodes.GetN()
			<< ")-m2mT(" << ueM2mTriggerNodes.GetN() << ")-m2mR(" << ueM2mRegularNodes.GetN() << ")-"
			<< currentExecution;
	if (suffixStatsFile.length() > 0) {
		ossUe << "-" << suffixStatsFile;
	}
	ossUe << ".csv";
	std::ofstream statsUeFile(ossUe.str().c_str(), std::ios::out);
	statsUeFile << "RNTI;QCI index;TB bytes;Tx Packets;Tx bytes\n";

	for (std::vector<StatsTools_s>::iterator it = statsToolsList.begin(); it != statsToolsList.end(); it++) {
		NetDeviceContainer *ptrNetDevCont = it->ptr_netDevContainer;
		NetDeviceContainer::Iterator itNetDev;
//		ApplicationContainer *ptrAppCont = it->ptr_appContainer;
//		ApplicationContainer::Iterator itApp;
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
		long double rxBytesExceedDelay = 0;
		unsigned int packetLost = 0;
		double avgDelay = 0.0;
		double avgExceedDelay = 0.0;
		int delayCount = 0;
		int delayExceedCount = 0;

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

			tbBytes += devTbBytes / 1024.0; // bytes => kbytes
			txBytes += devTxBytes / 1024.0;
			fairnessTb += (devTbBytes / 1024.0) * (devTbBytes / 1024.0); // bytes => kbytes
			txPackets += devTxPackets;
			fairnessTx += (devTxBytes / 1024.0) * (devTxBytes / 1024.0);

			M2mEpcStats_s ueStats = enbApp->GetM2mEpcStats(rnti);
//			std::cout << "rnti " << rnti  << " tx packets  " << devTxPackets <<  " rx packets "  << ueStats.rx_packets <<  " lost " << ueStats.lost_packets << std::endl;

			rxPackets += ueStats.rx_packets + ueStats.lost_packets;
			rxBytes += (ueStats.rx_size + ueStats.lost_size) / 1024.0;
			rxPktExceedDelay += ueStats.lost_packets;
			rxBytesExceedDelay += ueStats.lost_size / 1024.0;
			if (ueStats.rx_packets > 0) {
				avgDelay += ueStats.rx_delay.GetMilliSeconds() / (double) ueStats.rx_packets;
				delayCount++;
			}
			if (ueStats.lost_packets > 0) {
				avgExceedDelay += ueStats.lost_delay.GetMilliSeconds() / (double) ueStats.lost_packets;
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
