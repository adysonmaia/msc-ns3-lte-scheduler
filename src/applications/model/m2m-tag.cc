/*
 * m2m-tag-header.cc
 *
 *  Created on: 29/10/2013
 *      Author: adyson
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/tag.h"
#include "m2m-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("M2mTag");

NS_OBJECT_ENSURE_REGISTERED(M2mTag);

M2mTag::M2mTag() :
		m_delay(0) {
	m_txTime = Simulator::Now().GetTimeStep();
}

M2mTag::M2mTag(uint32_t delayBudget, uint64_t txTime) :
		m_delay(delayBudget), m_txTime(txTime) {
}

M2mTag::M2mTag(uint32_t delayBudget) :
		m_delay(delayBudget) {
	m_txTime = Simulator::Now().GetTimeStep();
}

M2mTag::~M2mTag() {
}

uint32_t M2mTag::GetMaxPacketDelay() const {
	return m_delay;
}

Time M2mTag::GetTxTime() const {
	return TimeStep(m_txTime);
}

void M2mTag::SetTxTime(Time txTime) {
	m_txTime = txTime.GetTimeStep();
}

void M2mTag::SetMaxPacketDelay(uint32_t delay) {
	m_delay = delay;
}

TypeId M2mTag::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::M2mTag").SetParent<Tag>().AddConstructor<M2mTag>();
	return tid;
}

TypeId M2mTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}

uint32_t M2mTag::GetSerializedSize(void) const {
	return 12; // bytes
}

void M2mTag::Serialize(TagBuffer i) const {
	i.WriteU32(m_delay);
	i.WriteU64(m_txTime);
	NS_LOG_INFO("Serialize tag delay " << m_delay << " tx time " << m_txTime);
}

void M2mTag::Deserialize(TagBuffer i) {
	m_delay = (uint32_t) i.ReadU32();
	m_txTime = (uint64_t) i.ReadU64();
	NS_LOG_INFO("Deserialize tag delay " << m_delay << " tx time " << m_txTime);
}

void M2mTag::Print(std::ostream &os) const {
	os << "delay=" << m_delay << ", tx time=" << m_txTime;
}

}
