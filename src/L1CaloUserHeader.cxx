
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"


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
