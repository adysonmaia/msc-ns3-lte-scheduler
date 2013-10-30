/*
 * m2m-scheduler-param.cc
 *
 *  Created on: 30/10/2013
 *      Author: adyson
 */

#include "ns3/log.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-ue-rrc.h"
#include "m2m-scheduler-param.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("M2mSchedulerParam");

NS_OBJECT_ENSURE_REGISTERED(M2mSchedulerParam);

M2mSchedulerParam::M2mSchedulerParam() {
}

M2mSchedulerParam::~M2mSchedulerParam() {
}

TypeId M2mSchedulerParam::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::M2mSchedulerParam").SetParent<Object>().AddConstructor<M2mSchedulerParam>();
	return tid;
}

TypeId M2mSchedulerParam::GetInstanceTypeId(void) const {
	return TypeId();
}

void M2mSchedulerParam::SetMaxPacketDelay(Ptr<NetDevice> dev, uint32_t delay) {
	std::map<Ptr<NetDevice>, uint32_t>::iterator it = m_delay.find(dev);
	if (it != m_delay.end()) {
		(*it).second = delay;
	} else {
		m_delay.insert(std::pair<Ptr<NetDevice>, uint32_t>(dev, delay));
	}
}

uint32_t M2mSchedulerParam::GetMaxPacketDelay(uint16_t rnti) {
	std::map<Ptr<NetDevice>, uint32_t>::iterator it;
	for (it = m_delay.begin(); it != m_delay.end(); it++) {
		uint16_t itRnti = it->first->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti();
		if (itRnti == rnti) {
			return it->second;
		}
	}
	return 0;
}

uint32_t M2mSchedulerParam::GetMaxPacketDelay(Ptr<NetDevice> dev) {
	std::map<Ptr<NetDevice>, uint32_t>::iterator it = m_delay.find(dev);
	if (it != m_delay.end()) {
		return it->second;
	} else {
		return 0;
	}
}

} /* namespace ns3 */
