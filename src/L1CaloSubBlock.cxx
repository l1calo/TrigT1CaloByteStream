
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"

// Static constant definitions

const int      L1CaloSubBlock::s_headerBit;
const int      L1CaloSubBlock::s_statusBit;
const uint32_t L1CaloSubBlock::s_errorMarker;
const uint32_t L1CaloSubBlock::s_headerMask;
const uint32_t L1CaloSubBlock::s_statusMask;
const uint32_t L1CaloSubBlock::s_headerVal;
const uint32_t L1CaloSubBlock::s_statusVal;

const int      L1CaloSubBlock::s_wordIdBit;
const int      L1CaloSubBlock::s_versionBit;
const int      L1CaloSubBlock::s_formatBit;
const int      L1CaloSubBlock::s_seqnoBit;
const int      L1CaloSubBlock::s_crateBit;
const int      L1CaloSubBlock::s_moduleBit;
const int      L1CaloSubBlock::s_slices2Bit;
const int      L1CaloSubBlock::s_slices1Bit;
const uint32_t L1CaloSubBlock::s_wordIdMask;
const uint32_t L1CaloSubBlock::s_versionMask;
const uint32_t L1CaloSubBlock::s_formatMask;
const uint32_t L1CaloSubBlock::s_seqnoMask;
const uint32_t L1CaloSubBlock::s_crateMask;
const uint32_t L1CaloSubBlock::s_moduleMask;
const uint32_t L1CaloSubBlock::s_slices2Mask;
const uint32_t L1CaloSubBlock::s_slices1Mask;
const uint32_t L1CaloSubBlock::s_wordIdVal;
const uint32_t L1CaloSubBlock::s_cmmWordIdVal;

const int      L1CaloSubBlock::s_cmmSummingBit;
const int      L1CaloSubBlock::s_cmmFirmwareBit;
const int      L1CaloSubBlock::s_cmmPositionBit;
const uint32_t L1CaloSubBlock::s_cmmSummingMask;
const uint32_t L1CaloSubBlock::s_cmmFirmwareMask;
const uint32_t L1CaloSubBlock::s_cmmPositionMask;

const int      L1CaloSubBlock::s_failingBcnBit;
const int      L1CaloSubBlock::s_glinkTimeoutBit;
const int      L1CaloSubBlock::s_glinkDownBit;
const int      L1CaloSubBlock::s_daqOverflowBit;
const int      L1CaloSubBlock::s_bcnMismatchBit;
const int      L1CaloSubBlock::s_glinkProtocolBit;
const int      L1CaloSubBlock::s_glinkParityBit;
const uint32_t L1CaloSubBlock::s_failingBcnMask;

const int      L1CaloSubBlock::s_maxWordBits;
const int      L1CaloSubBlock::s_maxStreamedBits;
const uint32_t L1CaloSubBlock::s_maxWordMask;
const uint32_t L1CaloSubBlock::s_maxStreamedMask;

L1CaloSubBlock::L1CaloSubBlock() : m_header(0), m_trailer(0),
                                   m_bitword(0), m_currentBit(0),
				   m_unpackerFlag(false)
{
  m_maxBits = s_maxWordBits;
  m_maxMask = s_maxWordMask;
}

L1CaloSubBlock::~L1CaloSubBlock()
{
}

// Clear all data

void L1CaloSubBlock::clear()
{
  m_header = 0;
  m_trailer = 0;
  m_bitword = 0;
  m_currentBit = 0;
  m_unpackerFlag = false;
  m_data.clear();
}

// Input complete packed sub-block from ROD vector

OFFLINE_FRAGMENTS_NAMESPACE::PointerType L1CaloSubBlock::read(
                        const OFFLINE_FRAGMENTS_NAMESPACE::PointerType beg,
			const OFFLINE_FRAGMENTS_NAMESPACE::PointerType end)
{
  OFFLINE_FRAGMENTS_NAMESPACE::PointerType pos(beg);
  OFFLINE_FRAGMENTS_NAMESPACE::PointerType pose(end);
  for (; pos != pose; ++pos) {
    uint32_t word = *pos;
    SubBlockWordType type = wordType(word);
    if (type == DATA_HEADER || type == ERROR_HEADER) {
      if (m_header) return pos;
      m_header = word;
    }
    else if (type == ERROR_STATUS) m_trailer = word;
    else m_data.push_back(word);
  }
  return pose;
}

// Output complete packed sub-block to ROD vector

void L1CaloSubBlock::write(
                  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD) const
{
  theROD->push_back(m_header);
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_data.begin(); pos != m_data.end(); ++pos) {
    theROD->push_back(*pos);
  }
  if (m_trailer) theROD->push_back(m_trailer);
}

// Store PPM header

void L1CaloSubBlock::setPpmHeader(int version, int format, int seqno, int crate,
                                  int module, int slicesFadc, int slicesLut)
{
  setHeader(s_wordIdVal, version, format, seqno, crate, module,
                                                 slicesFadc, slicesLut);
}

// Store JEM header

void L1CaloSubBlock::setJemHeader(int version, int format, int slice, int crate,
                                  int module, int timeslices)
{
  setHeader(s_wordIdVal, version, format, slice, crate, module, 0, timeslices);
}

// Store CPM header

void L1CaloSubBlock::setCpmHeader(int version, int format, int slice, int crate,
                                  int module, int timeslices)
{
  setHeader(s_wordIdVal, version, format, slice, crate, module, 0, timeslices);
}

// Store CMM header

void L1CaloSubBlock::setCmmHeader(int version, int format, int slice, int crate,
                     int summing, int firmware, int position, int timeslices)
{
  int module = 0;
  module |= (summing  & s_cmmSummingMask)  << s_cmmSummingBit;
  module |= (firmware & s_cmmFirmwareMask) << s_cmmFirmwareBit;
  module |= (position & s_cmmPositionMask) << s_cmmPositionBit;
  setHeader(s_cmmWordIdVal, version, format, slice, crate, module, 0,
                                                           timeslices);
}

// Store error status trailer

void L1CaloSubBlock::setStatus(uint32_t failingBCN, bool glinkTimeout,
     bool glinkDown, bool daqOverflow, bool bcnMismatch,
     bool glinkProtocol, bool glinkParity)
{
  uint32_t word = 0;
  word |= (failingBCN & s_failingBcnMask) << s_failingBcnBit;
  word |= glinkTimeout  << s_glinkTimeoutBit;
  word |= glinkDown     << s_glinkDownBit;
  word |= daqOverflow   << s_daqOverflowBit;
  word |= bcnMismatch   << s_bcnMismatchBit;
  word |= glinkProtocol << s_glinkProtocolBit;
  word |= glinkParity   << s_glinkParityBit;
  if (word) {
    word |= (wordId()    & s_wordIdMask) << s_wordIdBit;
    word |= (s_statusVal & s_statusMask) << s_statusBit;
    word |= (seqno()     & s_seqnoMask)  << s_seqnoBit;
    word |= (crate()     & s_crateMask)  << s_crateBit;
    word |= (module()    & s_moduleMask) << s_moduleBit;
  }
  m_trailer = word;
}

// Implemented by derived classes

bool L1CaloSubBlock::pack()
{
  return false;
}

bool L1CaloSubBlock::unpack()
{
  return false;
}

// Packing utilities

// Return the minimum number of bits needed for given data

int L1CaloSubBlock::minBits(uint32_t datum)
{
  const int maxBits = 32;
  int nbits = maxBits;
  for (int i = 0; i < maxBits; ++i) {
    if ( !(datum >> i)) {
      nbits = i;
      break;
    }
  }
  return nbits;
}

// Pack given data into given number of bits

void L1CaloSubBlock::packer(uint32_t datum, int nbits)
{
  if (nbits > 0) {
    uint32_t mask = 0x1;
    for (int i = 1; i < nbits; ++i) mask |= (mask << 1);
    m_bitword |= (datum & mask) << m_currentBit;
    m_currentBit += nbits;
    if (m_currentBit >= m_maxBits) {
      m_bitword &= m_maxMask;
      m_data.push_back(m_bitword);
      int bitsLeft = m_currentBit - m_maxBits;
      if (bitsLeft > 0) {
        m_bitword = (datum & mask) >> (nbits - bitsLeft);
	m_currentBit = bitsLeft;
      } else {
        m_bitword = 0;
	m_currentBit = 0;
      }
    }
  }
}

// Flush the current data word padded with zeros

void L1CaloSubBlock::packerFlush()
{
  if (m_currentBit > 0) {
    m_bitword &= m_maxMask;
    m_data.push_back(m_bitword);
    m_bitword = 0;
    m_currentBit = 0;
  }
}

// Unpack given number of bits of data

uint32_t L1CaloSubBlock::unpacker(int nbits)
{
  uint32_t word = 0;
  if (nbits > 0) {
    if (m_dataPos == m_dataPosEnd) {
      m_unpackerFlag = false;
      return 0;
    }
    int nbitsDone = nbits;
    if (nbitsDone > m_maxBits - m_currentBit) {
      nbitsDone = m_maxBits - m_currentBit;
    }
    uint32_t mask = 0x1;
    for (int i = 1; i < nbitsDone; ++i) mask |= (mask << 1);
    word = (m_bitword >> m_currentBit) & mask;
    m_currentBit += nbits;
    if (m_currentBit >= m_maxBits) {
      m_bitword = 0;
      if (m_dataPos != m_dataPosEnd) {
        ++m_dataPos;
        if (m_dataPos != m_dataPosEnd) {
          m_bitword = *m_dataPos;
        }
      }
      m_currentBit = 0;
      int bitsLeft = nbits - nbitsDone;
      if (bitsLeft > 0) {
        if (m_dataPos == m_dataPosEnd) {
          m_unpackerFlag = false;
          return word;
        }
	mask = 0x1;
	for (int i = 1; i < bitsLeft; ++i) mask |= (mask << 1);
	word |= (m_bitword & mask) << nbitsDone;
	m_currentBit = bitsLeft;
      }
    }
  }
  return word;
}

// Initialise unpacker

void L1CaloSubBlock::unpackerInit()
{
  m_bitword = 0;
  m_currentBit = 0;
  m_unpackerFlag = true;
  m_dataPos = m_data.begin();
  m_dataPosEnd = m_data.end();
  if (m_dataPos != m_dataPosEnd) m_bitword = *m_dataPos;
}

// Static function to return PPM error marker for seqno field

int L1CaloSubBlock::errorMarker()
{
  return s_errorMarker;
}

// Static function to determine word type

L1CaloSubBlock::SubBlockWordType L1CaloSubBlock::wordType(uint32_t word)
{
  SubBlockWordType type = DATA_WORD;
  if (((word >> s_headerBit) & s_headerMask) == s_headerVal) {
    if (((word >> s_statusBit) & s_statusMask) == s_statusVal) {
      type = ERROR_STATUS;
    } else if (((word >> s_seqnoBit) & s_seqnoMask) == s_errorMarker) {
      type = ERROR_HEADER;
    } else type = DATA_HEADER;
  }
  return type;
}

// Static function to determine header type

L1CaloSubBlock::SubBlockHeadType L1CaloSubBlock::headerType(uint32_t word)
{
  SubBlockHeadType type = PPM_JEM_CPM;
  if (((word >> s_wordIdBit) & s_wordIdMask) == s_cmmWordIdVal) {
    type = CMM;
  }
  return type;
}

// Static function to determine CMM type

L1CaloSubBlock::CmmFirmwareCode L1CaloSubBlock::cmmType(uint32_t word)
{
  CmmFirmwareCode type;
  int module = (word   >> s_moduleBit)      & s_moduleMask;
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

// Store header data

void L1CaloSubBlock::setHeader(int wordId, int version, int format, int seqno,
                               int crate, int module, int slices2, int slices1)
{
  uint32_t word = 0;
  word |= (wordId  & s_wordIdMask)  << s_wordIdBit;
  word |= (version & s_versionMask) << s_versionBit;
  word |= (format  & s_formatMask)  << s_formatBit;
  word |= (seqno   & s_seqnoMask)   << s_seqnoBit;
  word |= (crate   & s_crateMask)   << s_crateBit;
  word |= (module  & s_moduleMask)  << s_moduleBit;
  word |= (slices2 & s_slices2Mask) << s_slices2Bit;
  word |= (slices1 & s_slices1Mask) << s_slices1Bit;
  m_header = word;
}
