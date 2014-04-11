/*
 * m2m-abdalla-mac-scheduler.h
 *
 *  Created on: 11/04/2014
 *      Author: adyson
 */

#ifndef M2M_ABDALLA_MAC_SCHEDULER_H_
#define M2M_ABDALLA_MAC_SCHEDULER_H_

#include "ns3/m2m-mac-scheduler.h"

namespace ns3 {

class M2mAbdallaMacScheduler: public M2mMacScheduler {
public:
	M2mAbdallaMacScheduler();
	virtual ~M2mAbdallaMacScheduler();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);
protected:
	virtual void DoSchedUlTriggerReq(const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);
	virtual void SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap, const uint16_t rbSize,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
	virtual void SchedUlH2h(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap, const uint16_t rbSize,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
	void SchedUlUeType(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap, const uint16_t rbSize,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
};

} /* namespace ns3 */

#endif /* M2M_ABDALLA_MAC_SCHEDULER_H_ */
