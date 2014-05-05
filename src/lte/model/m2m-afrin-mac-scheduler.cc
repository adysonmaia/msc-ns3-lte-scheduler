/*
 * m2m-afrin-mac-scheduler.cc
 *
 *  Created on: 30/04/2014
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/m2m-mac-scheduler.h>
#include <cfloat>
#include "m2m-afrin-mac-scheduler.h"

NS_LOG_COMPONENT_DEFINE("M2mAfrinMacScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mAfrinMacScheduler);

M2mAfrinMacScheduler::M2mAfrinMacScheduler() {
}

M2mAfrinMacScheduler::~M2mAfrinMacScheduler() {
}

void M2mAfrinMacScheduler::DoDispose(void) {
	M2mMacScheduler::DoDispose();
}

TypeId M2mAfrinMacScheduler::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mAfrinMacScheduler").SetParent<M2mMacScheduler>().AddConstructor<
			M2mAfrinMacScheduler>();
	return tid;
}

void M2mAfrinMacScheduler::SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	uint16_t rbEnd = rbStart + rbSize;
	uint16_t allocatedRbSize = 0;

	uint32_t maxBsr = 0;
	std::map<uint16_t, double> uePriorities;
	for (std::vector<uint16_t>::const_iterator itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(*itUe);
		if (itBsr != m_ceBsrRxed.end() && itBsr->second > maxBsr)
			maxBsr = itBsr->second;
	}
	for (std::vector<uint16_t>::const_iterator itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uePriorities.insert(std::pair<uint16_t, double>(*itUe, GetUlM2mPriority(*itUe, maxBsr)));
	}

	while (allocatedRbSize < rbSize || uePriorities.size() > 0) {
		uint16_t rntiChosen = 0;
		double priorityMax = 0.0;
		std::map<uint16_t, double>::const_iterator itPriority;

		for (itPriority = uePriorities.begin(); itPriority != uePriorities.end(); itPriority++) {
			uint16_t rnti = itPriority->first;
			double priority = itPriority->second;
			if (priority >= priorityMax) {
				priorityMax = priority;
				rntiChosen = rnti;
			}
		}

		if (rntiChosen == 0)
			break;

		std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(rntiChosen);
		uint32_t bsrSizeByte = (itBsr == m_ceBsrRxed.end()) ? 0 : itBsr->second;
		if (bsrSizeByte == 0) {
			uePriorities.erase(rntiChosen);
			continue;
		}
		std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rntiChosen);

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


			if (sizeByte >= bsrSizeByte || size == maxSize) {
				foundSize = size;
				foundMcs = mcs;
				break;
			}
		}

		if (foundSize > 0) {
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

			uePriorities.erase(uldci.m_rnti);
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

double M2mAfrinMacScheduler::GetUlM2mPriority(uint16_t rnti, uint32_t maxBsr)
{
	Time now = Simulator::Now();
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

	std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(rnti);
	uint32_t bsr = (itBsr != m_ceBsrRxed.end()) ? itBsr->second : 0;

	if (delay > 1 && maxBsr > 0) {
		return (bsr/static_cast<double>(maxBsr))*(1.0/delay);
	} else {
		return 1.0;
	}
}


void M2mAfrinMacScheduler::UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList,
		const M2mRbAllocationMap &rbMap, const std::map<uint16_t, uint32_t> &delayMap) {

}

} /* namespace ns3 */
