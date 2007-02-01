
#include "TrigT1CaloByteStream/CmmCpSubBlock.h"

// Constant definitions

const int      CmmCpSubBlock::s_wordLength;

const int      CmmCpSubBlock::s_threshBit;
const int      CmmCpSubBlock::s_threshErrorBit;
const int      CmmCpSubBlock::s_sourceIdBit;
const int      CmmCpSubBlock::s_dataWordIdBit;
const int      CmmCpSubBlock::s_dataWordId;
const int      CmmCpSubBlock::s_maxHits;
const uint32_t CmmCpSubBlock::s_threshMask;
const uint32_t CmmCpSubBlock::s_errorMask;
const uint32_t CmmCpSubBlock::s_sourceIdMask;

const int      CmmCpSubBlock::s_hitsBits;
const int      CmmCpSubBlock::s_hitsErrorBits;
const int      CmmCpSubBlock::s_bunchCrossingBits;
const int      CmmCpSubBlock::s_paddingBits;


CmmCpSubBlock::CmmCpSubBlock()
{
}

CmmCpSubBlock::~CmmCpSubBlock()
{
}

// Clear all data

void CmmCpSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_hitsData.clear();
}

// Return hit counts for given CPM or source ID

unsigned int CmmCpSubBlock::hits(int slice, int source) const
{
  --source;
  unsigned int hits = 0;
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && !m_hitsData.empty()) {
    hits = (m_hitsData[index(slice, source)] >> s_threshBit) & s_threshMask;
  }
  return hits;
}

// Return hit error for given CPM or source ID

int CmmCpSubBlock::hitsError(int slice, int source) const
{
  --source;
  int error = 0;
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && !m_hitsData.empty()) {
    error = (m_hitsData[index(slice, source)] >> s_threshErrorBit)
                                                               & s_errorMask;
  }
  return error;
}

// Store hit counts for given CPM or source ID

void CmmCpSubBlock::setHits(int slice, int source, unsigned int hits,
                                                            int error)
{
  --source;
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && (hits || error)) {
    resize();
    uint32_t word = m_hitsData[index(slice, source)];
    word |= (hits & s_threshMask)     << s_threshBit;
    word |= (error & s_errorMask)     << s_threshErrorBit;
    word |= (source & s_sourceIdMask) << s_sourceIdBit;
    word |= (s_dataWordId)            << s_dataWordIdBit;
    m_hitsData[index(slice, source)] = word;
  }
}

// Packing/Unpacking routines

bool CmmCpSubBlock::pack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case NEUTRAL:
	  rc = packNeutral();
	  break;
        case UNCOMPRESSED:
	  rc = packUncompressed();
	  break;
        default:
	  break;
      }
      break;
    default:
      break;
  }
  return rc;
}

bool CmmCpSubBlock::unpack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case NEUTRAL:
	  rc = unpackNeutral();
	  break;
        case UNCOMPRESSED:
	  rc = unpackUncompressed();
	  break;
        default:
	  break;
      }
      break;
    default:
      break;
  }
  return rc;
}

// Return data index appropriate to format

int CmmCpSubBlock::index(int slice, int source) const
{
  int ix = source;
  if (format() == NEUTRAL) ix += slice * s_maxHits;
  return ix;
}

// Resize the hits vector according to format

void CmmCpSubBlock::resize()
{
  if (m_hitsData.empty()) {
    int size = s_maxHits;
    if (format() == NEUTRAL) size *= timeslices();
    m_hitsData.resize(size);
  }
}

// Pack neutral data

bool CmmCpSubBlock::packNeutral()
{
  resize();
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    for (int source = 1; source < MAX_SOURCE_ID; ++source) {
      int pin = source - 1;
      // CPM hits; remote(3), local and total hits; parity error
      packerNeutral(pin, hits(slice, source), s_hitsBits);
      packerNeutral(pin, hitsError(slice, source), s_hitsErrorBits);
      // Bunch crossing number; Fifo overflow
      if (pin < s_bunchCrossingBits) {
        packerNeutral(pin, bunchCrossing() >> pin, 1);
      } else packerNeutral(pin, 0, 1);
      // Padding
      packerNeutral(pin, 0, s_paddingBits);
      // G-Link parity
      packerNeutralParity(pin);
    }
  }
  return true;
}

// Pack uncompressed data

bool CmmCpSubBlock::packUncompressed()
{
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_hitsData.begin(); pos != m_hitsData.end(); ++pos) {
    if (*pos) packer(*pos, s_wordLength);
  }
  packerFlush();
  return true;
}

// Unpack neutral data

bool CmmCpSubBlock::unpackNeutral()
{
  resize();
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    int bunchCrossing = 0;
    for (int source = 1; source < MAX_SOURCE_ID; ++source) {
      int pin = source - 1;
      // CPM hits; remote(3), local and total hits; parity error
      unsigned int hits = unpackerNeutral(pin, s_hitsBits);
      int error = unpackerNeutral(pin, s_hitsErrorBits);
      setHits(slice, source, hits, error);
      // Bunch crossing number; Fifo overflow
      if (pin < s_bunchCrossingBits) {
        bunchCrossing |= unpackerNeutral(pin, 1) << pin;
      } else unpackerNeutral(pin, 1);
      // Padding
      unpackerNeutral(pin, s_paddingBits);
      // G-Link parity errors
      unpackerNeutralParityError(pin);
    }
    setBunchCrossing(bunchCrossing);
  }
  return unpackerSuccess();;
}

// Unpack uncompressed data

bool CmmCpSubBlock::unpackUncompressed()
{
  resize();
  unpackerInit();
  uint32_t word = unpacker(s_wordLength);
  while (unpackerSuccess()) {
    int source = sourceId(word);
    if (source < s_maxHits) m_hitsData[source] = word;
    else return false;
    word = unpacker(s_wordLength);
  }
  return true;
}
