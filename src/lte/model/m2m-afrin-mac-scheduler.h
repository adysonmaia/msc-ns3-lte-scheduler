/*
 * m2m-afrin-mac-scheduler.h
 *
 *  Created on: 30/04/2014
 *      Author: adyson
 */

#ifndef M2M_AFRIN_MAC_SCHEDULER_H_
#define M2M_AFRIN_MAC_SCHEDULER_H_

#include <ns3/m2m-mac-scheduler.h>

namespace ns3 {

class M2mAfrinMacScheduler: public M2mMacScheduler {
public:
	M2mAfrinMacScheduler();
	virtual ~M2mAfrinMacScheduler();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);

protected:
	virtual void SchedUlM2m(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap, const uint16_t rbSize,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
	virtual void UpdateM2MAccessGrantTimers(const std::vector<uint16_t> &ueList, const M2mRbAllocationMap &rbMap,
			const std::map<uint16_t, uint32_t> &delayMap);
	double GetUlM2mPriority(uint16_t rnti, uint32_t maxBsr);

};

} /* namespace ns3 */

#endif /* M2M_AFRIN_MAC_SCHEDULER_H_ */
