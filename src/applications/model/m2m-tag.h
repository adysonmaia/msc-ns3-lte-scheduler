/*
 * m2m-tag-header.h
 *
 *  Created on: 29/10/2013
 *      Author: adyson
 */

#ifndef M2M_TAG_H_
#define M2M_TAG_H_

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class M2mTag: public Tag {
public:
	M2mTag();
	M2mTag(uint32_t delay);
	M2mTag(uint32_t delay, uint64_t txTime);
	virtual ~M2mTag();

	uint32_t GetMaxPacketDelay() const;
	Time GetTxTime() const;
	void SetTxTime(Time txTime);
	void SetMaxPacketDelay(uint32_t delay);

	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual uint32_t GetSerializedSize() const;
	virtual void Print(std::ostream &os) const;
private:
	uint32_t m_delay;
	uint64_t m_txTime;
};
}

#endif /* M2M_TAG_H_ */
