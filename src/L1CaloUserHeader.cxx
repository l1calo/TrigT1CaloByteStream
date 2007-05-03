
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"

namespace LVL1BS {

// Static constant definitions

const int      L1CaloUserHeader::s_jepCmmBit;
const int      L1CaloUserHeader::s_cpCmmBit;
const int      L1CaloUserHeader::s_jemBit;
const int      L1CaloUserHeader::s_cpmBit;
const int      L1CaloUserHeader::s_ppmLutBit;
const int      L1CaloUserHeader::s_ppmFadcBit;
const uint32_t L1CaloUserHeader::s_mask;

// Constructor

L1CaloUserHeader::L1CaloUserHeader(uint32_t header) : m_header(header)
{
}

// Test for valid header word

bool L1CaloUserHeader::isValid(const uint32_t word)
{
  return ((word >> s_wordIdBit) & s_mask) == s_mask;
}

} // end namespace
