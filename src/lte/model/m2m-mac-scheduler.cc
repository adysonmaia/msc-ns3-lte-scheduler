/*
 * m2m-mac-scheduler.cpp
 *
 *  Created on: Oct 8, 2013
 *      Author: adyson
 */

#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>
#include <ns3/simulator.h>
#include <ns3/lte-amc.h>
//#include "m2m-mac-scheduler.h"
#include <ns3/m2m-mac-scheduler.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/integer.h>
#include <cfloat>
#include <set>

#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

NS_LOG_COMPONENT_DEFINE("M2mMacScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(M2mMacScheduler);

int M2mType0AllocationRbg[4] = { 10, // RGB size 1
		26, // RGB size 2
		63, // RGB size 3
		110 // RGB size 4
		};// see table 7.1.6.1-1 of 36.213

// --------------------------------------------------------------------------

M2mRbAllocationMap::M2mRbAllocationMap(const uint16_t size) {
	struct m2mRbMapValue_t mapValue;
	mapValue.allocated = false;
	mapValue.rnti = 0;
	m_map.resize(size, mapValue);
}

M2mRbAllocationMap::~M2mRbAllocationMap() {
	m_map.clear();
}

bool M2mRbAllocationMap::IsFree(const uint16_t indexStart, const uint16_t size) const {
	bool free = true;
	for (uint16_t j = indexStart; j < indexStart + size; j++) {
		if (m_map.at(j).allocated) {
			free = false;
			break;
		}
	}
	return free;
}

void M2mRbAllocationMap::Allocate(const uint16_t rnti, const uint16_t indexStart, const uint16_t size) {
	struct m2mRbMapValue_t mapValue = { true, rnti };
	for (uint16_t j = indexStart; j < indexStart + size; j++) {
		m_map.at(j) = mapValue;
	}
}

std::vector<uint16_t> M2mRbAllocationMap::GetIndexes(const uint16_t rnti) const {
	std::vector<uint16_t> result;
	for (uint16_t i = 0; i < m_map.size(); i++) {
		struct m2mRbMapValue_t mapValue = m_map.at(i);
		if (mapValue.allocated && mapValue.rnti == rnti) {
			result.push_back(i);
		}
	}
	return result;
}

bool M2mRbAllocationMap::HasResources(const uint16_t rnti) const {
	std::vector<m2mRbMapValue_t>::const_iterator it = m_map.begin();
	bool response = false;
	while (it != m_map.end()) {
		if ((*it).allocated && (*it).rnti == rnti) {
			response = true;
			break;
		}
		it++;
	}
	return response;
}

uint16_t M2mRbAllocationMap::GetRnti(const uint16_t index) const {
	struct m2mRbMapValue_t mapValue = m_map.at(index);
	return mapValue.rnti;
}

uint16_t M2mRbAllocationMap::GetSize() const {
	return m_map.size();
}

uint16_t M2mRbAllocationMap::GetAvailableRbSize() const {
	std::vector<m2mRbMapValue_t>::const_iterator it = m_map.begin();
	uint16_t response = 0;
	while (it != m_map.end()) {
		if (!(*it).allocated) {
			response++;
		}
		it++;
	}
	return response;
}

uint16_t M2mRbAllocationMap::GetFirstAvailableRb() const {
	for (uint16_t i = 0; i < m_map.size(); i++) {
		struct m2mRbMapValue_t mapValue = m_map.at(i);
		if (!mapValue.allocated) {
			return i;
		}
	}
	return m_map.size();
}

void M2mRbAllocationMap::Clear() {
	std::vector<m2mRbMapValue_t>::iterator it = m_map.begin();
	while (it != m_map.end()) {
		(*it).allocated = false;
		(*it).rnti = 0;
		it++;
	}
}

// --------------------------------------------------------------------------

M2mSchedulerMemberCschedSapProvider::M2mSchedulerMemberCschedSapProvider() :
		m_scheduler(NULL) {
}

M2mSchedulerMemberCschedSapProvider::M2mSchedulerMemberCschedSapProvider(M2mMacScheduler* scheduler) :
		m_scheduler(scheduler) {
}

void M2mSchedulerMemberCschedSapProvider::CschedCellConfigReq(
		const struct CschedCellConfigReqParameters& params) {
	m_scheduler->DoCschedCellConfigReq(params);
}

void M2mSchedulerMemberCschedSapProvider::CschedUeConfigReq(
		const struct CschedUeConfigReqParameters& params) {
	m_scheduler->DoCschedUeConfigReq(params);
}

void M2mSchedulerMemberCschedSapProvider::CschedLcConfigReq(
		const struct CschedLcConfigReqParameters& params) {
	m_scheduler->DoCschedLcConfigReq(params);
}

void M2mSchedulerMemberCschedSapProvider::CschedLcReleaseReq(
		const struct CschedLcReleaseReqParameters& params) {
	m_scheduler->DoCschedLcReleaseReq(params);
}

void M2mSchedulerMemberCschedSapProvider::CschedUeReleaseReq(
		const struct CschedUeReleaseReqParameters& params) {
	m_scheduler->DoCschedUeReleaseReq(params);
}

// ----------------------------

M2mSchedulerMemberSchedSapProvider::M2mSchedulerMemberSchedSapProvider() :
		m_scheduler(NULL) {
}

M2mSchedulerMemberSchedSapProvider::M2mSchedulerMemberSchedSapProvider(M2mMacScheduler* scheduler) :
		m_scheduler(scheduler) {
}

void M2mSchedulerMemberSchedSapProvider::SchedDlRlcBufferReq(
		const struct SchedDlRlcBufferReqParameters& params) {
	m_scheduler->DoSchedDlRlcBufferReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedDlPagingBufferReq(
		const struct SchedDlPagingBufferReqParameters& params) {
	m_scheduler->DoSchedDlPagingBufferReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedDlMacBufferReq(
		const struct SchedDlMacBufferReqParameters& params) {
	m_scheduler->DoSchedDlMacBufferReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedDlTriggerReq(const struct SchedDlTriggerReqParameters& params) {
	m_scheduler->DoSchedDlTriggerReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedDlRachInfoReq(
		const struct SchedDlRachInfoReqParameters& params) {
	m_scheduler->DoSchedDlRachInfoReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedDlCqiInfoReq(const struct SchedDlCqiInfoReqParameters& params) {
	m_scheduler->DoSchedDlCqiInfoReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedUlTriggerReq(const struct SchedUlTriggerReqParameters& params) {
	m_scheduler->DoSchedUlTriggerReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedUlNoiseInterferenceReq(
		const struct SchedUlNoiseInterferenceReqParameters& params) {
	m_scheduler->DoSchedUlNoiseInterferenceReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedUlSrInfoReq(const struct SchedUlSrInfoReqParameters& params) {
	m_scheduler->DoSchedUlSrInfoReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedUlMacCtrlInfoReq(
		const struct SchedUlMacCtrlInfoReqParameters& params) {
	m_scheduler->DoSchedUlMacCtrlInfoReq(params);
}

void M2mSchedulerMemberSchedSapProvider::SchedUlCqiInfoReq(const struct SchedUlCqiInfoReqParameters& params) {
	m_scheduler->DoSchedUlCqiInfoReq(params);
}

// -------------------------------------

M2mMacScheduler::M2mMacScheduler() :
		m_cschedSapUser(0), m_schedSapUser(0) {
	m_amc = CreateObject<LteAmc>();
	m_uniformRandom = CreateObject<UniformRandomVariable>();
	m_cschedSapProvider = new M2mSchedulerMemberCschedSapProvider(this);
	m_schedSapProvider = new M2mSchedulerMemberSchedSapProvider(this);
}

M2mMacScheduler::~M2mMacScheduler() {
	NS_LOG_FUNCTION(this);
}

void M2mMacScheduler::DoDispose() {
	NS_LOG_FUNCTION (this);
	m_dlHarqProcessesDciBuffer.clear();
	m_dlHarqProcessesTimer.clear();
	m_dlHarqProcessesRlcPduListBuffer.clear();
	m_dlInfoListBuffered.clear();
	m_ulHarqCurrentProcessId.clear();
	m_ulHarqProcessesStatus.clear();
	m_ulHarqProcessesDciBuffer.clear();
	delete m_cschedSapProvider;
	delete m_schedSapProvider;
}

TypeId M2mMacScheduler::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::M2mMacScheduler").SetParent<FfMacScheduler>().AddConstructor<M2mMacScheduler>().AddAttribute(
					"CqiTimerThreshold", "The number of TTIs a CQI is valid (default 1000 - 1 sec.)",
					UintegerValue(1000), MakeUintegerAccessor(&M2mMacScheduler::m_cqiTimersThreshold),
					MakeUintegerChecker<uint32_t>()).AddAttribute("HarqEnabled",
					"Activate/Deactivate the HARQ [by default is active].", BooleanValue(true),
					MakeBooleanAccessor(&M2mMacScheduler::m_harqOn), MakeBooleanChecker()).AddAttribute(
					"UlGrantMcs", "The MCS of the UL grant, must be [0..15] (default 0)", UintegerValue(0),
					MakeUintegerAccessor(&M2mMacScheduler::m_ulGrantMcs), MakeUintegerChecker<uint8_t>()).AddAttribute(
					"H2HTimeWindow", "(default 99.0)", DoubleValue(99.0),
					MakeDoubleAccessor(&M2mMacScheduler::m_h2hTimeWindow), MakeDoubleChecker<double>()).AddAttribute(
					"M2MTimeWindow", "(default 99.0)", DoubleValue(99.0),
					MakeDoubleAccessor(&M2mMacScheduler::m_m2mTimeWindow), MakeDoubleChecker<double>()).AddAttribute(
					"MinRBPerH2H", "(default 3)", UintegerValue(3),
					MakeUintegerAccessor(&M2mMacScheduler::m_minH2hRb), MakeUintegerChecker<uint16_t>()).AddAttribute(
					"MinRBPerM2M", "(default 3)", UintegerValue(3),
					MakeUintegerAccessor(&M2mMacScheduler::m_minM2mRb), MakeUintegerChecker<uint16_t>()).AddAttribute(
					"MinPercentRBForM2M", "(default 0.1)", DoubleValue(0.1),
					MakeDoubleAccessor(&M2mMacScheduler::m_minPercentM2mRb), MakeDoubleChecker<double>());
	return tid;
}

void M2mMacScheduler::SetFfMacCschedSapUser(FfMacCschedSapUser* s) {
	m_cschedSapUser = s;
}

void M2mMacScheduler::SetFfMacSchedSapUser(FfMacSchedSapUser* s) {
	m_schedSapUser = s;
}

FfMacCschedSapProvider*
M2mMacScheduler::GetFfMacCschedSapProvider() {
	return m_cschedSapProvider;
}

FfMacSchedSapProvider*
M2mMacScheduler::GetFfMacSchedSapProvider() {
	return m_schedSapProvider;
}

// --------------------------------------------
// Implementation of the CSCHED API primitives
// --------------------------------------------

void M2mMacScheduler::DoCschedCellConfigReq(
		const struct FfMacCschedSapProvider::CschedCellConfigReqParameters& params) {
//	NS_LOG_FUNCTION(this);
	// Read the subset of parameters used
	m_cschedCellConfig = params;
	m_rachAllocationMap.resize(m_cschedCellConfig.m_ulBandwidth, 0);
	FfMacCschedSapUser::CschedUeConfigCnfParameters cnf;
	cnf.m_result = SUCCESS;
	m_cschedSapUser->CschedUeConfigCnf(cnf);
}

void M2mMacScheduler::DoCschedUeConfigReq(
		const struct FfMacCschedSapProvider::CschedUeConfigReqParameters& params) {
//	NS_LOG_FUNCTION(
//			this << " RNTI " << params.m_rnti << " txMode "
//			<< (uint16_t) params.m_transmissionMode);
	std::map<uint16_t, uint8_t>::iterator it = m_uesTxMode.find(params.m_rnti);
	if (it == m_uesTxMode.end()) {
		m_uesTxMode.insert(std::pair<uint16_t, double>(params.m_rnti, params.m_transmissionMode));
		// generate HARQ buffers
		m_dlHarqCurrentProcessId.insert(std::pair<uint16_t, uint8_t>(params.m_rnti, 0));
		DlHarqProcessesStatus_t dlHarqPrcStatus;
		dlHarqPrcStatus.resize(8, 0);
		m_dlHarqProcessesStatus.insert(
				std::pair<uint16_t, DlHarqProcessesStatus_t>(params.m_rnti, dlHarqPrcStatus));
		DlHarqProcessesTimer_t dlHarqProcessesTimer;
		dlHarqProcessesTimer.resize(8, 0);
		m_dlHarqProcessesTimer.insert(
				std::pair<uint16_t, DlHarqProcessesTimer_t>(params.m_rnti, dlHarqProcessesTimer));
		DlHarqProcessesDciBuffer_t dlHarqdci;
		dlHarqdci.resize(8);
		m_dlHarqProcessesDciBuffer.insert(
				std::pair<uint16_t, DlHarqProcessesDciBuffer_t>(params.m_rnti, dlHarqdci));
		DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
		dlHarqRlcPdu.resize(2);
		dlHarqRlcPdu.at(0).resize(8);
		dlHarqRlcPdu.at(1).resize(8);
		m_dlHarqProcessesRlcPduListBuffer.insert(
				std::pair<uint16_t, DlHarqRlcPduListBuffer_t>(params.m_rnti, dlHarqRlcPdu));
		m_ulHarqCurrentProcessId.insert(std::pair<uint16_t, uint8_t>(params.m_rnti, 0));
		UlHarqProcessesStatus_t ulHarqPrcStatus;
		ulHarqPrcStatus.resize(8, 0);
		m_ulHarqProcessesStatus.insert(
				std::pair<uint16_t, UlHarqProcessesStatus_t>(params.m_rnti, ulHarqPrcStatus));
		UlHarqProcessesDciBuffer_t ulHarqdci;
		ulHarqdci.resize(8);
		m_ulHarqProcessesDciBuffer.insert(
				std::pair<uint16_t, UlHarqProcessesDciBuffer_t>(params.m_rnti, ulHarqdci));
	} else {
		(*it).second = params.m_transmissionMode;
	}

	std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(params.m_rnti);
	if (itGrant == m_m2mGrantTimers.end()) {
		m_m2mGrantTimers.insert(std::pair<uint16_t, uint32_t>(params.m_rnti, 0));
	}
}

void M2mMacScheduler::DoCschedUeReleaseReq(
		const struct FfMacCschedSapProvider::CschedUeReleaseReqParameters& params) {
//	NS_LOG_FUNCTION(this);

	m_uesTxMode.erase(params.m_rnti);
	m_dlHarqCurrentProcessId.erase(params.m_rnti);
	m_dlHarqProcessesStatus.erase(params.m_rnti);
	m_dlHarqProcessesTimer.erase(params.m_rnti);
	m_dlHarqProcessesDciBuffer.erase(params.m_rnti);
	m_dlHarqProcessesRlcPduListBuffer.erase(params.m_rnti);
	m_ulHarqCurrentProcessId.erase(params.m_rnti);
	m_ulHarqProcessesStatus.erase(params.m_rnti);
	m_ulHarqProcessesDciBuffer.erase(params.m_rnti);
	m_flowStatsDl.erase(params.m_rnti);
	m_flowStatsUl.erase(params.m_rnti);
	m_ceBsrRxed.erase(params.m_rnti);
	m_ueUlQci.erase(params.m_rnti);
	m_m2mGrantTimers.erase(params.m_rnti);
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it =
			m_rlcBufferReq.begin();
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
	while (it != m_rlcBufferReq.end()) {
		if ((*it).first.m_rnti == params.m_rnti) {
			temp = it;
			it++;
			m_rlcBufferReq.erase(temp);
		} else {
			it++;
		}
	}
}

void M2mMacScheduler::DoCschedLcConfigReq(
		const struct FfMacCschedSapProvider::CschedLcConfigReqParameters& params) {
//	NS_LOG_FUNCTION(this << " New LC, rnti: " << params.m_rnti);

	EpsBearer::Qci qciUl = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
	uint8_t qciUlPriority = UINT8_MAX;

	for (uint16_t i = 0; i < params.m_logicalChannelConfigList.size(); i++) {
		LogicalChannelConfigListElement_s lc = params.m_logicalChannelConfigList.at(i);
		if (lc.m_direction == LogicalChannelConfigListElement_s::DIR_BOTH
				|| lc.m_direction == LogicalChannelConfigListElement_s::DIR_UL) {
			EpsBearer::Qci lcQci = static_cast<EpsBearer::Qci>(lc.m_qci);
			uint8_t lcQciPriority = EpsBearer(lcQci).GetPriority();
			if (lcQciPriority < qciUlPriority) {
				qciUl = lcQci;
				qciUlPriority = lcQciPriority;
			}
		}
	}

	std::map<uint16_t, EpsBearer::Qci>::iterator itQci = m_ueUlQci.find(params.m_rnti);
	if (itQci == m_ueUlQci.end()) {
		m_ueUlQci.insert(std::pair<uint16_t, EpsBearer::Qci>(params.m_rnti, qciUl));
	} else {
		(*itQci).second = qciUl;
	}

	std::map<uint16_t, m2mFlowPerf_t>::iterator it = m_flowStatsDl.find(params.m_rnti);
	if (it == m_flowStatsDl.end()) {
		m2mFlowPerf_t flowStatsDl;
		flowStatsDl.flowStart = Simulator::Now();
		flowStatsDl.totalBytesTransmitted = 0;
		flowStatsDl.lastTtiBytesTrasmitted = 0;
		flowStatsDl.lastAveragedThroughput = 0;
		flowStatsDl.lastAverageResourcesAllocated = 0;
		flowStatsDl.lastAveragedBsrReceived = 0;
		flowStatsDl.lastTtiResourcesAllocated = 0;
		flowStatsDl.lastTtiBsrReceived = 0;
		m_flowStatsDl.insert(std::pair<uint16_t, m2mFlowPerf_t>(params.m_rnti, flowStatsDl));
	} else {
		(*it).second.isM2m = qciUl > EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
	}

	it = m_flowStatsUl.find(params.m_rnti);
	if (it == m_flowStatsUl.end()) {
		m2mFlowPerf_t flowStatsUl;
		flowStatsUl.flowStart = Simulator::Now();
		flowStatsUl.totalBytesTransmitted = 0;
		flowStatsUl.lastTtiBytesTrasmitted = 0;
		flowStatsUl.lastAveragedThroughput = 0;
		flowStatsUl.lastAverageResourcesAllocated = 0;
		flowStatsUl.lastAveragedBsrReceived = 0;
		flowStatsUl.lastTtiResourcesAllocated = 0;
		flowStatsUl.lastTtiBsrReceived = 0;
		m_flowStatsUl.insert(std::pair<uint16_t, m2mFlowPerf_t>(params.m_rnti, flowStatsUl));
	} else {
		(*it).second.isM2m = qciUl > EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
	}
}

void M2mMacScheduler::DoCschedLcReleaseReq(
		const struct FfMacCschedSapProvider::CschedLcReleaseReqParameters& params) {
//	NS_LOG_FUNCTION(this);
	for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size(); i++) {
		std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it =
				m_rlcBufferReq.begin();
		std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
		while (it != m_rlcBufferReq.end()) {
			if (((*it).first.m_rnti == params.m_rnti)
					&& ((*it).first.m_lcId == params.m_logicalChannelIdentity.at(i))) {
				temp = it;
				it++;
				m_rlcBufferReq.erase(it);
			} else {
				it++;
			}
		}
	}
}

// --------------------------------------------
// Implementation of the SCHED API primitives
// --------------------------------------------

void M2mMacScheduler::DoSchedDlRlcBufferReq(
		const struct FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params) {
//	NS_LOG_FUNCTION(
//			this << params.m_rnti
//			<< (uint32_t) params.m_logicalChannelIdentity);
	// API generated by RLC for updating RLC parameters on a LC (tx and retx queues)
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
	LteFlowId_t flow(params.m_rnti, params.m_logicalChannelIdentity);
	it = m_rlcBufferReq.find(flow);
	if (it == m_rlcBufferReq.end()) {
		m_rlcBufferReq.insert(
				std::pair<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>(flow, params));
	} else {
		(*it).second = params;
	}
	return;
}

void M2mMacScheduler::DoSchedDlPagingBufferReq(
		const struct FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params) {
//	NS_LOG_FUNCTION(this);
	NS_FATAL_ERROR("method not implemented");
	return;
}

void M2mMacScheduler::DoSchedDlMacBufferReq(
		const struct FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params) {
//	NS_LOG_FUNCTION(this);
	NS_FATAL_ERROR("method not implemented");
	return;
}

void M2mMacScheduler::DoSchedDlRachInfoReq(
		const struct FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params) {
//	NS_LOG_FUNCTION(this);
	m_rachList = params.m_rachList;
	return;
}

void M2mMacScheduler::DoSchedDlCqiInfoReq(
		const struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params) {
//	NS_LOG_FUNCTION (this);
	for (unsigned int i = 0; i < params.m_cqiList.size(); i++) {
		if (params.m_cqiList.at(i).m_cqiType == CqiListElement_s::P10) {
			// wideband CQI reporting
			std::map<uint16_t, uint8_t>::iterator it;
			uint16_t rnti = params.m_cqiList.at(i).m_rnti;
			it = m_p10CqiRxed.find(rnti);
			if (it == m_p10CqiRxed.end()) {
				// create the new entry
				m_p10CqiRxed.insert(std::pair<uint16_t, uint8_t>(rnti, params.m_cqiList.at(i).m_wbCqi.at(0))); // only codeword 0 at this stage (SISO)
				// generate correspondent timer
				m_p10CqiTimers.insert(std::pair<uint16_t, uint32_t>(rnti, m_cqiTimersThreshold));
			} else {
				// update the CQI value and refresh correspondent timer
				(*it).second = params.m_cqiList.at(i).m_wbCqi.at(0);
				// update correspondent timer
				std::map<uint16_t, uint32_t>::iterator itTimers;
				itTimers = m_p10CqiTimers.find(rnti);
				(*itTimers).second = m_cqiTimersThreshold;
			}
		} else if (params.m_cqiList.at(i).m_cqiType == CqiListElement_s::A30) {
			// subband CQI reporting high layer configured
			std::map<uint16_t, SbMeasResult_s>::iterator it;
			uint16_t rnti = params.m_cqiList.at(i).m_rnti;
			it = m_a30CqiRxed.find(rnti);
			if (it == m_a30CqiRxed.end()) {
				// create the new entry
				m_a30CqiRxed.insert(
						std::pair<uint16_t, SbMeasResult_s>(rnti, params.m_cqiList.at(i).m_sbMeasResult));
				m_a30CqiTimers.insert(std::pair<uint16_t, uint32_t>(rnti, m_cqiTimersThreshold));
			} else {
				// update the CQI value and refresh correspondent timer
				(*it).second = params.m_cqiList.at(i).m_sbMeasResult;
				std::map<uint16_t, uint32_t>::iterator itTimers;
				itTimers = m_a30CqiTimers.find(rnti);
				(*itTimers).second = m_cqiTimersThreshold;
			}
		} else {
			NS_LOG_ERROR (this << " CQI type unknown");
		}
	}
	return;
}

void M2mMacScheduler::DoSchedDlTriggerReq(
		const struct FfMacSchedSapProvider::SchedDlTriggerReqParameters& params) {
//	NS_LOG_FUNCTION(
//			this << " Dl Frame no. " << (params.m_sfnSf >> 4) << " subframe no. "
//			<< (0xF & params.m_sfnSf));
	// API generated by RLC for triggering the scheduling of a DL subframe

	// evaluate the relative channel quality indicator for each UE per each RBG
	// (since we are using allocation type 0 the small unit of allocation is RBG)
	// Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

	RefreshDlCqiMaps();

	int rbgSize = GetRbgSize(m_cschedCellConfig.m_dlBandwidth);
	int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
	std::map<uint16_t, std::vector<uint16_t> > allocationMap; // RBs map per RNTI
	std::vector<bool> rbgMap; // global RBGs map
	uint16_t rbgAllocatedNum = 0;
	std::set<uint16_t> rntiAllocated;
	rbgMap.resize(rbgNum, false);
	FfMacSchedSapUser::SchedDlConfigIndParameters ret;

	// RACH Allocation
	m_rachAllocationMap.resize(m_cschedCellConfig.m_ulBandwidth, 0);
	uint16_t rbStart = 0;
	std::vector<struct RachListElement_s>::iterator itRach;
	for (itRach = m_rachList.begin(); itRach != m_rachList.end(); itRach++) {
		NS_ASSERT_MSG(
				m_amc->GetTbSizeFromMcs(m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth)
						> (*itRach).m_estimatedSize,
				" Default UL Grant MCS does not allow to send RACH messages");
		BuildRarListElement_s newRar;
		newRar.m_rnti = (*itRach).m_rnti;
		// DL-RACH Allocation
		// Ideal: no needs of configuring m_dci
		// UL-RACH Allocation
		newRar.m_grant.m_rnti = newRar.m_rnti;
		newRar.m_grant.m_mcs = m_ulGrantMcs;
		uint16_t rbLen = 1;
		uint16_t tbSizeBits = 0;
		// find lowest TB size that fits UL grant estimated size
		while ((tbSizeBits < (*itRach).m_estimatedSize)
				&& (rbStart + rbLen < m_cschedCellConfig.m_ulBandwidth)) {
			rbLen++;
			tbSizeBits = m_amc->GetTbSizeFromMcs(m_ulGrantMcs, rbLen);
		}
		if (tbSizeBits < (*itRach).m_estimatedSize) {
			// no more allocation space: finish allocation
			break;
		}
		newRar.m_grant.m_rbStart = rbStart;
		newRar.m_grant.m_rbLen = rbLen;
		newRar.m_grant.m_tbSize = tbSizeBits / 8;
		newRar.m_grant.m_hopping = false;
		newRar.m_grant.m_tpc = 0;
		newRar.m_grant.m_cqiRequest = false;
		newRar.m_grant.m_ulDelay = false;
//		NS_LOG_INFO(
//				this << " UL grant allocated to RNTI " << (*itRach).m_rnti
//				<< " rbStart " << rbStart << " rbLen " << rbLen
//				<< " MCS " << m_ulGrantMcs << " tbSize "
//				<< newRar.m_grant.m_tbSize);
		for (uint16_t i = rbStart; i < rbStart + rbLen; i++) {
			m_rachAllocationMap.at(i) = (*itRach).m_rnti;
		}
		rbStart = rbStart + rbLen;

		ret.m_buildRarList.push_back(newRar);
	}
	m_rachList.clear();
	// -----------------------------------------------------------------

	// Process DL HARQ feedback
	RefreshHarqProcesses();
	// retrieve past HARQ retx buffered
	if (m_dlInfoListBuffered.size() > 0) {
		if (params.m_dlInfoList.size() > 0) {
//			NS_LOG_INFO(this << " Dl Received DL-HARQ feedback");
			m_dlInfoListBuffered.insert(m_dlInfoListBuffered.end(), params.m_dlInfoList.begin(),
					params.m_dlInfoList.end());
		}
	} else {
		if (params.m_dlInfoList.size() > 0) {
			m_dlInfoListBuffered = params.m_dlInfoList;
		}
	}
	if (m_harqOn == false) {
		// Ignore HARQ feedback
		m_dlInfoListBuffered.clear();
	}
	std::vector<struct DlInfoListElement_s> dlInfoListUntxed;
	for (uint16_t i = 0; i < m_dlInfoListBuffered.size(); i++) {
		std::set<uint16_t>::iterator itRnti = rntiAllocated.find(m_dlInfoListBuffered.at(i).m_rnti);
		if (itRnti != rntiAllocated.end()) {
			// RNTI already allocated for retx
			continue;
		}
		uint8_t nLayers = m_dlInfoListBuffered.at(i).m_harqStatus.size();
		std::vector<bool> retx;
//		NS_LOG_INFO(this << " Dl Processing DLHARQ feedback");
		if (nLayers == 1) {
			retx.push_back(m_dlInfoListBuffered.at(i).m_harqStatus.at(0) == DlInfoListElement_s::NACK);
			retx.push_back(false);
		} else {
			retx.push_back(m_dlInfoListBuffered.at(i).m_harqStatus.at(0) == DlInfoListElement_s::NACK);
			retx.push_back(m_dlInfoListBuffered.at(i).m_harqStatus.at(1) == DlInfoListElement_s::NACK);
		}
		if (retx.at(0) || retx.at(1)) {
			// retrieve HARQ process information
			uint16_t rnti = m_dlInfoListBuffered.at(i).m_rnti;
			uint8_t harqId = m_dlInfoListBuffered.at(i).m_harqProcessId;
//			NS_LOG_INFO(
//					this << " Dl HARQ retx RNTI " << rnti << " harqId "
//					<< (uint16_t) harqId);
			std::map<uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find(
					rnti);
			if (itHarq == m_dlHarqProcessesDciBuffer.end()) {
				NS_FATAL_ERROR(" Dl No info find in HARQ buffer for UE " << rnti);
			}

			DlDciListElement_s dci = (*itHarq).second.at(harqId);
			int rv = 0;
			if (dci.m_rv.size() == 1) {
				rv = dci.m_rv.at(0);
			} else {
				rv = (dci.m_rv.at(0) > dci.m_rv.at(1) ? dci.m_rv.at(0) : dci.m_rv.at(1));
			}

			if (rv == 3) {
				// maximum number of retx reached -> drop process
//				NS_LOG_INFO(
//						"Dl Maximum number of retransmissions reached -> drop process");
				std::map<uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find(rnti);
				if (it == m_dlHarqProcessesStatus.end()) {
					NS_LOG_ERROR(
							"Dl No info find in HARQ buffer for UE (might change eNB) "
							<< m_dlInfoListBuffered.at(i).m_rnti);
				}
				(*it).second.at(harqId) = 0;
				std::map<uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =
						m_dlHarqProcessesRlcPduListBuffer.find(rnti);
				if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end()) {
					NS_FATAL_ERROR(
							"Dl Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at(i).m_rnti);
				}
				for (uint16_t k = 0; k < (*itRlcPdu).second.size(); k++) {
					(*itRlcPdu).second.at(k).at(harqId).clear();
				}
				continue;
			}
			// check the feasibility of retransmitting on the same RBGs
			// translate the DCI to Spectrum framework
			std::vector<int> dciRbg;
			uint32_t mask = 0x1;
//			NS_LOG_INFO(
//					"Dl Original RBGs " << dci.m_rbBitmap << " rnti "
//					<< dci.m_rnti);
			for (int j = 0; j < 32; j++) {
				if (((dci.m_rbBitmap & mask) >> j) == 1) {
					dciRbg.push_back(j);
//					NS_LOG_INFO("\t" << j);
				}
				mask = (mask << 1);
			}
			bool free = true;
			for (uint8_t j = 0; j < dciRbg.size(); j++) {
				if (rbgMap.at(dciRbg.at(j)) == true) {
					free = false;
					break;
				}
			}
			if (free) {
				// use the same RBGs for the retx
				// reserve RBGs
				for (uint8_t j = 0; j < dciRbg.size(); j++) {
					rbgMap.at(dciRbg.at(j)) = true;
//					NS_LOG_INFO("Dl RBG " << dciRbg.at(j) << " assigned");
					rbgAllocatedNum++;
				}

//				NS_LOG_INFO(this << " Dl Send retx in the same RBGs");
			} else {
				// find RBGs for sending HARQ retx
				uint8_t j = 0;
				uint8_t rbgId = (dciRbg.at(dciRbg.size() - 1) + 1) % rbgNum;
				uint8_t startRbg = dciRbg.at(dciRbg.size() - 1);
				std::vector<bool> rbgMapCopy = rbgMap;
				while ((j < dciRbg.size()) && (startRbg != rbgId)) {
					if (rbgMapCopy.at(rbgId) == false) {
						rbgMapCopy.at(rbgId) = true;
						dciRbg.at(j) = rbgId;
						j++;
					}
					rbgId++;
				}
				if (j == dciRbg.size()) {
					// find new RBGs -> update DCI map
					uint32_t rbgMask = 0;
					for (uint16_t k = 0; k < dciRbg.size(); k++) {
						rbgMask = rbgMask + (0x1 << dciRbg.at(k));
						rbgAllocatedNum++;
					}
					dci.m_rbBitmap = rbgMask;
					rbgMap = rbgMapCopy;
//					NS_LOG_INFO(this << " Dl Move retx in RBGs " << dciRbg.size());
				} else {
					// HARQ retx cannot be performed on this TTI -> store it
					dlInfoListUntxed.push_back(params.m_dlInfoList.at(i));
//					NS_LOG_INFO(
//							this << " Dl No resource for this retx -> buffer it");
				}
			}
			// retrieve RLC PDU list for retx TBsize and update DCI
			BuildDataListElement_s newEl;
			std::map<uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =
					m_dlHarqProcessesRlcPduListBuffer.find(rnti);
			if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end()) {
				NS_FATAL_ERROR("Dl Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
			}
			for (uint8_t j = 0; j < nLayers; j++) {
				if (retx.at(j)) {
					if (j >= dci.m_ndi.size()) {
						// for avoiding errors in MIMO transient phases
						dci.m_ndi.push_back(0);
						dci.m_rv.push_back(0);
						dci.m_mcs.push_back(0);
						dci.m_tbsSize.push_back(0);
//						NS_LOG_INFO(
//								this << " Dl layer " << (uint16_t) j
//								<< " no txed (MIMO transition)");
					} else {
						dci.m_ndi.at(j) = 0;
						dci.m_rv.at(j)++; //
						(
*						itHarq).second.at(harqId).m_rv.at(j)++;
//						NS_LOG_INFO(
//								this << " Dl layer " << (uint16_t) j << " RV "
//								<< (uint16_t) dci.m_rv.at(j));
					}
				} else {
					// empty TB of layer j
					dci.m_ndi.at(j) = 0;
					dci.m_rv.at(j) = 0;
					dci.m_mcs.at(j) = 0;
					dci.m_tbsSize.at(j) = 0;
//					NS_LOG_INFO(
//							this << " Dl layer " << (uint16_t) j << " no retx");
				}
			}
			for (uint16_t k = 0; k < (*itRlcPdu).second.at(0).at(dci.m_harqProcess).size(); k++) {
				std::vector<struct RlcPduListElement_s> rlcPduListPerLc;
				for (uint8_t j = 0; j < nLayers; j++) {
					if (retx.at(j)) {
						if (j < dci.m_ndi.size()) {
							rlcPduListPerLc.push_back((*itRlcPdu).second.at(j).at(dci.m_harqProcess).at(k));
						}
					}
				}

				if (rlcPduListPerLc.size() > 0) {
					newEl.m_rlcPduList.push_back(rlcPduListPerLc);
				}
			}
			newEl.m_rnti = rnti;
			newEl.m_dci = dci;
			(*itHarq).second.at(harqId).m_rv = dci.m_rv;
			// refresh timer
			std::map<uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find(
					rnti);
			if (itHarqTimer == m_dlHarqProcessesTimer.end()) {
				NS_FATAL_ERROR("Dl Unable to find HARQ timer for RNTI " << (uint16_t) rnti);
			}
			(*itHarqTimer).second.at(harqId) = 0;
			ret.m_buildDataList.push_back(newEl);
			rntiAllocated.insert(rnti);
		} else {
			// update HARQ process status
//			NS_LOG_INFO(
//					this << " Dl HARQ received ACK for UE "
//					<< m_dlInfoListBuffered.at(i).m_rnti);
			std::map<uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find(
					m_dlInfoListBuffered.at(i).m_rnti);
			if (it == m_dlHarqProcessesStatus.end()) {
				NS_FATAL_ERROR("Dl No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at(i).m_rnti);
			}
			(*it).second.at(m_dlInfoListBuffered.at(i).m_harqProcessId) = 0;
			std::map<uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =
					m_dlHarqProcessesRlcPduListBuffer.find(m_dlInfoListBuffered.at(i).m_rnti);
			if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end()) {
				NS_FATAL_ERROR(
						"Dl Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at(i).m_rnti);
			}
			for (uint16_t k = 0; k < (*itRlcPdu).second.size(); k++) {
				(*itRlcPdu).second.at(k).at(m_dlInfoListBuffered.at(i).m_harqProcessId).clear();
			}
		}
	}
	m_dlInfoListBuffered.clear();
	m_dlInfoListBuffered = dlInfoListUntxed;
	// ------------------------------------------------------------------------

	for (int i = 0; i < rbgNum; i++) {
//		NS_LOG_INFO(this << " Dl ALLOCATION for RBG " << i << " of " << rbgNum);
		if (rbgMap.at(i) == false) {
			std::map<uint16_t, m2mFlowPerf_t>::iterator it;
			std::map<uint16_t, m2mFlowPerf_t>::iterator itMax = m_flowStatsDl.end();
			double rcqiMax = 0.0;
			for (it = m_flowStatsDl.begin(); it != m_flowStatsDl.end(); it++) {
				std::set<uint16_t>::iterator itRnti = rntiAllocated.find((*it).first);
				if ((itRnti != rntiAllocated.end()) || (!HarqProcessAvailability((*it).first))) {
					// UE already allocated for HARQ or without HARQ process available -> drop it
					if (itRnti != rntiAllocated.end()) {
//						NS_LOG_DEBUG(
//								this << " Dl RNTI discared for HARQ tx"
//								<< (uint16_t)(*it).first);
					}
					if (!HarqProcessAvailability((*it).first)) {
//						NS_LOG_DEBUG(
//								this << " Dl RNTI discared for HARQ id"
//								<< (uint16_t)(*it).first);
					}
					continue;
				}
				std::map<uint16_t, SbMeasResult_s>::iterator itCqi;
				itCqi = m_a30CqiRxed.find((*it).first);
				std::map<uint16_t, uint8_t>::iterator itTxMode;
				itTxMode = m_uesTxMode.find((*it).first);
				if (itTxMode == m_uesTxMode.end()) {
					NS_FATAL_ERROR("Dl No Transmission Mode info on user " << (*it).first);
				}
				int nLayer = TransmissionModesLayers::TxMode2LayerNum((*itTxMode).second);
				std::vector<uint8_t> sbCqi;
				if (itCqi == m_a30CqiRxed.end()) {
					for (uint8_t k = 0; k < nLayer; k++) {
						sbCqi.push_back(1); // start with lowest value
					}
				} else {
					sbCqi = (*itCqi).second.m_higherLayerSelected.at(i).m_sbCqi;
				}
				uint8_t cqi1 = sbCqi.at(0);
				uint8_t cqi2 = 1;
				if (sbCqi.size() > 1) {
					cqi2 = sbCqi.at(1);
				}

				if ((cqi1 > 0) || (cqi2 > 0)) // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
						{
					if (LcActivePerFlow((*it).first) > 0) {
						// this UE has data to transmit
						double achievableRate = 0.0;
						uint8_t mcs = 0;
						for (uint8_t k = 0; k < nLayer; k++) {
							if (sbCqi.size() > k) {
								mcs = m_amc->GetMcsFromCqi(sbCqi.at(k));
							} else {
								// no info on this subband -> worst MCS
								mcs = 0;
							}
							achievableRate += ((m_amc->GetTbSizeFromMcs(mcs, rbgSize) / 8) / 0.001); // = TB size / TTI
						}

						double rcqi = achievableRate / (*it).second.lastAveragedThroughput;
//						NS_LOG_INFO(
//								this << " Dl RNTI " << (*it).first << " MCS "
//								<< (uint32_t) mcs << " achievableRate "
//								<< achievableRate << " avgThr "
//								<< (*it).second.lastAveragedThroughput
//								<< " RCQI " << rcqi);

						if (rcqi > rcqiMax) {
							rcqiMax = rcqi;
							itMax = it;
						}
					}
				} // end if cqi
			} // end for m_rlcBufferReq

			if (itMax == m_flowStatsDl.end()) {
				// no UE available for this RB
//				NS_LOG_INFO(this << " Dl any UE found");
			} else {
				rbgMap.at(i) = true;
				std::map<uint16_t, std::vector<uint16_t> >::iterator itMap;
				itMap = allocationMap.find((*itMax).first);
				if (itMap == allocationMap.end()) {
					// insert new element
					std::vector<uint16_t> tempMap;
					tempMap.push_back(i);
					allocationMap.insert(
							std::pair<uint16_t, std::vector<uint16_t> >((*itMax).first, tempMap));
				} else {
					(*itMap).second.push_back(i);
				}
//				NS_LOG_INFO(this << " Dl UE assigned " << (*itMax).first);
			}
		} // end for RBG free
	} // end for RBGs

	// reset TTI stats of users
	std::map<uint16_t, m2mFlowPerf_t>::iterator itStats;
	for (itStats = m_flowStatsDl.begin(); itStats != m_flowStatsDl.end(); itStats++) {
		(*itStats).second.lastTtiBytesTrasmitted = 0;
	}

	// generate the transmission opportunities by grouping the RBGs of the same RNTI and
	// creating the correspondent DCIs
	std::map<uint16_t, std::vector<uint16_t> >::iterator itMap = allocationMap.begin();
	while (itMap != allocationMap.end()) {
		// create new BuildDataListElement_s for this LC
		BuildDataListElement_s newEl;
		newEl.m_rnti = (*itMap).first;
		// create the DlDciListElement_s
		DlDciListElement_s newDci;
		newDci.m_rnti = (*itMap).first;
		newDci.m_harqProcess = UpdateHarqProcessId((*itMap).first);

		uint16_t lcActives = LcActivePerFlow((*itMap).first);
//		NS_LOG_INFO(
//				this << " Dl Allocate user " << newEl.m_rnti << " rbg "
//				<< lcActives);
		if (lcActives == 0) {
			// Set to max value, to avoid divide by 0 below
			lcActives = (uint16_t) 65535; // UINT16_MAX;
		}
		uint16_t RgbPerRnti = (*itMap).second.size();
		std::map<uint16_t, SbMeasResult_s>::iterator itCqi;
		itCqi = m_a30CqiRxed.find((*itMap).first);
		std::map<uint16_t, uint8_t>::iterator itTxMode;
		itTxMode = m_uesTxMode.find((*itMap).first);
		if (itTxMode == m_uesTxMode.end()) {
			NS_FATAL_ERROR("Dl No Transmission Mode info on user " << (*itMap).first);
		}
		int nLayer = TransmissionModesLayers::TxMode2LayerNum((*itTxMode).second);
		std::vector<uint8_t> worstCqi(2, 15);
		if (itCqi != m_a30CqiRxed.end()) {
			for (uint16_t k = 0; k < (*itMap).second.size(); k++) {
				if ((*itCqi).second.m_higherLayerSelected.size() > (*itMap).second.at(k)) {
//					NS_LOG_INFO(
//							this << " Dl RBG " << (*itMap).second.at(k) << " CQI "
//							<< (uint16_t)(
//									(*itCqi).second.m_higherLayerSelected.at(
//											(*itMap).second.at(k)).m_sbCqi.at(
//											0)));
					for (uint8_t j = 0; j < nLayer; j++) {
						if ((*itCqi).second.m_higherLayerSelected.at((*itMap).second.at(k)).m_sbCqi.size()
								> j) {
							if (((*itCqi).second.m_higherLayerSelected.at((*itMap).second.at(k)).m_sbCqi.at(j))
									< worstCqi.at(j)) {
								worstCqi.at(j) = ((*itCqi).second.m_higherLayerSelected.at(
										(*itMap).second.at(k)).m_sbCqi.at(j));
							}
						} else {
							// no CQI for this layer of this suband -> worst one
							worstCqi.at(j) = 1;
						}
					}
				} else {
					for (uint8_t j = 0; j < nLayer; j++) {
						worstCqi.at(j) = 1; // try with lowest MCS in RBG with no info on channel
					}
				}
			}
		} else {
			for (uint8_t j = 0; j < nLayer; j++) {
				worstCqi.at(j) = 1; // try with lowest MCS in RBG with no info on channel
			}
		}
		for (uint8_t j = 0; j < nLayer; j++) {
//			NS_LOG_INFO(
//					this << " Dl Layer " << (uint16_t) j << " CQI selected "
//					<< (uint16_t) worstCqi.at(j));
		}
		uint32_t bytesTxed = 0;
		for (uint8_t j = 0; j < nLayer; j++) {
			newDci.m_mcs.push_back(m_amc->GetMcsFromCqi(worstCqi.at(j)));
			int tbSize = (m_amc->GetTbSizeFromMcs(newDci.m_mcs.at(j), RgbPerRnti * rbgSize) / 8); // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
			newDci.m_tbsSize.push_back(tbSize);
//			NS_LOG_INFO(
//					this << " Dl Layer " << (uint16_t) j << " MCS selected"
//					<< m_amc->GetMcsFromCqi(worstCqi.at(j)));
			bytesTxed += tbSize;
		}

		newDci.m_resAlloc = 0; // only allocation type 0 at this stage
		newDci.m_rbBitmap = 0; // TBD (32 bit bitmap see 7.1.6 of 36.213)
		uint32_t rbgMask = 0;
		for (uint16_t k = 0; k < (*itMap).second.size(); k++) {
			rbgMask = rbgMask + (0x1 << (*itMap).second.at(k));
//			NS_LOG_INFO(this << " Dl Allocated RBG " << (*itMap).second.at(k));
		}
		newDci.m_rbBitmap = rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)

		// create the rlc PDUs -> equally divide resources among actives LCs
		std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itBufReq;
		for (itBufReq = m_rlcBufferReq.begin(); itBufReq != m_rlcBufferReq.end(); itBufReq++) {
			if (((*itBufReq).first.m_rnti == (*itMap).first)
					&& (((*itBufReq).second.m_rlcTransmissionQueueSize > 0)
							|| ((*itBufReq).second.m_rlcRetransmissionQueueSize > 0)
							|| ((*itBufReq).second.m_rlcStatusPduSize > 0))) {
				std::vector<struct RlcPduListElement_s> newRlcPduLe;
				for (uint8_t j = 0; j < nLayer; j++) {
					RlcPduListElement_s newRlcEl;
					newRlcEl.m_logicalChannelIdentity = (*itBufReq).first.m_lcId;
					newRlcEl.m_size = newDci.m_tbsSize.at(j) / lcActives;
//					NS_LOG_INFO(
//							this << " Dl LCID "
//							<< (uint32_t) newRlcEl.m_logicalChannelIdentity
//							<< " size " << newRlcEl.m_size << " layer "
//							<< (uint16_t) j);
					newRlcPduLe.push_back(newRlcEl);
					UpdateDlRlcBufferInfo(newDci.m_rnti, newRlcEl.m_logicalChannelIdentity, newRlcEl.m_size);
					if (m_harqOn == true) {
						// store RLC PDU list for HARQ
						std::map<uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =
								m_dlHarqProcessesRlcPduListBuffer.find((*itMap).first);
						if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end()) {
							NS_FATAL_ERROR(
									"Dl Unable to find RlcPdcList in HARQ buffer for RNTI " << (*itMap).first);
						}
						(*itRlcPdu).second.at(j).at(newDci.m_harqProcess).push_back(newRlcEl);
					}
				}
				newEl.m_rlcPduList.push_back(newRlcPduLe);
			}
			if ((*itBufReq).first.m_rnti > (*itMap).first) {
				break;
			}
		}
		for (uint8_t j = 0; j < nLayer; j++) {
			newDci.m_ndi.push_back(1);
			newDci.m_rv.push_back(0);
		}

		newEl.m_dci = newDci;

		if (m_harqOn == true) {
			// store DCI for HARQ
			std::map<uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find(
					newEl.m_rnti);
			if (itDci == m_dlHarqProcessesDciBuffer.end()) {
				NS_FATAL_ERROR("Dl Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
			}
			(*itDci).second.at(newDci.m_harqProcess) = newDci;
			// refresh timer
			std::map<uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find(
					newEl.m_rnti);
			if (itHarqTimer == m_dlHarqProcessesTimer.end()) {
				NS_FATAL_ERROR("Dl Unable to find HARQ timer for RNTI " << (uint16_t) newEl.m_rnti);
			}
			(*itHarqTimer).second.at(newDci.m_harqProcess) = 0;
		}

		// ...more parameters -> ingored in this version

		ret.m_buildDataList.push_back(newEl);
		// update UE stats
		std::map<uint16_t, m2mFlowPerf_t>::iterator it;
		it = m_flowStatsDl.find((*itMap).first);
		if (it != m_flowStatsDl.end()) {
			(*it).second.lastTtiBytesTrasmitted = bytesTxed;
//			NS_LOG_INFO(
//					this << " Dl UE total bytes txed "
//					<< (*it).second.lastTtiBytesTrasmitted);

		} else {
			NS_FATAL_ERROR(this << " Dl No Stats for this allocated UE");
		}

		itMap++;
	} // end while allocation
	ret.m_nrOfPdcchOfdmSymbols = 1; /// \todo check correct value according the DCIs txed

	// update UEs stats
//	NS_LOG_INFO(this << " Dl Update UEs statistics");
	for (itStats = m_flowStatsDl.begin(); itStats != m_flowStatsDl.end(); itStats++) {
		(*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTrasmitted;
		// update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
		(*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_h2hTimeWindow))
				* (*itStats).second.lastAveragedThroughput)
				+ ((1.0 / m_h2hTimeWindow) * (double) ((*itStats).second.lastTtiBytesTrasmitted / 0.001));
//		NS_LOG_INFO(
//				this << " DL UE " << (*itStats).first << " Total bytes "
//				<< (*itStats).second.totalBytesTransmitted << " Average throughput "
//				<< (*itStats).second.lastAveragedThroughput);
		(*itStats).second.lastTtiBytesTrasmitted = 0;
	}

	m_schedSapUser->SchedDlConfigInd(ret);

	return;
}

void M2mMacScheduler::DoSchedUlNoiseInterferenceReq(
		const struct FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params) {
	NS_LOG_FUNCTION(this);
	return;
}

void M2mMacScheduler::DoSchedUlSrInfoReq(
		const struct FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params) {
	NS_LOG_FUNCTION(this);
	return;
}

void M2mMacScheduler::DoSchedUlMacCtrlInfoReq(
		const struct FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params) {
//	NS_LOG_FUNCTION (this);
	std::map<uint16_t, uint32_t>::iterator it;
	for (unsigned int i = 0; i < params.m_macCeList.size(); i++) {
		if (params.m_macCeList.at(i).m_macCeType == MacCeListElement_s::BSR) {
			// buffer status report
			// note that this scheduler does not differentiate the
			// allocation according to which LCGs have more/less bytes
			// to send.
			// Hence the BSR of different LCGs are just summed up to get
			// a total queue size that is used for allocation purposes.

			uint32_t buffer = 0;
			for (uint8_t lcg = 0; lcg < 4; ++lcg) {
				uint8_t bsrId = params.m_macCeList.at(i).m_macCeValue.m_bufferStatus.at(lcg);
				buffer += BufferSizeLevelBsr::BsrId2BufferSize(bsrId);
			}

			uint16_t rnti = params.m_macCeList.at(i).m_rnti;
//			NS_LOG_LOGIC (this << " UL RNTI " << rnti << " Buffer " << buffer);
			it = m_ceBsrRxed.find(rnti);
			if (it == m_ceBsrRxed.end()) {
				// create the new entry
				m_ceBsrRxed.insert(std::pair<uint16_t, uint32_t>(rnti, buffer));
			} else {
				// update the buffer size value
				(*it).second = buffer;
			}
		}
	}
	return;
}

void M2mMacScheduler::DoSchedUlCqiInfoReq(
		const struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params) {
//	NS_LOG_FUNCTION (this);
// retrieve the allocation for this subframe
	switch (m_ulCqiFilter) { // FfMacScheduler protected attribute
	case FfMacScheduler::SRS_UL_CQI: {
		// filter all the CQIs that are not SRS based
		if (params.m_ulCqi.m_type != UlCqi_s::SRS) {
			return;
		}
	}
		break;
	case FfMacScheduler::PUSCH_UL_CQI: {
		// filter all the CQIs that are not SRS based
		if (params.m_ulCqi.m_type != UlCqi_s::PUSCH) {
			return;
		}
	}
	case FfMacScheduler::ALL_UL_CQI:
		break;

	default:
		NS_FATAL_ERROR("Unknown UL CQI type");
	}

	switch (params.m_ulCqi.m_type) {
	case UlCqi_s::PUSCH: {
		std::map<uint16_t, M2mRbAllocationMap>::iterator itMap;
		std::map<uint16_t, std::vector<double> >::iterator itCqi;
//		NS_LOG_DEBUG (this << " Collect PUSCH CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
		itMap = m_ulAllocationMaps.find(params.m_sfnSf);
		if (itMap == m_ulAllocationMaps.end()) {
			return;
		}
		for (uint16_t i = 0; i < (*itMap).second.GetSize(); i++) {
			// convert from fixed point notation Sxxxxxxxxxxx.xxx to double
			double sinr = LteFfConverter::fpS11dot3toDouble(params.m_ulCqi.m_sinr.at(i));
			uint16_t rnti = (*itMap).second.GetRnti(i);
			if (rnti == 0) {
				continue;
			}
			itCqi = m_ueCqi.find(rnti);
			if (itCqi == m_ueCqi.end()) {
				// create a new entry
				std::vector<double> newCqi;
				for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++) {
					if (i == j) {
						newCqi.push_back(sinr);
					} else {
						// initialize with NO_SINR value.
						newCqi.push_back(NO_SINR);
					}
				}
				m_ueCqi.insert(std::pair<uint16_t, std::vector<double> >(rnti, newCqi));
				// generate correspondent timer
				m_ueCqiTimers.insert(std::pair<uint16_t, uint32_t>(rnti, m_cqiTimersThreshold));
			} else {
				// update the value
				(*itCqi).second.at(i) = sinr;
				// update correspondent timer
				std::map<uint16_t, uint32_t>::iterator itTimers;
				itTimers = m_ueCqiTimers.find(rnti);
				(*itTimers).second = m_cqiTimersThreshold;
			}
//			NS_LOG_DEBUG (this << " UL RNTI " << rnti << " RB " << i << " SINR " << sinr);
		}
		// remove obsolete info on allocation
		m_ulAllocationMaps.erase(itMap);
	}
		break;
	case UlCqi_s::SRS: {
//		NS_LOG_DEBUG (this << " Collect SRS CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
		// get the RNTI from vendor specific parameters
		uint16_t rnti = 0;
		NS_ASSERT(params.m_vendorSpecificList.size() > 0);
		for (uint16_t i = 0; i < params.m_vendorSpecificList.size(); i++) {
			if (params.m_vendorSpecificList.at(i).m_type == SRS_CQI_RNTI_VSP) {
				Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp>(
						params.m_vendorSpecificList.at(i).m_value);
				rnti = vsp->GetRnti();
			}
		}
		std::map<uint16_t, std::vector<double> >::iterator itCqi;
		itCqi = m_ueCqi.find(rnti);
		if (itCqi == m_ueCqi.end()) {
			// create a new entry
			std::vector<double> newCqi;
			for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++) {
				double sinr = LteFfConverter::fpS11dot3toDouble(params.m_ulCqi.m_sinr.at(j));
				newCqi.push_back(sinr);
//				NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);
			}
			m_ueCqi.insert(std::pair<uint16_t, std::vector<double> >(rnti, newCqi));
			// generate correspondent timer
			m_ueCqiTimers.insert(std::pair<uint16_t, uint32_t>(rnti, m_cqiTimersThreshold));
		} else {
			// update the values
			for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++) {
				double sinr = LteFfConverter::fpS11dot3toDouble(params.m_ulCqi.m_sinr.at(j));
				(*itCqi).second.at(j) = sinr;
//				NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
			}
			// update correspondent timer
			std::map<uint16_t, uint32_t>::iterator itTimers;
			itTimers = m_ueCqiTimers.find(rnti);
			(*itTimers).second = m_cqiTimersThreshold;
		}
	}
		break;
	case UlCqi_s::PUCCH_1:
	case UlCqi_s::PUCCH_2:
	case UlCqi_s::PRACH: {
		NS_FATAL_ERROR("PfFfMacScheduler supports only PUSCH and SRS UL-CQIs");
	}
		break;
	default:
		NS_FATAL_ERROR("Unknown type of UL-CQI");
	}
	return;
}

void M2mMacScheduler::DoSchedUlTriggerReq(
		const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params) {
//	NS_LOG_FUNCTION(
//			this << " Ul Frame no. " << (params.m_sfnSf >> 4) << " subframe no. "
//			<< (0xF & params.m_sfnSf));

	RefreshUlCqiMaps();
	RefreshM2MAccessGrantTimers();

	struct FfMacSchedSapUser::SchedUlConfigIndParameters response;
	M2mRbAllocationMap rbMap(m_cschedCellConfig.m_ulBandwidth);
	std::vector<uint16_t> harqList, h2hList, m2mList;

	uint16_t h2hRbDemand = 0;
	double m2mMaxLastAveragedThroughput = 0;
	uint16_t m2mMaxDelay = 0;

	if (m_harqOn && params.m_ulInfoList.size() > 0) {
		for (std::vector<UlInfoListElement_s>::const_iterator itUlInfo = params.m_ulInfoList.begin();
				itUlInfo != params.m_ulInfoList.end(); itUlInfo++) {
			if ((*itUlInfo).m_receptionStatus == UlInfoListElement_s::NotOk && m_harqOn) {
				harqList.push_back((*itUlInfo).m_rnti);
			}
		}
	}

	for (std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.begin(); itBsr != m_ceBsrRxed.end();
			itBsr++) {
		uint16_t rnti = (*itBsr).first;

		if (m_harqOn) {
			bool isHarq = false;
			for (std::vector<uint16_t>::iterator itHarq = harqList.begin(); itHarq != harqList.end();
					itHarq++) {
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
		std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(rnti);
		if (itQci != m_ueUlQci.end() && (*itQci).second > EpsBearer::NGBR_VIDEO_TCP_DEFAULT) {
			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(rnti);
			if (itGrant != m_m2mGrantTimers.end() && (*itGrant).second > 0) {
				continue;
			}

			m2mList.push_back(rnti);

			if (itStats != m_flowStatsUl.end()
					&& (*itStats).second.lastAveragedThroughput > m2mMaxLastAveragedThroughput) {
				m2mMaxLastAveragedThroughput = (*itStats).second.lastAveragedThroughput;
			}
			uint16_t delay = EpsBearer((*itQci).second).GetPacketDelayBudgetMs();
			if (delay > m2mMaxDelay) {
				m2mMaxDelay = delay;
			}
		} else {
			h2hList.push_back(rnti);

			if (itStats != m_flowStatsUl.end() && (*itStats).second.lastAveragedBsrReceived > 0) {
				double demand = ((*itBsr).second * (*itStats).second.lastAverageResourcesAllocated)
						/ (*itStats).second.lastAveragedBsrReceived;
				h2hRbDemand += std::max(m_minH2hRb, static_cast<uint16_t>(ceil(demand)));
			} else {
				h2hRbDemand += m_minH2hRb;
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

	// H2H
	uint16_t nRbAvailable = rbMap.GetAvailableRbSize();
	h2hRbDemand = std::min(h2hRbDemand, nRbAvailable);
	uint16_t m2mMinRbDemand = static_cast<uint16_t>(std::max(
			floor(m_minPercentM2mRb * nRbAvailable / m_minM2mRb),
			floor((nRbAvailable - h2hRbDemand) / m_minM2mRb)));
	m2mMinRbDemand = m_minM2mRb * std::min(m2mMinRbDemand, static_cast<uint16_t>(m2mList.size()));
	uint16_t nH2hRb = nRbAvailable - m2mMinRbDemand;
	if (h2hList.size() > 0 && nH2hRb > 0) {
		SchedUlH2h(h2hList, rbMap, nH2hRb, response);
	}

	// M2M
	// Time Domain
	uint16_t nMaxM2m = std::min(static_cast<uint16_t>(m2mList.size()),
			static_cast<uint16_t>(floor(rbMap.GetAvailableRbSize() / m_minM2mRb)));
	std::map<uint16_t, double> priorityMap;
	std::vector<uint16_t>::iterator itM2m = m2mList.begin();
	while (itM2m != m2mList.end()) {
		uint16_t rnti = *itM2m;
		std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(rnti);
		std::map<uint16_t, EpsBearer::Qci>::iterator itQci = m_ueUlQci.find(rnti);
		double priority = 2;
		if (itStats != m_flowStatsUl.end() && m2mMaxLastAveragedThroughput > 0) {
			priority -= (*itStats).second.lastAveragedThroughput / m2mMaxLastAveragedThroughput;
		}
		if (itQci != m_ueUlQci.end() && m2mMaxDelay > 0) {
			priority -= EpsBearer((*itQci).second).GetPacketDelayBudgetMs() / m2mMaxDelay;
		}
		priority = std::max(priority, 0.0);
		priorityMap.insert(std::pair<uint16_t, double>(rnti, priority));
		itM2m++;
	}
	std::vector<uint16_t> m2mChosen;
	while (m2mChosen.size() < nMaxM2m) {
		std::pair<uint16_t, double> maxPriority(0, 0.0);
		std::map<uint16_t, double>::iterator itPrio = priorityMap.begin();
		while (itPrio != priorityMap.end()) {
			if ((*itPrio).second >= maxPriority.second) {
				maxPriority = *itPrio;
			}
			itPrio++;
		}
		if (maxPriority.first != 0) {
			m2mChosen.push_back(maxPriority.first);
			priorityMap.erase(maxPriority.first);
		}
	}

	// Frequency Domain
	std::map<uint16_t, double> m2mPrioValues;
	for (itM2m = m2mChosen.begin(); itM2m != m2mChosen.end(); itM2m++) {
		m2mPrioValues.insert(std::pair<uint16_t, double>(*itM2m, 0.0));
	}
	for (uint16_t rbStart = rbMap.GetFirstAvailableRb(); rbStart < rbMap.GetSize(); rbStart += m_minM2mRb) {
		std::pair<uint16_t, double> maxPriority(0, 0);
		std::map<uint16_t, double>::iterator itValue = m2mPrioValues.begin();
		while (itValue != m2mPrioValues.end()) {
			uint16_t rnti = (*itValue).first;
			std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
			if (itCqi != m_ueCqi.end()) {
				double minSinr = DBL_MAX;
				for (uint16_t j = 0; j < m_minM2mRb; j++) {
					double sinr = (*itCqi).second.at(rbStart + j);
					if (sinr == NO_SINR) {
						sinr = EstimateUlSinr(rnti, rbStart + j);
					}
					if (sinr < minSinr) {
						minSinr = sinr;
					}
				}
				double spectralEfficiency = log2(
						1.0 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
				int cqi = m_amc->GetCqiFromSpectralEfficiency(spectralEfficiency);
				(*itValue).second = spectralEfficiency;
				if (cqi != 0) {
					if (spectralEfficiency >= maxPriority.second) {
						maxPriority.first = rnti;
						maxPriority.second = spectralEfficiency;
					}
					itValue++;
				} else {
					m2mPrioValues.erase(itValue++);
				}
			} else {
				m2mPrioValues.erase(itValue++);
			}
		}
		if (maxPriority.first != 0) {
			int cqi = m_amc->GetCqiFromSpectralEfficiency(maxPriority.second);
			UlDciListElement_s uldci;
			uldci.m_rnti = maxPriority.first;
			uldci.m_rbStart = rbStart;
			uldci.m_rbLen = m_minM2mRb;
			uldci.m_mcs = m_amc->GetMcsFromCqi(cqi);
			uldci.m_tbSize = m_amc->GetTbSizeFromMcs(uldci.m_mcs, m_minM2mRb) / 8;
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
			m2mPrioValues.erase(uldci.m_rnti);
			rbMap.Allocate(uldci.m_rnti, rbStart, m_minM2mRb);
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

	UpdateM2MAccessGrantTimers(m2mList, rbMap);

//	if (rbMap.GetSize() != rbMap.GetAvailableRbSize()) {
//		NS_LOG_FUNCTION(
//				this << " Ul Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf)); NS_LOG_INFO(this << "RB Available: " << nRbAvailable << ", H2H RB Demand: " << h2hRbDemand << ", H2H RB: " << nH2hRb << ", M2M RB Demand: " << m2mMinRbDemand);
//	}

	// Update global UE stats
	// update UEs stats
	for (std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.begin();
			itStats != m_flowStatsUl.end(); itStats++) {
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

//		if ((*itStats).second.lastTtiResourcesAllocated > 0) {
//			NS_LOG_INFO(
//					this << " UL UE" << ((*itStats).second.isM2m ? " M2M " : " H2H ") << (*itStats).first << " Last bytes "
//					<< (*itStats).second.lastTtiBytesTrasmitted << " Average throughput " << (*itStats).second.lastAveragedThroughput
//					<< " RB " << (*itStats).second.lastTtiResourcesAllocated);
//		}

		(*itStats).second.lastTtiBytesTrasmitted = 0;
		(*itStats).second.lastTtiResourcesAllocated = 0;
		(*itStats).second.lastTtiBsrReceived = 0;
	}

	m_ulAllocationMaps.insert(std::pair<uint16_t, M2mRbAllocationMap>(params.m_sfnSf, rbMap));
	m_schedSapUser->SchedUlConfigInd(response);
}

// --------------------------------------------------------------

void M2mMacScheduler::RefreshDlCqiMaps(void) {
// refresh DL CQI P01 Map
	std::map<uint16_t, uint32_t>::iterator itP10 = m_p10CqiTimers.begin();
	while (itP10 != m_p10CqiTimers.end()) {
//		NS_LOG_INFO(
//				this << " P10-CQI for user " << (*itP10).first << " is "
//				<< (uint32_t)(*itP10).second << " thr "
//				<< (uint32_t) m_cqiTimersThreshold);
		if ((*itP10).second == 0) {
			// delete correspondent entries
			std::map<uint16_t, uint8_t>::iterator itMap = m_p10CqiRxed.find((*itP10).first);
			NS_ASSERT_MSG(itMap != m_p10CqiRxed.end(),
					" Does not find CQI report for user " << (*itP10).first);
//			NS_LOG_INFO(this << " P10-CQI expired for user " << (*itP10).first);
			m_p10CqiRxed.erase(itMap);
			std::map<uint16_t, uint32_t>::iterator temp = itP10;
			itP10++;
			m_p10CqiTimers.erase(temp);
		} else {
			(*itP10).second--;
			itP10++;
		}
	}

// refresh DL CQI A30 Map
	std::map<uint16_t, uint32_t>::iterator itA30 = m_a30CqiTimers.begin();
	while (itA30 != m_a30CqiTimers.end()) {
//		NS_LOG_INFO(
//				this << " A30-CQI for user " << (*itA30).first << " is "
//				<< (uint32_t)(*itA30).second << " thr "
//				<< (uint32_t) m_cqiTimersThreshold);
		if ((*itA30).second == 0) {
			// delete correspondent entries
			std::map<uint16_t, SbMeasResult_s>::iterator itMap = m_a30CqiRxed.find((*itA30).first);
			NS_ASSERT_MSG(itMap != m_a30CqiRxed.end(),
					" Does not find CQI report for user " << (*itA30).first);
//			NS_LOG_INFO(this << " A30-CQI expired for user " << (*itA30).first);
			m_a30CqiRxed.erase(itMap);
			std::map<uint16_t, uint32_t>::iterator temp = itA30;
			itA30++;
			m_a30CqiTimers.erase(temp);
		} else {
			(*itA30).second--;
			itA30++;
		}
	}
	return;
}

void M2mMacScheduler::RefreshUlCqiMaps(void) {
// refresh UL CQI  Map
	std::map<uint16_t, uint32_t>::iterator itUl = m_ueCqiTimers.begin();
	while (itUl != m_ueCqiTimers.end()) {
//		NS_LOG_INFO (this << " UL-CQI for user " << (*itUl).first << " is " << (uint32_t)(*itUl).second << " thr " << (uint32_t)m_cqiTimersThreshold);
		if ((*itUl).second == 0) {
			// delete correspondent entries
			std::map<uint16_t, std::vector<double> >::iterator itMap = m_ueCqi.find((*itUl).first);
			NS_ASSERT_MSG(itMap != m_ueCqi.end(), " Does not find CQI report for user " << (*itUl).first);
//			NS_LOG_INFO (this << " UL-CQI exired for user " << (*itUl).first);
			(*itMap).second.clear();
			m_ueCqi.erase(itMap);
			std::map<uint16_t, uint32_t>::iterator temp = itUl;
			itUl++;
			m_ueCqiTimers.erase(temp);
		} else {
			(*itUl).second--;
			itUl++;
		}
	}
	return;
}

int M2mMacScheduler::GetRbgSize(int dlbandwidth) {
	for (int i = 0; i < 4; i++) {
		if (dlbandwidth < M2mType0AllocationRbg[i]) {
			return (i + 1);
		}
	}
	return (-1);
}

int M2mMacScheduler::LcActivePerFlow(uint16_t rnti) {
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
	int lcActive = 0;
	for (it = m_rlcBufferReq.begin(); it != m_rlcBufferReq.end(); it++) {
		if (((*it).first.m_rnti == rnti)
				&& (((*it).second.m_rlcTransmissionQueueSize > 0)
						|| ((*it).second.m_rlcRetransmissionQueueSize > 0)
						|| ((*it).second.m_rlcStatusPduSize > 0))) {
			lcActive++;
		}
		if ((*it).first.m_rnti > rnti) {
			break;
		}
	}
	return (lcActive);
}

void M2mMacScheduler::RefreshHarqProcesses() {
//	NS_LOG_FUNCTION(this);
	std::map<uint16_t, DlHarqProcessesTimer_t>::iterator itTimers;
	for (itTimers = m_dlHarqProcessesTimer.begin(); itTimers != m_dlHarqProcessesTimer.end(); itTimers++) {
		for (uint16_t i = 0; i < HARQ_PROC_NUM; i++) {
			if ((*itTimers).second.at(i) == HARQ_DL_TIMEOUT) {
				// reset HARQ process
//				NS_LOG_DEBUG(
//						this << " Reset HARQ proc " << i << " for RNTI "
//						<< (*itTimers).first);
				std::map<uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find(
						(*itTimers).first);
				if (itStat == m_dlHarqProcessesStatus.end()) {
					NS_FATAL_ERROR("No Process Id Status found for this RNTI " << (*itTimers).first);
				}
				(*itStat).second.at(i) = 0;
				(*itTimers).second.at(i) = 0;
			} else {
				(*itTimers).second.at(i)++;}
			}
		}
	}

	/**
	 * \brief Return the availability of free process for the RNTI specified
	 *
	 * \param rnti the RNTI of the UE to be updated
	 * \return the process id  value
	 */
uint8_t M2mMacScheduler::HarqProcessAvailability(uint16_t rnti) {
//	NS_LOG_FUNCTION (this << rnti);
	std::map<uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find(rnti);
	if (it == m_dlHarqCurrentProcessId.end()) {
		NS_FATAL_ERROR("No Process Id found for this RNTI " << rnti);
	}
	std::map<uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find(rnti);
	if (itStat == m_dlHarqProcessesStatus.end()) {
		NS_FATAL_ERROR("No Process Id Statusfound for this RNTI " << rnti);
	}
	uint8_t i = (*it).second;
	do {
		i = (i + 1) % HARQ_PROC_NUM;
	} while (((*itStat).second.at(i) != 0) && (i != (*it).second));
	if ((*itStat).second.at(i) == 0) {
		return (true);
	} else {
		return (false); // return a not valid harq proc id
	}
}

/**
 * \brief Update and return a new process Id for the RNTI specified
 *
 * \param rnti the RNTI of the UE to be updated
 * \return the process id  value
 */
uint8_t M2mMacScheduler::UpdateHarqProcessId(uint16_t rnti) {
//	NS_LOG_FUNCTION (this << rnti);
	if (m_harqOn == false) {
		return (0);
	}
	std::map<uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find(rnti);
	if (it == m_dlHarqCurrentProcessId.end()) {
		NS_FATAL_ERROR("No Process Id found for this RNTI " << rnti);
	}
	std::map<uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find(rnti);
	if (itStat == m_dlHarqProcessesStatus.end()) {
		NS_FATAL_ERROR("No Process Id Statusfound for this RNTI " << rnti);
	}
	uint8_t i = (*it).second;
	do {
		i = (i + 1) % HARQ_PROC_NUM;
	} while (((*itStat).second.at(i) != 0) && (i != (*it).second));
	if ((*itStat).second.at(i) == 0) {
		(*it).second = i;
		(*itStat).second.at(i) = 1;
	} else {
		NS_FATAL_ERROR(
				"No HARQ process available for RNTI " << rnti << " check before update with HarqProcessAvailability");
	}
	return ((*it).second);
}

void M2mMacScheduler::UpdateDlRlcBufferInfo(uint16_t rnti, uint8_t lcid, uint16_t size) {
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
	LteFlowId_t flow(rnti, lcid);
	it = m_rlcBufferReq.find(flow);
	if (it != m_rlcBufferReq.end()) {
//		NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it).second.m_rlcTransmissionQueueSize << " retxqueue " << (*it).second.m_rlcRetransmissionQueueSize << " status " << (*it).second.m_rlcStatusPduSize << " decrease " << size);
		// Update queues: RLC tx order Status, ReTx, Tx
		// Update status queue
		if (((*it).second.m_rlcStatusPduSize > 0) && (size >= (*it).second.m_rlcStatusPduSize)) {
			(*it).second.m_rlcStatusPduSize = 0;
		} else if (((*it).second.m_rlcRetransmissionQueueSize > 0)
				&& (size >= (*it).second.m_rlcRetransmissionQueueSize)) {
			(*it).second.m_rlcRetransmissionQueueSize = 0;
		} else if ((*it).second.m_rlcTransmissionQueueSize > 0) {
			uint32_t rlcOverhead;
			if (lcid == 1) {
				// for SRB1 (using RLC AM) it's better to
				// overestimate RLC overhead rather than
				// underestimate it and risk unneeded
				// segmentation which increases delay
				rlcOverhead = 4;
			} else {
				// minimum RLC overhead due to header
				rlcOverhead = 2;
			}
			// update transmission queue
			if ((*it).second.m_rlcTransmissionQueueSize <= size - rlcOverhead) {
				(*it).second.m_rlcTransmissionQueueSize = 0;
			} else {
				(*it).second.m_rlcTransmissionQueueSize -= size - rlcOverhead;
			}
		}
	} else {
		NS_LOG_ERROR (this << " Does not find DL RLC Buffer Report of UE " << rnti);
	}
}

void M2mMacScheduler::UpdateUlRlcBufferInfo(uint16_t rnti, uint16_t size) {
	size = size - 2; // remove the minimum RLC overhead
	std::map<uint16_t, uint32_t>::iterator it = m_ceBsrRxed.find(rnti);
	if (it != m_ceBsrRxed.end()) {
//		NS_LOG_INFO (this << " UE " << rnti << " size " << size << " BSR " << (*it).second);
		if ((*it).second >= size) {
			(*it).second -= size;
		} else {
			(*it).second = 0;
		}
	} else {
		NS_LOG_ERROR (this << " Does not find BSR report info of UE " << rnti);
	}
}

double M2mMacScheduler::EstimateUlSinr(uint16_t rnti, uint16_t rb) {
	std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(rnti);
	if (itCqi == m_ueCqi.end()) {
		// no cqi info about this UE
		return (NO_SINR);
	} else {
		// take the average SINR value among the available
		double sinrSum = 0;
		int sinrNum = 0;
		for (uint32_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++) {
			double sinr = (*itCqi).second.at(i);
			if (sinr != NO_SINR) {
				sinrSum += sinr;
				sinrNum++;
			}
		}
		double estimatedSinr = (sinrNum > 0) ? (sinrSum / sinrNum) : DBL_MAX;
		// store the value
		(*itCqi).second.at(rb) = estimatedSinr;
		return (estimatedSinr);
	}
}

void M2mMacScheduler::SchedUlHarq(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	//   Process UL HARQ feedback
	//   update UL HARQ proc id
	std::map<uint16_t, uint8_t>::iterator itProcId;
	for (itProcId = m_ulHarqCurrentProcessId.begin(); itProcId != m_ulHarqCurrentProcessId.end();
			itProcId++) {
		(*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
	}
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
	for (std::vector<uint16_t>::const_iterator itUe = ueList.begin(); itUe != ueList.end(); itUe++) {
		// retx correspondent block: retrieve the UL-DCI
		uint16_t rnti = *itUe;
		itProcId = m_ulHarqCurrentProcessId.find(rnti);
		if (itProcId == m_ulHarqCurrentProcessId.end()) {
			NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
		}
		uint8_t harqId = (uint8_t) ((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
//		NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << harqId);
		std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find(
				rnti);
		if (itHarq == m_ulHarqProcessesDciBuffer.end()) {
			NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
			continue;
		}
		UlDciListElement_s dci = (*itHarq).second.at(harqId);
		std::map<uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find(rnti);
		if (itStat == m_ulHarqProcessesStatus.end()) {
			NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
		}
		if ((*itStat).second.at(harqId) >= 3) {
//			NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
			continue;
		}
		if (rbStart + dci.m_rbLen <= rbMap.GetSize() && rbMap.IsFree(rbStart, dci.m_rbLen)) {
			rbMap.Allocate(rbStart, dci.m_rbLen);
			dci.m_rbStart = rbStart;
			rbStart += dci.m_rbLen;
		} else {
//			NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
			continue;
		}
		dci.m_ndi = 0;
		// Update HARQ buffers with new HarqId
		(*itStat).second.at((*itProcId).second) = (*itStat).second.at(harqId) + 1;
		(*itStat).second.at(harqId) = 0;
		(*itHarq).second.at((*itProcId).second) = dci;
		response.m_dciList.push_back(dci);
	}
}

void M2mMacScheduler::SchedUlH2h(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
		const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
	uint16_t rbStart = rbMap.GetFirstAvailableRb();
//	uint32_t nRbGroup = 0.5 * rbSize * (rbSize + 1) + 1;
	uint32_t nRbGroup = 0.5 * rbSize * (rbSize + 1);

	// Create de Matrix of possible allocations
//	bool *mtx = new bool[nRbGroup*rbSize];
	std::vector<std::vector<bool> > mtx;
	mtx.resize(nRbGroup, std::vector<bool>(rbSize, false));

//	uint32_t groupSize = 0;
	uint32_t groupSize = 1;
	uint32_t groupStart = 0;
	for (uint32_t c = 0; c < nRbGroup; c++) {
		for (uint32_t r = 0; r < rbSize; r++) {
			bool value = (r >= groupStart && r < groupStart + groupSize) ? true : false;
//			mtx[c * nRbGroup + r] = value;
			mtx[c][r] = value;
//			if ((r + 1 == rbSize) && (value || c == 0)) {
			if ((r + 1 == rbSize) && value) {
				groupSize++;
				groupStart = rbSize;
			}
		}
		if (groupStart < rbSize)
			groupStart++;
		else
			groupStart = 0;
	}
	uint32_t nRbgAvailable = nRbGroup;
	std::vector<uint16_t> ueAvailable = ueList;
	while (nRbgAvailable > 0 && ueAvailable.size() != 0) {
		double maxValue = 0;
		uint32_t rbgChosenSize = 0;
		uint32_t rbgChosenStart = 0;
		double spectralEfficiencyChosen = 0.0;
		std::vector<uint16_t>::iterator itUeChosen = ueAvailable.end();
		std::vector<uint16_t>::iterator itUe = ueAvailable.begin();
		while (itUe != ueAvailable.end()) {
			std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(*itUe);
			std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(*itUe);

			for (uint32_t c = 0; c < nRbGroup; c++) {
				uint32_t rbgSize = 0;
				uint32_t rbgStart = rbSize;
				double minSinr = DBL_MAX;
				double value = 0.0;
				for (uint32_t r = 0; r < rbSize; r++) {
//						if (mtx[c * nRbGroup + r]) {
					if (mtx[c][r]) {
						if (rbgStart == rbSize)
							rbgStart = r;
						rbgSize++;
						if (itCqi != m_ueCqi.end()) {
							double sinr = (*itCqi).second.at(r + rbStart);
							if (sinr == NO_SINR) {
								sinr = EstimateUlSinr(*itUe, r + rbStart);
							}
							if (sinr < minSinr)
								minSinr = sinr;
						}
					}
				}
				if (rbgSize > 0) {
					int mcs = m_ulGrantMcs;
					double spectralEfficiency = 0.0;
					if (minSinr != NO_SINR && minSinr != DBL_MAX) {
						// translate SINR -> cqi: WILD ACK: same as DL
						spectralEfficiency = log2(
								1 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
						int cqi = m_amc->GetCqiFromSpectralEfficiency(spectralEfficiency);
						if (cqi != 0) {
							mcs = m_amc->GetMcsFromCqi(cqi);
						}
					}
					double achievableThr = 1000 * m_amc->GetTbSizeFromMcs(mcs, rbgSize)
							/ 8;
					if (itStats != m_flowStatsUl.end()) {
						if ((*itStats).second.lastAveragedThroughput > 0.0) {
							value = achievableThr / (*itStats).second.lastAveragedThroughput;
						} else {
							value = achievableThr;
						}
					} else {
						value = achievableThr;
					}
					if (value > maxValue || maxValue == 0.0) {
						maxValue = value;
						itUeChosen = itUe;
						rbgChosenStart = rbgStart;
						rbgChosenSize = rbgSize;
						spectralEfficiencyChosen = spectralEfficiency;
					}
				}
			}
			itUe++;
		}
		if (itUeChosen != ueAvailable.end()) {
			int cqi =
					(spectralEfficiencyChosen > 0.0) ?
							m_amc->GetCqiFromSpectralEfficiency(spectralEfficiencyChosen) : 0;
			int mcs = (cqi != 0) ? m_amc->GetMcsFromCqi(cqi) : m_ulGrantMcs;
			UlDciListElement_s uldci;
			uldci.m_rnti = *itUeChosen;
			uldci.m_rbStart = rbStart + rbgChosenStart;
			uldci.m_rbLen = rbgChosenSize;
			uldci.m_mcs = mcs;
			uldci.m_tbSize = (m_amc->GetTbSizeFromMcs(uldci.m_mcs, uldci.m_rbLen) / 8);
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

			UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);
			rbMap.Allocate(uldci.m_rnti, uldci.m_rbStart, uldci.m_rbLen);
			response.m_dciList.push_back(uldci);
			ueAvailable.erase(itUeChosen);

			for (uint32_t c = 0; c < nRbGroup; c++) {
				bool intercepts = false;
				for (uint32_t r = rbgChosenStart; r < rbgChosenStart + rbgChosenSize; r++) {
					if (mtx[c][r]) {
//					if (mtx[c * nRbGroup + r]) {
						intercepts = true;
						break;
					}
				}
				if (intercepts) {
					nRbgAvailable--;
					for (uint32_t r = 0; r < rbSize; r++) {
//						mtx[c * nRbGroup + r] = false;
						mtx[c][r] = false;
					}
				}
			}

			// store DCI for HARQ_PERIOD
			if (m_harqOn == true) {
				uint8_t harqId = 0;
				std::map<uint16_t, uint8_t>::iterator itProcId;
				itProcId = m_ulHarqCurrentProcessId.find(uldci.m_rnti);
				if (itProcId == m_ulHarqCurrentProcessId.end()) {
					NS_FATAL_ERROR("No info find in HARQ buffer for UE " << uldci.m_rnti);
				}
				harqId = (*itProcId).second;
				std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci =
						m_ulHarqProcessesDciBuffer.find(uldci.m_rnti);
				if (itDci == m_ulHarqProcessesDciBuffer.end()) {
					NS_FATAL_ERROR(
							"Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
				}
				(*itDci).second.at(harqId) = uldci;
			}

			std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(uldci.m_rnti);
			std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(uldci.m_rnti);
			if (itStats != m_flowStatsUl.end()) {
				(*itStats).second.lastTtiBytesTrasmitted = uldci.m_tbSize;
				(*itStats).second.lastTtiResourcesAllocated = uldci.m_rbLen;
				(*itStats).second.lastTtiBsrReceived =
						(itBsr != m_ceBsrRxed.end()) ? (*itBsr).second : uldci.m_tbSize;
			}
		} else {
			break;
		}
	}

	// TODO verificar se ainda tem recursos disponiveis e aloca-los

//	delete[] mtx;
}

/*
 void M2mMacScheduler::SchedUlH2h(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
 const uint16_t rbSize, struct FfMacSchedSapUser::SchedUlConfigIndParameters &response) {
 int nflows = ueList.size();
 uint16_t rbStart = rbMap.GetFirstAvailableRb();
 uint16_t rbEnd = rbStart + rbSize;
 // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
 uint16_t minRbPerFlow = 3;

 // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
 uint16_t rbPerFlow = rbSize / nflows;
 if (rbPerFlow < minRbPerFlow) {
 rbPerFlow = minRbPerFlow; // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
 }

 std::vector<uint16_t>::const_iterator it = ueList.begin();
 while (it != ueList.end() && rbStart + minRbPerFlow < rbEnd) {
 if (rbStart + rbPerFlow > rbEnd) {
 rbPerFlow = minRbPerFlow;
 }
 if (rbStart + rbPerFlow + minRbPerFlow > rbEnd) {
 rbPerFlow = rbEnd - rbStart;
 }
 if (rbMap.IsFree(rbStart, rbPerFlow)) {
 UlDciListElement_s uldci;
 uldci.m_rnti = *it;
 uldci.m_rbStart = rbStart;
 uldci.m_rbLen = rbPerFlow;

 std::map<uint16_t, std::vector<double> >::iterator itCqi = m_ueCqi.find(*it);
 if (itCqi != m_ueCqi.end()) {
 // take the lowest CQI value (worst RB)
 double minSinr = (*itCqi).second.at(uldci.m_rbStart);
 if (minSinr == NO_SINR) {
 minSinr = EstimateUlSinr(*it, uldci.m_rbStart);
 }
 for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++) {
 double sinr = (*itCqi).second.at(i);
 if (sinr == NO_SINR) {
 sinr = EstimateUlSinr(*it, i);
 }
 if ((*itCqi).second.at(i) < minSinr) {
 minSinr = (*itCqi).second.at(i);
 }
 }
 // translate SINR -> cqi: WILD ACK: same as DL
 double s = log2(1 + (std::pow(10, minSinr / 10) / ((-std::log(5.0 * 0.00005)) / 1.5)));
 int cqi = m_amc->GetCqiFromSpectralEfficiency(s);
 if (cqi != 0) {
 uldci.m_mcs = m_amc->GetMcsFromCqi(cqi);
 uldci.m_tbSize = (m_amc->GetTbSizeFromMcs(uldci.m_mcs, rbPerFlow) / 8);
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

 UpdateUlRlcBufferInfo(uldci.m_rnti, uldci.m_tbSize);
 rbMap.Allocate(uldci.m_rnti, uldci.m_rbStart, uldci.m_rbLen);
 response.m_dciList.push_back(uldci);
 rbStart += uldci.m_rbLen;

 // store DCI for HARQ_PERIOD
 if (m_harqOn == true) {
 uint8_t harqId = 0;
 std::map<uint16_t, uint8_t>::iterator itProcId;
 itProcId = m_ulHarqCurrentProcessId.find(uldci.m_rnti);
 if (itProcId == m_ulHarqCurrentProcessId.end()) {
 NS_FATAL_ERROR("No info find in HARQ buffer for UE " << uldci.m_rnti);
 }
 harqId = (*itProcId).second;
 std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci =
 m_ulHarqProcessesDciBuffer.find(uldci.m_rnti);
 if (itDci == m_ulHarqProcessesDciBuffer.end()) {
 NS_FATAL_ERROR(
 "Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
 }
 (*itDci).second.at(harqId) = uldci;
 }

 std::map<uint16_t, m2mFlowPerf_t>::iterator itStats = m_flowStatsUl.find(uldci.m_rnti);
 std::map<uint16_t, uint32_t>::iterator itBsr = m_ceBsrRxed.find(uldci.m_rnti);
 if (itStats != m_flowStatsUl.end()) {
 (*itStats).second.lastTtiBytesTrasmitted = uldci.m_tbSize;
 (*itStats).second.lastTtiResourcesAllocated = uldci.m_rbLen;
 (*itStats).second.lastTtiBsrReceived =
 (itBsr != m_ceBsrRxed.end()) ? (*itBsr).second : uldci.m_tbSize;
 }
 }
 }
 }
 it++;
 }
 }
 */

void M2mMacScheduler::RefreshM2MAccessGrantTimers() {
	for (std::map<uint16_t, uint32_t>::iterator it = m_m2mGrantTimers.begin(); it != m_m2mGrantTimers.end();
			it++) {
		if ((*it).second > 0) {
			(*it).second--;
		}
	}
}

void M2mMacScheduler::UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList,
		const M2mRbAllocationMap &rbMap) {
	for (std::vector<uint16_t>::const_iterator itM2m = ueList.begin(); itM2m != ueList.end(); itM2m++) {
		if (!rbMap.HasResources(*itM2m)) {
			std::map<uint16_t, EpsBearer::Qci>::iterator itQci = m_ueUlQci.find(*itM2m);
			std::map<uint16_t, uint32_t>::iterator itGrant = m_m2mGrantTimers.find(*itM2m);
			if (itQci != m_ueUlQci.end()) {
				uint16_t delay = EpsBearer((*itQci).second).GetPacketDelayBudgetMs();
				uint32_t waitTime = m_uniformRandom->GetInteger(0, delay / 2);
				if (itGrant != m_m2mGrantTimers.end()) {
					(*itGrant).second = waitTime;
				} else {
					m_m2mGrantTimers.insert(std::pair<uint16_t, uint32_t>(*itM2m, waitTime));
				}
			}
		}
	}
}

}

