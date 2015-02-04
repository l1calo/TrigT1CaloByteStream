#include "CaloUserHeader.h"
#include "WordDecoder.h"

using namespace LVL1BS;

namespace {
  WordDecoder caloUserWordDecoder({
    BitField("Length", 0, 4),
    BitField("L1A Slice: PP FADC", 4, 5),
    BitField("LUT", 9, 3),
    BitField("CP", 12, 4),
    BitField("JEP", 16, 4),
    BitField("PP Lower Bound", 20, 8)
 });
}


CaloUserHeader::CaloUserHeader(uint32_t header):
  m_header(header){
}


uint8_t CaloUserHeader::length() const { return ::caloUserWordDecoder.get<uint8_t>(m_header, 0); }
uint8_t CaloUserHeader::ppFadc() const  { return ::caloUserWordDecoder.get<uint8_t>(m_header, 1); }
uint8_t CaloUserHeader::lut()  const { return ::caloUserWordDecoder.get<uint8_t>(m_header,2); }
uint8_t CaloUserHeader::cp()  const { return ::caloUserWordDecoder.get<uint8_t>(m_header, 3); }
uint8_t CaloUserHeader::jep() const  { return ::caloUserWordDecoder.get<uint8_t>(m_header, 4); }
uint8_t CaloUserHeader::ppLowerBound() const  { return ::caloUserWordDecoder.get<uint8_t>(m_header, 5); }
bool CaloUserHeader::isValid() const { return length() == 0xf;}
