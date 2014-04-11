/*
 * m2m-mac-scheduler-2.cc
 *
 *  Created on: 10/04/2014
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <cfloat>
#include <ns3/m2m-mac-scheduler-2.h>
#include <ns3/m2m-mac-scheduler.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/integer.h>

NS_LOG_COMPONENT_DEFINE("M2mMacScheduler2");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mMacScheduler2);

M2mMacScheduler2::M2mMacScheduler2() {
	m_congestion = 0.0;
}

M2mMacScheduler2::~M2mMacScheduler2() {

}

void M2mMacScheduler2::DoDispose(void) {
	M2mMacScheduler::DoDispose();
}

TypeId M2mMacScheduler2::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mMacScheduler2").SetParent<M2mMacScheduler>().AddConstructor<
			M2mMacScheduler2>().AddAttribute("CongestionLow", "", DoubleValue(0.2),
			MakeDoubleAccessor(&M2mMacScheduler2::m_congestionLow), MakeDoubleChecker<double>()).AddAttribute(
			"CongestionHigh", "", DoubleValue(0.6), MakeDoubleAccessor(&M2mMacScheduler2::m_congestionHigh),
			MakeDoubleChecker<double>()).AddAttribute("CongestionTimeWindow", "", DoubleValue(2.0),
			MakeDoubleAccessor(&M2mMacScheduler2::m_congestionTimeWindow), MakeDoubleChecker<double>()).AddAttribute(
			"MinPercentRBForM2MLow", "", DoubleValue(0.36), MakeDoubleAccessor(&M2mMacScheduler2::m_minPercentM2mRbLow),
			MakeDoubleChecker<double>()).AddAttribute("MinPercentRBForM2MNormal", "", DoubleValue(0.48),
			MakeDoubleAccessor(&M2mMacScheduler2::m_minPercentM2mRbNormal), MakeDoubleChecker<double>()).AddAttribute(
			"MinPercentRBForM2MHigh", "", DoubleValue(0.72),
			MakeDoubleAccessor(&M2mMacScheduler2::m_minPercentM2mRbHigh), MakeDoubleChecker<double>());
	return tid;
}

void M2mMacScheduler2::DoSchedUlTriggerReq(const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params) {
	std::vector<uint16_t> harqList, m2mList;

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
		if (itQci != m_ueUlQci.end() && (*itQci).second == EpsBearer::NGBR_M2M_TRIGGER_REPORT) {
			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(rnti);
			if (itGrant != m_m2mGrantTimers.end() && (*itGrant).second > 0) {
				continue;
			}
			m2mList.push_back(rnti);
		}
	}

	M2mMacScheduler::DoSchedUlTriggerReq(params);

	std::map<uint16_t,M2mRbAllocationMap>::const_iterator itMap = m_ulAllocationMaps.find(params.m_sfnSf);
	std::vector<uint16_t>::const_iterator itM2m;
	int countReceivedRb = 0;
	for (itM2m = m2mList.begin(); itM2m != m2mList.end(); itM2m++) {
		if (itMap != m_ulAllocationMaps.end() && itMap->second.HasResources(*itM2m))
			countReceivedRb++;
	}

	double congestionLevel = 0;
	if (m2mList.size() > 0)
		congestionLevel = 1.0 - (countReceivedRb / static_cast<double>(m2mList.size()));

	m_congestion = (1.0 / m_congestionTimeWindow) * congestionLevel
			+ (1.0 - (1.0 / m_congestionTimeWindow)) * m_congestion;
	if (m_congestion > m_congestionHigh) {
		m_minPercentM2mRb = m_minPercentM2mRbHigh;
	} else if (m_congestion < m_congestionLow) {
		m_minPercentM2mRb = m_minPercentM2mRbLow;
	} else {
		m_minPercentM2mRb = m_minPercentM2mRbNormal;
	}

}

}

