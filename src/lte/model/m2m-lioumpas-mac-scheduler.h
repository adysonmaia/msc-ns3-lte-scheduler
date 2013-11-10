/*
 * m2m-mac-lioumpas-scheduler.h
 *
 *  Created on: 31/10/2013
 *      Author: adyson
 */

#ifndef M2M_MAC_LIOUMPAS_SCHEDULER_H_
#define M2M_MAC_LIOUMPAS_SCHEDULER_H_

#include "ns3/m2m-mac-scheduler.h"

namespace ns3 {

class M2mLioumpasMacScheduler: public M2mMacScheduler {
public:
	M2mLioumpasMacScheduler();
	virtual ~M2mLioumpasMacScheduler();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);
protected:
	virtual void SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap, const uint16_t rbSize,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
	virtual void UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList, const M2mRbAllocationMap &rbMap,
			const std::map<uint16_t, uint32_t> &delayMap);
};

}

#endif /* M2M_MAC_LIOUMPAS_SCHEDULER_H_ */
