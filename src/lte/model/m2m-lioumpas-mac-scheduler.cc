/*
 * m2m-lioumpas-mac-scheduler.cc
 *
 *  Created on: 31/10/2013
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <cfloat>
#include <ns3/m2m-lioumpas-mac-scheduler.h>
#include <ns3/m2m-mac-scheduler.h>
#include <ns3/integer.h>
#include <algorithm>
//#include "m2m-lioumpas-mac-scheduler.h"

NS_LOG_COMPONENT_DEFINE("M2mLioumpasMacScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mLioumpasMacScheduler);

static bool compareDelay(const std::pair<uint16_t, uint32_t> &i, const std::pair<uint16_t, uint32_t> &j) {
	return (i.second < j.second);
}

static bool compareSinr(const std::pair<double, std::pair<uint16_t, uint16_t> > &i,
		const std::pair<double, std::pair<uint16_t, uint16_t> > &j) {
	return (i.first > j.first);
}

M2mLioumpasMacScheduler::M2mLioumpasMacScheduler() {

}

M2mLioumpasMacScheduler::~M2mLioumpasMacScheduler() {

}

void M2mLioumpasMacScheduler::DoDispose(void) {
	M2mMacScheduler::DoDispose();
}

TypeId M2mLioumpasMacScheduler::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mLioumpasMacScheduler").SetParent<M2mMacScheduler>().AddConstructor<
			M2mLioumpasMacScheduler>().AddAttribute("Version", "Version of the algorithm", UintegerValue(2),
			MakeUintegerAccessor(&M2mLioumpasMacScheduler::m_version), MakeUintegerChecker<uint8_t>(1, 2));
	return tid;
}

void M2mLioumpasMacScheduler::SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	if (m_version == 1) {
		SchedUlM2mV1(ueList, rbMap, rbSize, response);
	} else {
		SchedUlM2mV2(ueList, rbMap, rbSize, response);
	}
}

void M2mLioumpasMacScheduler::SchedUlM2mV1(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	if (rbSize == 0 || rbStart >= rbMap.GetSize())
		return;
	uint16_t allocatedRbSize = 0;
	uint16_t rbEnd = rbStart + rbSize;
	uint16_t rbPerUe = rbSize / ueList.size();
	if (rbPerUe < m_minM2mRb)
		rbPerUe = m_minM2mRb;
	std::map<uint16_t, uint16_t> rbDemandUe;
	std::vector<std::pair<double, std::pair<uint16_t, uint16_t> > > sinrValues;
	std::vector<uint16_t> deniedUeList;
	std::vector<uint16_t>::const_iterator itUe;
	for (itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uint16_t rnti = *itUe;
		rbDemandUe.insert(std::pair<uint16_t, uint16_t>(rnti, rbPerUe));
		for (uint16_t rbIndex = rbStart; rbIndex < rbEnd; rbIndex++) {
			double sinr = GetUeUlSinr(rnti, rbIndex);
			std::pair<uint16_t, uint16_t> ueRb = std::pair<uint16_t, uint16_t>(rnti, rbIndex);
			sinrValues.push_back(std::pair<double, std::pair<uint16_t, uint16_t> >(sinr, ueRb));
		}
	}

	std::sort(sinrValues.begin(), sinrValues.end(), compareSinr);
	std::vector<std::pair<double, std::pair<uint16_t, uint16_t> > >::iterator itSinr = sinrValues.begin();
	while (itSinr != sinrValues.end() && rbDemandUe.size() > 0) {
		uint16_t rnti = (*itSinr).second.first;
		uint16_t rbIndex = (*itSinr).second.second;
		double sinr = (*itSinr).first;
		std::map<uint16_t, uint16_t>::iterator itRbDemand = rbDemandUe.find(rnti);
		uint32_t delay = GetUeUlMaxPacketDelay(rnti);

		if (!rbMap.IsFree(rbIndex) || itRbDemand == rbDemandUe.end() || (*itRbDemand).second == 0) {
			itSinr++;
			continue;
		}
		uint16_t rbDemand = itRbDemand->second;

		double meanDelay = 0;
		int meanDelaySize = 0;
		std::map<uint16_t, uint16_t>::iterator itRbDemandUe;
		for (itRbDemandUe = rbDemandUe.begin(); itRbDemandUe != rbDemandUe.end(); itRbDemandUe++) {
			if (itRbDemandUe->first == rnti || itRbDemandUe->second == 0)
				continue;
			uint32_t ueDelay = GetUeUlMaxPacketDelay(itRbDemandUe->first);
			if (ueDelay > 0) {
				meanDelay += ueDelay;
				meanDelaySize++;
			}
		}
		if (meanDelaySize > 0 && meanDelay > 0.0 && delay > (meanDelay / (2.0 * meanDelaySize))) {
			itSinr++;
			itRbDemand->second = 0;
			rbDemandUe.erase(itRbDemand);
			deniedUeList.push_back(rnti);

			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(rnti);
			uint32_t waitTime = static_cast<uint32_t>(ceil(delay / 2.0));
			if (itGrant != m_m2mGrantTimers.end()) {
				(*itGrant).second = waitTime;
			} else {
				m_m2mGrantTimers.insert(std::pair<uint16_t, uint32_t>(rnti, waitTime));
			}

			continue;
		}

		uint16_t rbIndexStart = rbIndex;
		uint16_t rbIndexEnd = rbIndex + 1;
		rbDemand--;

		// Left expansion
		if (rbIndex > 0 && rbDemand > 0) {
			for (int rbJ = rbIndex - 1; rbJ >= rbStart && rbDemand > 0; rbJ--) {
				if (!rbMap.IsFree(rbJ))
					break;
				double sinr = GetUeUlSinr(rnti, rbJ);
				double maxSinr = sinr;
				for (itRbDemandUe = rbDemandUe.begin(); itRbDemandUe != rbDemandUe.end(); itRbDemandUe++) {
					if (itRbDemandUe->first == rnti || itRbDemandUe->second == 0)
						continue;
					uint32_t ueSinr = GetUeUlSinr(itRbDemandUe->first, rbJ);
					if (maxSinr < ueSinr) {
						maxSinr = ueSinr;
					}
				}

				if (sinr < maxSinr) {
					break;
				} else {
					rbDemand--;
					rbIndexStart = rbJ;
				}
			}
		}

		// Right expansion
		if (rbIndex < rbEnd - 1 && rbDemand > 0) {
			for (uint16_t rbJ = rbIndex + 1; rbJ < rbEnd && rbDemand > 0; rbJ++) {
				if (!rbMap.IsFree(rbJ))
					break;
				double sinr = GetUeUlSinr(rnti, rbJ);
				double maxSinr = sinr;
				for (itRbDemandUe = rbDemandUe.begin(); itRbDemandUe != rbDemandUe.end(); itRbDemandUe++) {
					if (itRbDemandUe->first == rnti || itRbDemandUe->second == 0)
						continue;
					uint32_t ueSinr = GetUeUlSinr(itRbDemandUe->first, rbJ);
					if (maxSinr < ueSinr) {
						maxSinr = ueSinr;
					}
				}

				if (sinr < maxSinr) {
					break;
				} else {
					rbDemand--;
					rbIndexEnd = rbJ + 1;
				}
			}
		}

		double spectralEfficiency = log2(1.0 + (std::pow(10, sinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
		int cqi = (spectralEfficiency > 0) ? m_amc->GetCqiFromSpectralEfficiency(spectralEfficiency) : 0;
		int mcs = (cqi != 0) ? m_amc->GetMcsFromCqi(cqi) : m_ulGrantMcs;
		UlDciListElement_s uldci;
		uldci.m_rnti = rnti;
		uldci.m_rbStart = rbIndexStart;
		uldci.m_rbLen = rbIndexEnd - rbIndexStart;
		uldci.m_mcs = mcs;
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

		rbMap.Allocate(rnti, uldci.m_rbStart, uldci.m_rbLen);
		allocatedRbSize += uldci.m_rbLen;
		itRbDemand->second = 0;
		rbDemandUe.erase(itRbDemand);
		response.m_dciList.push_back(uldci);
		UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);

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

		itSinr++;
	}

	if (allocatedRbSize < rbSize && deniedUeList.size() > 0) {
		SchedUlM2mV1Rest(deniedUeList, rbMap, rbSize - allocatedRbSize, rbPerUe, response);
	}
}

void M2mLioumpasMacScheduler::SchedUlM2mV2(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	uint16_t rbEnd = rbStart + rbSize;
	uint16_t rbPerUe = rbSize / ueList.size();
	//	if (rbPerUe < 1)
	//		rbPerUe = 1;
	if (rbPerUe < m_minM2mRb)
		rbPerUe = m_minM2mRb;
	std::vector<std::pair<uint16_t, uint32_t> > delayValues;
	std::map<uint16_t, uint16_t> rbDemandUe;
	std::map<uint16_t, std::pair<uint16_t, uint16_t> > rbRangeUe;
	std::vector<uint16_t>::const_iterator itUe;
	for (itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uint32_t delay = GetUeUlMaxPacketDelay(*itUe);
		delayValues.push_back(std::pair<uint16_t, uint32_t>(*itUe, delay));
		rbDemandUe.insert(std::pair<uint16_t, uint16_t>(*itUe, rbPerUe));
		rbRangeUe.insert(
				std::pair<uint16_t, std::pair<uint16_t, uint16_t> >(*itUe,
						std::pair<uint16_t, uint16_t>(rbEnd, rbEnd)));
	}
	std::sort(delayValues.begin(), delayValues.end(), compareDelay);

	std::vector<std::pair<uint16_t, uint32_t> >::iterator itDelay = delayValues.begin();
	while (delayValues.size() > 0) {
		uint16_t rnti = (*itDelay).first;
		if (rbDemandUe[rnti] > 0) {
			uint16_t rbChosen = rbEnd;
			double maxSinr = -DBL_MAX;
			std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
			for (uint16_t rbI = rbStart; rbI < rbEnd; rbI++) {
				if (!rbMap.IsFree(rbI)) {
					//					NS_LOG_INFO("rnti " << rnti <<  " not free rbI " << rbI);
					continue;
				}
				if ((rbRangeUe[rnti].first != rbEnd && rbRangeUe[rnti].second != rbEnd)
						&& (rbI + 1 < rbRangeUe[rnti].first || rbI - 1 > rbRangeUe[rnti].second)) {
					//					NS_LOG_INFO("rnti " << rnti <<  " rbI " << rbI << " first " << rbRangeUe[rnti].first << " last " << rbRangeUe[rnti].second);
					continue;
				}
				if (itCqi != m_ueCqi.end()) {
					double sinr = (*itCqi).second.at(rbI);
					if (sinr == NO_SINR) {
						sinr = EstimateUlSinr(rnti, rbI);
					}
					if (sinr >= maxSinr) {
						maxSinr = sinr;
						rbChosen = rbI;
					}
					//					NS_LOG_INFO("rnti " << rnti << " rbI "  << rbI <<  " sinr " << sinr << " max sinr " << maxSinr << " NO SINR " << NO_SINR);
				} else {
					//					NS_LOG_INFO("rnti " << rnti <<  " cqi not found rbI " << rbI);
					rbChosen = rbI;
					break;
				}
			}
			if (rbChosen != rbEnd) {
				if (rbChosen < rbRangeUe[rnti].first || rbRangeUe[rnti].first == rbEnd) {
					rbRangeUe[rnti].first = rbChosen;
				}
				if (rbChosen > rbRangeUe[rnti].second || rbRangeUe[rnti].second == rbEnd) {
					rbRangeUe[rnti].second = rbChosen;
				}
				rbMap.Allocate(rnti, rbChosen);
				rbDemandUe[rnti]--;
				//				NS_LOG_INFO("rnti " << rnti <<  " allocated rb " << rbChosen);
			} else {
				// there are no more rb available for the device
				//				NS_LOG_INFO("rnti " << rnti << " no more rb " << rbMap.GetAvailableRbSize());
				itDelay = delayValues.erase(itDelay);
			}
		} else {
			itDelay = delayValues.erase(itDelay);
		}
	}

	for (itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		uint16_t rnti = *itUe;
		std::vector<uint16_t> rbg = rbMap.GetIndexes(rnti);
		if (rbg.size() == 0)
			continue;
		std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
		double spectralEfficiency = 0;
		if (itCqi != m_ueCqi.end()) {
			double minSinr = DBL_MAX;
			for (uint16_t j = rbg[0]; j < rbg[rbg.size() - 1]; j++) {
				double sinr = (*itCqi).second.at(j);
				if (sinr < minSinr) {
					minSinr = sinr;
				}
			}
			spectralEfficiency = log2(1.0 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
		}
		int cqi = (spectralEfficiency > 0) ? m_amc->GetCqiFromSpectralEfficiency(spectralEfficiency) : 0;
		int mcs = (cqi != 0) ? m_amc->GetMcsFromCqi(cqi) : m_ulGrantMcs;
		UlDciListElement_s uldci;
		uldci.m_rnti = rnti;
		uldci.m_rbStart = rbg[0];
		uldci.m_rbLen = rbg.size();
		uldci.m_mcs = mcs;
		uldci.m_tbSize = m_amc->GetTbSizeFromMcs(uldci.m_mcs, rbg.size()) / 8;
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

		response.m_dciList.push_back(uldci);
		UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);

		NS_LOG_INFO("allocated rnti " << uldci.m_rnti << " mcs " << (int)uldci.m_mcs << " tb " << uldci.m_tbSize << " rbg " << rbg.size());

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

void M2mLioumpasMacScheduler::SchedUlM2mV1Rest(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, const uint16_t maxRbPerUe,
		struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint32_t rbAllocatedSize = 0;
	std::vector<uint16_t>::const_iterator itUe;
	for (itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		if (rbMap.GetAvailableRbSize() == 0)
			break;

		uint16_t rnti = *itUe;
		std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(rnti);
		uint32_t bsrSizeByte = (itBsr == m_ceBsrRxed.end()) ? 0 : itBsr->second;
		if (bsrSizeByte == 0)
			continue;

		uint16_t rbStart = rbMap.GetFirstAvailableRb();
		std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
		uint16_t foundRbSize = 0;
		int foundMcs = 0;
		bool found = false;

		uint16_t size = 1;
		for (uint16_t i = 2; i <= maxRbPerUe; i++) {
			if (i + rbAllocatedSize <= rbSize && rbMap.IsFree(rbStart + i - 1)) {
				size = i;
			}
		}

		if (size > 0) {
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

			UlDciListElement_s uldci;
			uldci.m_rnti = rnti;
			uldci.m_rbStart = rbStart;
			uldci.m_rbLen = size;
			uldci.m_mcs = (cqi != 0) ? m_amc->GetMcsFromCqi(cqi) : m_ulGrantMcs;
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

			rbAllocatedSize += uldci.m_rbLen;
			rbMap.Allocate(uldci.m_rnti, uldci.m_rbStart, uldci.m_rbLen);
			UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);
			response.m_dciList.push_back(uldci);

			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(rnti);
			if (itGrant != m_m2mGrantTimers.end()) {
				itGrant->second = 0;
			}

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

void M2mLioumpasMacScheduler::UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList,
		const M2mRbAllocationMap &rbMap, const std::map<uint16_t, uint32_t> &delayMap) {

}

}

