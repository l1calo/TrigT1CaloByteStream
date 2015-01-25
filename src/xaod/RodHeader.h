#ifndef TRIGT1CALOBYTESTREAM_RODHEADER_H
#define TRIGT1CALOBYTESTREAM_RODHEADER_H

namespace LVL1BS {

class RodHeader {
private:
	uint8_t m_sourceId;
	uint16_t m_rodVer;

public:
	RodHeader(uint8_t sourceId, uint16_t rodVer):
		m_sourceId(sourceId), m_rodVer(rodVer){}

	uint8_t sourceId() const { return m_source_id;}
	uint16_t rodVer() const { return m_rodVer;}
};

}



#endif
