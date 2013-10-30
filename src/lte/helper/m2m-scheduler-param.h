/*
 * m2m-scheduler-param.h
 *
 *  Created on: 30/10/2013
 *      Author: adyson
 */

#ifndef M2M_SCHEDULER_PARAM_H_
#define M2M_SCHEDULER_PARAM_H_

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/net-device.h"


namespace ns3 {

class M2mSchedulerParam: public Object {
public:
	M2mSchedulerParam();
	virtual ~M2mSchedulerParam();
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;

	void SetMaxPacketDelay(Ptr<NetDevice> dev, uint32_t delay);
	uint32_t GetMaxPacketDelay(uint16_t rnti);
	uint32_t GetMaxPacketDelay(Ptr<NetDevice> dev);
private:
	std::map<Ptr<NetDevice>, uint32_t > m_delay;
};

} /* namespace ns3 */
#endif /* M2M_SCHEDULER_PARAM_H_ */
