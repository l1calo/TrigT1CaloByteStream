
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
const int      L1CaloSubBlock::s_slicesFadcBit;
const int      L1CaloSubBlock::s_slicesLutBit;
const uint32_t L1CaloSubBlock::s_wordIdMask;
const uint32_t L1CaloSubBlock::s_versionMask;
const uint32_t L1CaloSubBlock::s_formatMask;
const uint32_t L1CaloSubBlock::s_seqnoMask;
const uint32_t L1CaloSubBlock::s_crateMask;
const uint32_t L1CaloSubBlock::s_moduleMask;
const uint32_t L1CaloSubBlock::s_slicesFadcMask;
const uint32_t L1CaloSubBlock::s_slicesLutMask;

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

L1CaloSubBlock::L1CaloSubBlock() : m_header(0), m_trailer(0), m_wordId(0),
                                   m_bitword(0), m_currentBit(0)
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

// Store header

void L1CaloSubBlock::setHeader(int iversion, int iformat, int iseqno,
                 int icrate, int imodule, int islicesFadc, int islicesLut)
{
  uint32_t word = 0;
  word |= (m_wordId & s_wordIdMask) << s_wordIdBit;
  word |= (iversion & s_versionMask) << s_versionBit;
  word |= (iformat & s_formatMask) << s_formatBit;
  word |= (iseqno & s_seqnoMask) << s_seqnoBit;
  word |= (icrate & s_crateMask) << s_crateBit;
  word |= (imodule & s_moduleMask) << s_moduleBit;
  word |= (islicesFadc & s_slicesFadcMask) << s_slicesFadcBit;
  word |= (islicesLut & s_slicesLutMask) << s_slicesLutBit;
  m_header = word;
}

// Store error status trailer

void L1CaloSubBlock::setStatus(uint32_t ifailingBCN, bool bglinkTimeout,
     bool bglinkDown, bool bdaqOverflow, bool bbcnMismatch,
     bool bglinkProtocol, bool bglinkParity)
{
  uint32_t word = 0;
  if (ifailingBCN || bglinkTimeout || bglinkDown || bdaqOverflow ||
      bbcnMismatch || bglinkProtocol || bglinkParity) {
    word |= (wordId() & s_wordIdMask) << s_wordIdBit;
    word |= (s_statusVal & s_statusMask) << s_statusBit;
    word |= (ifailingBCN & s_failingBcnMask) << s_failingBcnBit;
    word |= (seqno() & s_seqnoMask) << s_seqnoBit;
    word |= (crate() & s_crateMask) << s_crateBit;
    word |= (module() & s_moduleMask) << s_moduleBit;
    word |= bglinkTimeout << s_glinkTimeoutBit;
    word |= bglinkDown << s_glinkDownBit;
    word |= bdaqOverflow << s_daqOverflowBit;
    word |= bbcnMismatch << s_bcnMismatchBit;
    word |= bglinkProtocol << s_glinkProtocolBit;
    word |= bglinkParity << s_glinkParityBit;
  }
  m_trailer = word;
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
