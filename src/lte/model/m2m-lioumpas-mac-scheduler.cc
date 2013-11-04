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
#include <algorithm>
//#include "m2m-lioumpas-mac-scheduler.h"

NS_LOG_COMPONENT_DEFINE("M2mLioumpasMacScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mLioumpasMacScheduler);

static bool compareDelay(const std::pair<uint16_t, uint32_t> &i, const std::pair<uint16_t, uint32_t> &j) {
	return (i.second < j.second);
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
			M2mLioumpasMacScheduler>();
	return tid;
}

void M2mLioumpasMacScheduler::SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	uint16_t rbEnd = rbStart + rbSize;
	uint16_t rbPerUe = rbSize / ueList.size();
	if (rbPerUe < 1)
		rbPerUe = 1;
//	if (rbPerUe < m_minM2mRb)
//		rbPerUe = m_minM2mRb;
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
			double maxSinr = DBL_MIN;
			std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
			for (uint16_t rbI = rbStart; rbI < rbEnd; rbI++) {
				if (!rbMap.IsFree(rbI))
					continue;
				if ((rbRangeUe[rnti].first != rbEnd && rbRangeUe[rnti].second != rbEnd)
						&& (rbI + 1 < rbRangeUe[rnti].first || rbI - 1 > rbRangeUe[rnti].second)) {
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
				} else {
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
			} else {
				// there are no more rb available
				break;
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
			spectralEfficiency = log2(
					1.0 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
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
	}
}

void M2mLioumpasMacScheduler::UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList,
		const M2mRbAllocationMap &rbMap, const std::map<uint16_t, uint32_t> &delayMap) {

}

}

