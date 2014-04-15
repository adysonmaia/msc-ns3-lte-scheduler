/*
 * m2m-abdalla-mac-scheduler.cpp
 *
 *  Created on: 11/04/2014
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <cfloat>
#include <ns3/m2m-mac-scheduler.h>
#include <ns3/m2m-abdalla-mac-scheduler.h>
#include <math.h>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE("M2mAbdallaMacScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mAbdallaMacScheduler);

M2mAbdallaMacScheduler::M2mAbdallaMacScheduler() {
}

M2mAbdallaMacScheduler::~M2mAbdallaMacScheduler() {
}

void M2mAbdallaMacScheduler::DoDispose(void) {
	M2mMacScheduler::DoDispose();
}

TypeId M2mAbdallaMacScheduler::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mAbdallaMacScheduler").SetParent<M2mMacScheduler>().AddConstructor<
			M2mAbdallaMacScheduler>();
	return tid;
}

void M2mAbdallaMacScheduler::DoSchedUlTriggerReq(
		const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params) {
	RefreshUlCqiMaps();
	RefreshM2MAccessGrantTimers();

	struct FfMacSchedSapUser::SchedUlConfigIndParameters response;
	M2mRbAllocationMap rbMap(m_cschedCellConfig.m_ulBandwidth);
	std::vector<uint16_t> harqList, h2hList, m2mList;
	uint16_t h2hRbDemand = 0;

	if (m_harqOn && params.m_ulInfoList.size() > 0) {
		for (std::vector<UlInfoListElement_s>::const_iterator itUlInfo = params.m_ulInfoList.begin();
				itUlInfo != params.m_ulInfoList.end(); itUlInfo++) {
			if ((*itUlInfo).m_receptionStatus == UlInfoListElement_s::NotOk && m_harqOn) {
				harqList.push_back((*itUlInfo).m_rnti);
			}
		}
	}

	for (std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.begin(); itBsr != m_ceBsrRxed.end(); itBsr++) {
		uint16_t rnti = (*itBsr).first;

		if (m_harqOn) {
			bool isHarq = false;
			for (std::vector<uint16_t>::iterator itHarq = harqList.begin(); itHarq != harqList.end(); itHarq++) {
				if ((*itHarq) == rnti) {
					isHarq = true;
					break;
				}
			}
			if (isHarq)
				continue;
		}

		if ((*itBsr).second == 0) {
			continue;
		}

		std::map<uint16_t, EpsBearer::Qci>::iterator itQci = m_ueUlQci.find(rnti);
		if (itQci != m_ueUlQci.end() && (*itQci).second > EpsBearer::NGBR_VIDEO_TCP_DEFAULT) {
			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(rnti);
			if (itGrant != m_m2mGrantTimers.end() && (*itGrant).second > 0) {
				continue;
			}
			m2mList.push_back(rnti);
		} else {
			h2hList.push_back(rnti);
		}
	}

	// RACH
	for (uint16_t i = 0; i < rbMap.GetSize(); i++) {
		uint16_t rnti = m_rachAllocationMap.at(i);
		if (rnti != 0) {
			rbMap.Allocate(rnti, i);
		}
	}
	m_rachAllocationMap.clear();
	m_rachAllocationMap.resize(m_cschedCellConfig.m_ulBandwidth, 0);

	// HARQ
	if (m_harqOn && harqList.size() > 0) {
		SchedUlHarq(harqList, rbMap, response);
	}

//	std::cout << " Ul Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << std::endl;

	uint16_t nRbAvailable = rbMap.GetAvailableRbSize();
	uint16_t maxM2mRbSize = static_cast<uint16_t>(floor(nRbAvailable * m_minPercentM2mRb));
//	std::cout << "nRbAvailable " << nRbAvailable << " maxM2mRbSize " << maxM2mRbSize << " m2mList " << m2mList.size();
	SchedUlM2m(m2mList, rbMap, maxM2mRbSize, response);
	nRbAvailable = rbMap.GetAvailableRbSize();
//	std::cout << " nRbAvailable " << nRbAvailable << " h2hList " << h2hList.size() << std::endl;
	SchedUlH2h(h2hList, rbMap, nRbAvailable, response);

	// Update global UE stats
	// update UEs stats
	for (std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.begin(); itStats != m_flowStatsUl.end();
			itStats++) {
		double timeWindow = ((*itStats).second.isM2m) ? m_m2mTimeWindow : m_h2hTimeWindow;
		(*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTrasmitted;
		(*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / timeWindow))
				* (*itStats).second.lastAveragedThroughput)
				+ ((1.0 / timeWindow) * (double) ((*itStats).second.lastTtiBytesTrasmitted / 0.001));
		(*itStats).second.lastAverageResourcesAllocated = (1.0 / timeWindow)
				* (*itStats).second.lastTtiResourcesAllocated
				+ (1.0 - (1.0 / timeWindow)) * (*itStats).second.lastAverageResourcesAllocated;
		(*itStats).second.lastAveragedBsrReceived = (1.0 / timeWindow) * (*itStats).second.lastTtiBsrReceived
				+ (1.0 - (1.0 / timeWindow)) * (*itStats).second.lastAveragedBsrReceived;

		(*itStats).second.lastTtiBytesTrasmitted = 0;
		(*itStats).second.lastTtiResourcesAllocated = 0;
		(*itStats).second.lastTtiBsrReceived = 0;
	}

	m_ulAllocationMaps.insert(std::pair<uint16_t, M2mRbAllocationMap>(params.m_sfnSf, rbMap));
	m_schedSapUser->SchedUlConfigInd(response);
}

void M2mAbdallaMacScheduler::SchedUlM2m(const std::vector<uint16_t> &m2mList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {

	SchedUlUeType(m2mList, rbMap, rbSize, response);
}

void M2mAbdallaMacScheduler::SchedUlH2h(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {

	SchedUlUeType(ueList, rbMap, rbSize, response);
}

void M2mAbdallaMacScheduler::SchedUlUeType(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {

	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	uint16_t rbEnd = rbStart + rbSize;
	uint16_t allocatedRbSize = 0;

//	std::cout << "\n\nrbSize: " << rbSize << " rbStart: " << rbStart  << " rbEnd: " << rbEnd << "\n";

	std::map<uint16_t, uint32_t> ueDelays;
	for (std::vector<uint16_t>::const_iterator itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uint16_t rnti = *itUe;
		ueDelays.insert(std::pair<uint16_t, uint32_t>(rnti, GetUeUlMaxPacketDelay(rnti)));
	}

	while (allocatedRbSize < rbSize || ueDelays.size() > 0) {
		uint16_t rntiChosen = 0;
		uint32_t delayMin = UINT32_MAX;
		std::map<uint16_t, uint32_t>::const_iterator itDelay;

		for (itDelay = ueDelays.begin(); itDelay != ueDelays.end(); itDelay++) {
			uint16_t rnti = itDelay->first;
			uint32_t delay = itDelay->second;
			if (delay <= delayMin) {
				delayMin = delay;
				rntiChosen = rnti;
			}
		}

		if (rntiChosen == 0)
			break;

		std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(rntiChosen);
		uint32_t bsrSizeByte = (itBsr == m_ceBsrRxed.end()) ? 0 : itBsr->second;
		if (bsrSizeByte == 0) {
			ueDelays.erase(rntiChosen);
			continue;
		}
		std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rntiChosen);

		// TODO determinar a quantidade de recursos demandados por UE
		uint16_t foundSize = 0;
		int foundMcs = 0;
		uint16_t maxSize = rbEnd - rbStart;
		maxSize = std::min(maxSize, std::max(m_minM2mRb, static_cast<uint16_t>(rbSize/ueList.size())));
		for (uint16_t size = 1; size <= maxSize; size++) {
			int cqi = 0;
			if (itCqi != m_ueCqi.end()) {
				double spectralEfficiency = 0.0;
				double minSinr = DBL_MAX;
				for (uint16_t j = 0; j < size; j++) {
					double sinr = (*itCqi).second.at(rbStart + j);
					if (sinr == NO_SINR) {
						sinr = EstimateUlSinr(rntiChosen, rbStart + j);
					}
					if (sinr < minSinr) {
						minSinr = sinr;
					}
				}
				spectralEfficiency = log2(1.0 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
				cqi = m_amc->GetCqiFromSpectralEfficiency(spectralEfficiency);
			}
			int mcs = (cqi != 0) ? m_amc->GetMcsFromCqi(cqi) : m_ulGrantMcs;
			uint16_t sizeByte = m_amc->GetTbSizeFromMcs(mcs, size) / 8;

//			std::cout << " sizeByte: " << sizeByte << " bsrSizeByte: " << bsrSizeByte << " size: " << size << " - " << (rbEnd - rbStart - 1) << "\n";

			if (sizeByte >= bsrSizeByte || size == maxSize) {
				foundSize = size;
				foundMcs = mcs;
				break;
			}
		}

		if (foundSize > 0) {
//			std::cout << "rntiChosen: " << rntiChosen << " foundSize: " << foundSize << " delay: "<< ueDelays.at(rntiChosen) <<  "\n";
			UlDciListElement_s uldci;
			uldci.m_rnti = rntiChosen;
			uldci.m_rbStart = rbStart;
			uldci.m_rbLen = foundSize;
			uldci.m_mcs = foundMcs;
			uldci.m_tbSize = m_amc->GetTbSizeFromMcs(uldci.m_mcs, uldci.m_rbLen) / 8;
			uldci.m_ndi = 1;
			uldci.m_cceIndex = 0;
			uldci.m_aggrLevel = 1;
			uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
			uldci.m_hopping = false;
			uldci.m_n2Dmrs = 0;
			uldci.m_tpc = 0; // no power control
			uldci.m_cqiRequest = false; // only period CQI at this stage
			uldci.m_ulIndex = 0; // TDD parameter
			uldci.m_dai = 1; // TDD parameter
			uldci.m_freqHopping = 0;
			uldci.m_pdcchPowerOffset = 0; // not used

			ueDelays.erase(uldci.m_rnti);
			rbMap.Allocate(uldci.m_rnti, rbStart, uldci.m_rbLen);
			UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);
			allocatedRbSize += uldci.m_rbLen;
			rbStart += uldci.m_rbLen;
			response.m_dciList.push_back(uldci);

			std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(uldci.m_rnti);
			if (itStats != m_flowStatsUl.end()) {
				(*itStats).second.lastTtiBytesTrasmitted = uldci.m_tbSize;
				(*itStats).second.lastTtiResourcesAllocated = uldci.m_rbLen;
				(*itStats).second.lastTtiBsrReceived = uldci.m_tbSize;
			}

			if (m_harqOn == true) {
				std::map<uint16_t, uint8_t>::iterator itProcId;
				itProcId = m_ulHarqCurrentProcessId.find(uldci.m_rnti);
				if (itProcId != m_ulHarqCurrentProcessId.end()) {
					uint8_t harqId = (*itProcId).second;
					std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci =
							m_ulHarqProcessesDciBuffer.find(uldci.m_rnti);
					if (itDci != m_ulHarqProcessesDciBuffer.end()) {
						(*itDci).second.at(harqId) = uldci;
					}
				}
			}
		} else {
			break;
		}
	}
}

} /* namespace ns3 */
