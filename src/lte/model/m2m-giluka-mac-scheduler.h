/*
 * m2m-giluka-mac-scheduler.h
 *
 *  Created on: 01/05/2014
 *      Author: adyson
 */

#ifndef M2M_GILUKA_MAC_SCHEDULER_H_
#define M2M_GILUKA_MAC_SCHEDULER_H_

namespace ns3 {

class M2mGilukaMacScheduler: public M2mMacScheduler {
public:
	M2mGilukaMacScheduler();
	virtual ~M2mGilukaMacScheduler();

	// inherited from Object
	virtual void DoDispose(void);
	static TypeId GetTypeId(void);
protected:
	virtual void DoSchedUlTriggerReq(const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);
	void SchedUlClass(const std::vector<uint16_t> &ueList, M2mRbAllocationMap &rbMap,
			struct FfMacSchedSapUser::SchedUlConfigIndParameters &response);
};

} /* namespace ns3 */

#endif /* M2M_GILUKA_MAC_SCHEDULER_H_ */
