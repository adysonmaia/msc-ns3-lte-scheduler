#ifndef M2M_MAC_SCHEDULER_2_H_
#define M2M_MAC_SCHEDULER_2_H_

#include "ns3/m2m-mac-scheduler.h"

namespace ns3 {

class M2mMacScheduler2: public M2mMacScheduler {
public:
	M2mMacScheduler2();
	virtual ~M2mMacScheduler2();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);
protected:
	virtual void DoSchedUlTriggerReq(const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);
protected:
	double m_congestionLow;
	double m_congestionHigh;
	double m_congestion;
	double m_congestionTimeWindow;
	double m_minPercentM2mRbLow;
	double m_minPercentM2mRbNormal;
	double m_minPercentM2mRbHigh;
};

}

#endif /* M2M_MAC_SCHEDULER_2_H_ */
