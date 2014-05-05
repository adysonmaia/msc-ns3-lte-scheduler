/*
 * m2m-giluka-mac-scheduler.cc
 *
 *  Created on: 01/05/2014
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/m2m-mac-scheduler.h>
#include <cfloat>
#include <algorithm>
#include "m2m-giluka-mac-scheduler.h"

NS_LOG_COMPONENT_DEFINE("M2mGilukaMacScheduler");

static uint32_t GetPacketDelay(ns3::EpsBearer::Qci qci) {
	return ns3::EpsBearer(qci).GetPacketDelayBudgetMs();
}

static bool comparePriority(const std::pair<uint16_t, uint32_t> &i, const std::pair<uint16_t, uint32_t> &j) {
	return (i.second > j.second);
}

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mGilukaMacScheduler);

M2mGilukaMacScheduler::M2mGilukaMacScheduler() {

}

M2mGilukaMacScheduler::~M2mGilukaMacScheduler() {
}

void M2mGilukaMacScheduler::DoDispose(void) {
	M2mMacScheduler::DoDispose();
}

TypeId M2mGilukaMacScheduler::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mGilukaMacScheduler").SetParent<M2mMacScheduler>().AddConstructor<
			M2mGilukaMacScheduler>();
	return tid;
}

void M2mGilukaMacScheduler::DoSchedUlTriggerReq(
		const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params) {
	RefreshUlCqiMaps();

	struct FfMacSchedSapUser::SchedUlConfigIndParameters response;
	M2mRbAllocationMap rbMap(m_cschedCellConfig.m_ulBandwidth);
	std::vector<uint16_t> harqList;

	Time now = Simulator::Now();
	std::vector<std::pair<uint32_t, std::vector<uint16_t> > > m2mClassList, h2hClassList;
	std::vector<std::pair<uint32_t, std::vector<uint16_t> > >::iterator itClass;
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_IMS), std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::GBR_CONV_VOICE),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::GBR_CONV_VIDEO),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_VIDEO_TCP_DEFAULT),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_6),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_7),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_8),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_9),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_10),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_11),
					std::vector<uint16_t>()));
	m2mClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_M2M_REGULAR_REPORT_12),
					std::vector<uint16_t>()));

	h2hClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_IMS), std::vector<uint16_t>()));
	h2hClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::GBR_CONV_VOICE),
					std::vector<uint16_t>()));
	h2hClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::GBR_CONV_VIDEO),
					std::vector<uint16_t>()));
	h2hClassList.push_back(
			std::pair<uint32_t, std::vector<uint16_t> >(GetPacketDelay(EpsBearer::NGBR_VIDEO_TCP_DEFAULT),
					std::vector<uint16_t>()));

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

		std::map<uint16_t, Time>::iterator itBsrTime = m_ceBsrRxedTime.find(rnti);
		uint32_t delay = GetUeUlMaxPacketDelay(rnti);
		if (itBsrTime != m_ceBsrRxedTime.end()) {
			uint32_t timeDiff = (now - (*itBsrTime).second).GetMilliSeconds();
			if (delay <= timeDiff) {
				delay = 0;
			} else {
				delay -= timeDiff;
			}
		}

		std::map<uint16_t, EpsBearer::Qci>::iterator itQci = m_ueUlQci.find(rnti);
		std::vector<std::pair<uint32_t, std::vector<uint16_t> > > *ptList;
		if (itQci != m_ueUlQci.end() && (*itQci).second > EpsBearer::NGBR_VIDEO_TCP_DEFAULT) {
			ptList = &m2mClassList;
		} else {
			ptList = &h2hClassList;
		}

		for (itClass = ptList->begin(); itClass != ptList->end(); itClass++) {
			if (delay <= itClass->first || (itClass + 1) == ptList->end()) {
				(*itClass).second.push_back(rnti);
			}
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

	for (int i = 0; i < m2mClassList.size(); i++) {
		if (i < h2hClassList.size()) {
			SchedUlClass(h2hClassList.at(i).second, rbMap, response);
		}
		SchedUlClass(m2mClassList.at(i).second, rbMap, response);
	}

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

void M2mGilukaMacScheduler::SchedUlClass(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	Time now = Simulator::Now();
	uint16_t rbSize = rbMap.GetAvailableRbSize();
	std::vector<std::pair<uint16_t, uint32_t> > uePriorities;
	std::vector<std::pair<uint16_t, uint32_t> >::iterator itPriority;
	for (std::vector<uint16_t>::const_iterator itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uint16_t rnti = *itUe;
		uint32_t timeDiff = 0;
		std::map<uint16_t, Time>::iterator itBsrTime = m_ceBsrRxedTime.find(rnti);
		if (itBsrTime != m_ceBsrRxedTime.end()) {
			timeDiff = (now - (*itBsrTime).second).GetMilliSeconds();
		}
		uePriorities.push_back(std::pair<uint16_t, uint32_t>(rnti, timeDiff));
	}

	std::sort(uePriorities.begin(), uePriorities.end(), comparePriority);
	for (itPriority = uePriorities.begin(); itPriority != uePriorities.end(); itPriority++) {
		uint16_t maxRbSize = rbMap.GetAvailableRbSize();
		if (maxRbSize == 0)
			break;

		uint16_t rnti = itPriority->first;
		std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(rnti);
		uint32_t bsrSizeByte = (itBsr == m_ceBsrRxed.end()) ? 0 : itBsr->second;
		if (bsrSizeByte == 0)
			continue;

		uint16_t rbStart = rbMap.GetFirstAvailableRb();
		std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
		uint16_t foundRbSize = 0;
		int foundMcs = 0;
		maxRbSize = std::min(maxRbSize, std::max(m_minM2mRb, static_cast<uint16_t>(rbSize / ueList.size())));

		for (uint16_t size = 1; size <= maxRbSize; size++) {
			int cqi = 0;
			if (itCqi != m_ueCqi.end()) {
				double spectralEfficiency = 0.0;
				double minSinr = DBL_MAX;
				for (uint16_t j = 0; j < size; j++) {
					double sinr = (*itCqi).second.at(rbStart + j);
					if (sinr == NO_SINR) {
						sinr = EstimateUlSinr(rnti, rbStart + j);
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

			if (sizeByte >= bsrSizeByte || size == maxRbSize) {
				foundRbSize = size;
				foundMcs = mcs;
				break;
			}
		}

		if (foundRbSize > 0) {
			UlDciListElement_s uldci;
			uldci.m_rnti = rnti;
			uldci.m_rbStart = rbStart;
			uldci.m_rbLen = foundRbSize;
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

			rbMap.Allocate(uldci.m_rnti, rbStart, uldci.m_rbLen);
			UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);
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
					std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find(
							uldci.m_rnti);
					if (itDci != m_ulHarqProcessesDciBuffer.end()) {
						(*itDci).second.at(harqId) = uldci;
					}
				}
			}
		}
	}
}

} /* namespace ns3 */
