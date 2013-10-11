/*
 * m2m-mac-scheduler.h
 *
 *  Created on: Oct 8, 2013
 *      Author: adyson
 */

#ifndef M2M_MAC_SCHEDULER_H_
#define M2M_MAC_SCHEDULER_H_

#include <ns3/lte-common.h>
#include <ns3/ff-mac-csched-sap.h>
#include <ns3/ff-mac-sched-sap.h>
#include <ns3/ff-mac-scheduler.h>
#include <ns3/nstime.h>
#include <ns3/lte-amc.h>
#include <ns3/fdbet-ff-mac-scheduler.h>
#include <vector>
#include <map>

#define HARQ_PROC_NUM 8
#define HARQ_DL_TIMEOUT 11

// value for SINR outside the range defined by FF-API, used to indicate that there
// is no CQI for this element
#define NO_SINR -5000

namespace ns3 {

struct pfsFlowPerf_t {
	Time flowStart;
	unsigned long totalBytesTransmitted;
	unsigned int lastTtiBytesTrasmitted;
	double lastAveragedThroughput;
};

class M2mMacScheduler: public FfMacScheduler {
public:
	M2mMacScheduler();
	virtual ~M2mMacScheduler();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);

	// inherited from FfMacScheduler
	virtual void SetFfMacCschedSapUser(FfMacCschedSapUser* s);
	virtual void SetFfMacSchedSapUser(FfMacSchedSapUser* s);
	virtual FfMacCschedSapProvider* GetFfMacCschedSapProvider();
	virtual FfMacSchedSapProvider* GetFfMacSchedSapProvider();

	friend class M2mSchedulerMemberCschedSapProvider;
	friend class M2mSchedulerMemberSchedSapProvider;

private:
	//
	// Implementation of the CSCHED API primitives
	// (See 4.1 for description of the primitives)
	//
	void DoCschedCellConfigReq(
			const struct FfMacCschedSapProvider::CschedCellConfigReqParameters& params);
	void DoCschedUeConfigReq(
			const struct FfMacCschedSapProvider::CschedUeConfigReqParameters& params);
	void DoCschedLcConfigReq(
			const struct FfMacCschedSapProvider::CschedLcConfigReqParameters& params);
	void DoCschedLcReleaseReq(
			const struct FfMacCschedSapProvider::CschedLcReleaseReqParameters& params);
	void DoCschedUeReleaseReq(
			const struct FfMacCschedSapProvider::CschedUeReleaseReqParameters& params);

	//
	// Implementation of the SCHED API primitives
	// (See 4.2 for description of the primitives)
	//
	void DoSchedDlRlcBufferReq(
			const struct FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params);
	void DoSchedDlPagingBufferReq(
			const struct FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params);
	void DoSchedDlMacBufferReq(
			const struct FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params);
	void DoSchedDlTriggerReq(
			const struct FfMacSchedSapProvider::SchedDlTriggerReqParameters& params);
	void DoSchedDlRachInfoReq(
			const struct FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params);
	void DoSchedDlCqiInfoReq(
			const struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params);
	void DoSchedUlTriggerReq(
			const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);
	void DoSchedUlTriggerReq_Old(
				const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);
	void DoSchedUlNoiseInterferenceReq(
			const struct FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params);
	void DoSchedUlSrInfoReq(
			const struct FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params);
	void DoSchedUlMacCtrlInfoReq(
			const struct FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params);
	void DoSchedUlCqiInfoReq(
			const struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params);

private:
	void RefreshDlCqiMaps(void);
	void RefreshUlCqiMaps(void);

	void UpdateDlRlcBufferInfo(uint16_t rnti, uint8_t lcid, uint16_t size);
	void UpdateUlRlcBufferInfo(uint16_t rnti, uint16_t size);

	void RefreshHarqProcesses();
	uint8_t HarqProcessAvailability(uint16_t rnti);
	uint8_t UpdateHarqProcessId(uint16_t rnti);

	int GetRbgSize(int dlbandwidth);
	int LcActivePerFlow(uint16_t rnti);
	double EstimateUlSinr(uint16_t rnti, uint16_t rb);
private:
	Ptr<LteAmc> m_amc;

	// MAC SAPs
	FfMacCschedSapUser* m_cschedSapUser;
	FfMacSchedSapUser* m_schedSapUser;
	FfMacCschedSapProvider* m_cschedSapProvider;
	FfMacSchedSapProvider* m_schedSapProvider;

	// Internal parameters
	FfMacCschedSapProvider::CschedCellConfigReqParameters m_cschedCellConfig;
	/*
	 * Vectors of UE's LC info
	 */
	std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> m_rlcBufferReq;

	// Map of UE statistics (per RNTI basis)
	std::map<uint16_t, pfsFlowPerf_t> m_flowStatsDl;
	std::map<uint16_t, pfsFlowPerf_t> m_flowStatsUl;

	/*
	 * Map of UE's buffer status reports received
	 */
	std::map<uint16_t, uint32_t> m_ceBsrRxed;

	std::map<uint16_t, uint8_t> m_uesTxMode; // txMode of the UEs <rtni,transmissionMode>

	uint32_t m_cqiTimersThreshold; // # of TTIs for which a CQI can be considered valid

	/*
	 * Map of UE's DL CQI P01 received
	 */
	std::map<uint16_t, uint8_t> m_p10CqiRxed;
	/*
	 * Map of UE's timers on DL CQI P01 received
	 */
	std::map<uint16_t, uint32_t> m_p10CqiTimers;

	/*
	 * Map of UE's DL CQI A30 received
	 */
	std::map<uint16_t, SbMeasResult_s> m_a30CqiRxed;
	/*
	 * Map of UE's timers on DL CQI A30 received
	 */
	std::map<uint16_t, uint32_t> m_a30CqiTimers;

	double m_timeWindow;

	// HARQ attributes <rtni,*>
	/**
	 * m_harqOn when false inhibit te HARQ mechanisms (by default active)
	 */
	bool m_harqOn;
	std::map<uint16_t, uint8_t> m_dlHarqCurrentProcessId;
	// HARQ status
	// 0: process Id available
	// x>0: process Id equal to `x` trasmission count
	std::map<uint16_t, DlHarqProcessesStatus_t> m_dlHarqProcessesStatus;
	std::map<uint16_t, DlHarqProcessesTimer_t> m_dlHarqProcessesTimer;
	std::map<uint16_t, DlHarqProcessesDciBuffer_t> m_dlHarqProcessesDciBuffer;
	std::map<uint16_t, DlHarqRlcPduListBuffer_t> m_dlHarqProcessesRlcPduListBuffer;
	std::vector<DlInfoListElement_s> m_dlInfoListBuffered; // HARQ retx buffered
	std::map<uint16_t, uint8_t> m_ulHarqCurrentProcessId;
	std::map<uint16_t, UlHarqProcessesStatus_t> m_ulHarqProcessesStatus;
	std::map<uint16_t, UlHarqProcessesDciBuffer_t> m_ulHarqProcessesDciBuffer;

	// RACH attributes
	std::vector<struct RachListElement_s> m_rachList;
	std::vector<uint16_t> m_rachAllocationMap; // [rb i] = rnti
	uint8_t m_ulGrantMcs; // MCS for UL grant (default 0)

	// UL
	uint16_t m_nextRntiUl; // RNTI of the next user to be served next scheduling in UL
	/*
	 * Map of previous allocated UE per RBG
	 * (used to retrieve info from UL-CQI)
	 */
	std::map<uint16_t, std::vector<uint16_t> > m_allocationMaps;
	/*
	 * Map of UEs' UL-CQI per RBG
	 */
	std::map<uint16_t, std::vector<double> > m_ueCqi;
	/*
	 * Map of UEs' timers on UL-CQI per RBG
	 */
	std::map<uint16_t, uint32_t> m_ueCqiTimers;
};

class M2mSchedulerMemberCschedSapProvider: public FfMacCschedSapProvider {
public:
	M2mSchedulerMemberCschedSapProvider(M2mMacScheduler* scheduler);

	// inherited from FfMacCschedSapProvider
	virtual void CschedCellConfigReq(
			const struct CschedCellConfigReqParameters& params);
	virtual void CschedUeConfigReq(
			const struct CschedUeConfigReqParameters& params);
	virtual void CschedLcConfigReq(
			const struct CschedLcConfigReqParameters& params);
	virtual void CschedLcReleaseReq(
			const struct CschedLcReleaseReqParameters& params);
	virtual void CschedUeReleaseReq(
			const struct CschedUeReleaseReqParameters& params);

private:
	M2mSchedulerMemberCschedSapProvider();
	M2mMacScheduler* m_scheduler;
};

class M2mSchedulerMemberSchedSapProvider: public FfMacSchedSapProvider {
public:
	M2mSchedulerMemberSchedSapProvider(M2mMacScheduler* scheduler);

	// inherited from FfMacSchedSapProvider
	virtual void SchedDlRlcBufferReq(
			const struct SchedDlRlcBufferReqParameters& params);
	virtual void SchedDlPagingBufferReq(
			const struct SchedDlPagingBufferReqParameters& params);
	virtual void SchedDlMacBufferReq(
			const struct SchedDlMacBufferReqParameters& params);
	virtual void SchedDlTriggerReq(
			const struct SchedDlTriggerReqParameters& params);
	virtual void SchedDlRachInfoReq(
			const struct SchedDlRachInfoReqParameters& params);
	virtual void SchedDlCqiInfoReq(
			const struct SchedDlCqiInfoReqParameters& params);
	virtual void SchedUlTriggerReq(
			const struct SchedUlTriggerReqParameters& params);
	virtual void SchedUlNoiseInterferenceReq(
			const struct SchedUlNoiseInterferenceReqParameters& params);
	virtual void SchedUlSrInfoReq(
			const struct SchedUlSrInfoReqParameters& params);
	virtual void SchedUlMacCtrlInfoReq(
			const struct SchedUlMacCtrlInfoReqParameters& params);
	virtual void SchedUlCqiInfoReq(
			const struct SchedUlCqiInfoReqParameters& params);

private:
	M2mSchedulerMemberSchedSapProvider();
	M2mMacScheduler* m_scheduler;
};

#endif /* M2M_MAC_SCHEDULER_H_ */

}
