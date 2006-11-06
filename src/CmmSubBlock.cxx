
#include "TrigT1CaloByteStream/CmmSubBlock.h"

// Static constant definitions

const int      CmmSubBlock::s_wordIdVal;

const int      CmmSubBlock::s_cmmSummingBit;
const int      CmmSubBlock::s_cmmFirmwareBit;
const int      CmmSubBlock::s_cmmPositionBit;
const uint32_t CmmSubBlock::s_cmmSummingMask;
const uint32_t CmmSubBlock::s_cmmFirmwareMask;
const uint32_t CmmSubBlock::s_cmmPositionMask;

CmmSubBlock::CmmSubBlock()
{
}

CmmSubBlock::~CmmSubBlock()
{
}

// Store CMM header

void CmmSubBlock::setCmmHeader(int version, int format, int slice, int crate,
                     int summing, int firmware, int position, int timeslices)
{
  int module = 0;
  module |= (summing  & s_cmmSummingMask)  << s_cmmSummingBit;
  module |= (firmware & s_cmmFirmwareMask) << s_cmmFirmwareBit;
  module |= (position & s_cmmPositionMask) << s_cmmPositionBit;
  setHeader(s_wordIdVal, version, format, slice, crate, module, 0,
                                                           timeslices);
}

// Static function to determine CMM type

CmmSubBlock::CmmFirmwareCode CmmSubBlock::cmmType(uint32_t word)
{
  CmmFirmwareCode type;
  int module = L1CaloSubBlock::module(word);
  int code   = (module >> s_cmmFirmwareBit) & s_cmmFirmwareMask;
  switch (code) {
    case CMM_CP:
      type = CMM_CP;
      break;
    case CMM_JET:
      type = CMM_JET;
      break;
    case CMM_ENERGY:
      type = CMM_ENERGY;
      break;
    default:
      type = CMM_UNKNOWN;
      break;
  }
  return type;
}

// Static function to determine if header word corresponds to CMM block

bool CmmSubBlock::cmmBlock(uint32_t word)
{
  return L1CaloSubBlock::wordId(word) == s_wordIdVal;
}
