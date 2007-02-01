
#include "TrigT1CaloByteStream/CmmJetSubBlock.h"

// Constant definitions

const int      CmmJetSubBlock::s_wordLength;

const int      CmmJetSubBlock::s_threshBit;
const int      CmmJetSubBlock::s_fwdErrorBit;
const int      CmmJetSubBlock::s_etMapBit;
const int      CmmJetSubBlock::s_threshErrorBit;
const int      CmmJetSubBlock::s_sourceIdBit;
const int      CmmJetSubBlock::s_dataWordIdBit;
const int      CmmJetSubBlock::s_dataWordId;
const int      CmmJetSubBlock::s_maxHits;
const uint32_t CmmJetSubBlock::s_threshMask;
const uint32_t CmmJetSubBlock::s_fwdMask;
const uint32_t CmmJetSubBlock::s_etMapMask;
const uint32_t CmmJetSubBlock::s_errorMask;
const uint32_t CmmJetSubBlock::s_sourceIdMask;

const int      CmmJetSubBlock::s_jetHitsBits;
const int      CmmJetSubBlock::s_jetHitsErrorBits;
const int      CmmJetSubBlock::s_fwdHitsBits;
const int      CmmJetSubBlock::s_bunchCrossingBits;
const int      CmmJetSubBlock::s_paddingBits;
const int      CmmJetSubBlock::s_rightBit;


CmmJetSubBlock::CmmJetSubBlock()
{
}

CmmJetSubBlock::~CmmJetSubBlock()
{
}

// Clear all data

void CmmJetSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_hitsData.clear();
}

// Return jet hit counts for given jem or source ID

unsigned int CmmJetSubBlock::jetHits(int slice, int source) const
{
  unsigned int hits = 0;
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && !m_hitsData.empty()) {
    if (source <= TOTAL_MAIN) {
      hits = (m_hitsData[index(slice, source)] >> s_threshBit) & s_threshMask;
    } else if (source >= REMOTE_FORWARD) {
      hits = (m_hitsData[index(slice, source)] >> s_threshBit) & s_fwdMask;
    }
  }
  return hits;
}

// Return jet hit error for given jem or source ID

int CmmJetSubBlock::jetHitsError(int slice, int source) const
{
  int error = 0;
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && !m_hitsData.empty()) {
    if (source <= TOTAL_MAIN) {
      error = (m_hitsData[index(slice, source)] >> s_threshErrorBit)
                                                                  & s_errorMask;
    } else if (source == REMOTE_FORWARD) {
      error = (m_hitsData[index(slice, source)] >> s_fwdErrorBit) & s_errorMask;
    }
  }
  return error;
}

// Return jet ET map

unsigned int CmmJetSubBlock::jetEtMap(int slice) const
{
  unsigned int map = 0;
  if (slice >= 0 && slice < timeslices() && !m_hitsData.empty()) {
    map = (m_hitsData[index(slice, TOTAL_FORWARD)] >> s_etMapBit) & s_etMapMask;
  }
  return map;
}

// Store jet hit counts and error for given jem or source ID

void CmmJetSubBlock::setJetHits(int slice, int source, unsigned int hits,
                                                                int error)
{
  if (slice >= 0 && slice < timeslices() &&
      source >= 0 && source < s_maxHits  && (hits || error)) {
    resize();
    uint32_t word = m_hitsData[index(slice, source)];
    if (source <= TOTAL_MAIN) {
      word |= (hits & s_threshMask)     << s_threshBit;
      word |= (error & s_errorMask)     << s_threshErrorBit;
    } else {
      word |= (hits & s_fwdMask)        << s_threshBit;
      word |= (error & s_errorMask)     << s_fwdErrorBit;
    }
    word   |= (source & s_sourceIdMask) << s_sourceIdBit;
    word   |= (s_dataWordId)            << s_dataWordIdBit;
    m_hitsData[index(slice, source)] = word;
  }
}

// Store jet ET map

void CmmJetSubBlock::setJetEtMap(int slice, unsigned int map)
{
  if (slice >= 0 && slice < timeslices() && map) {
    resize();
    uint32_t word = m_hitsData[index(slice, TOTAL_FORWARD)];
    word |= (map & s_etMapMask)              << s_etMapBit;
    word |= (TOTAL_FORWARD & s_sourceIdMask) << s_sourceIdBit;
    word |= (s_dataWordId)                   << s_dataWordIdBit;
    m_hitsData[index(slice, TOTAL_FORWARD)] = word;
  }
}

// Packing/Unpacking routines

bool CmmJetSubBlock::pack()
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

bool CmmJetSubBlock::unpack()
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

int CmmJetSubBlock::index(int slice, int source) const
{
  int ix = source;
  if (format() == NEUTRAL) ix += slice * s_maxHits;
  return ix;
}

// Resize the hits vector according to format

void CmmJetSubBlock::resize()
{
  if (m_hitsData.empty()) {
    int size = s_maxHits;
    if (format() == NEUTRAL) size *= timeslices();
    m_hitsData.resize(size);
  }
}

// Pack neutral data

bool CmmJetSubBlock::packNeutral()
{
  resize();
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    for (int pin = 0; pin <= TOTAL_MAIN; ++pin) {
      // Jem hits; remote, local and total main hits; parity error
      packerNeutral(pin, jetHits(slice, pin), s_jetHitsBits);
      packerNeutral(pin, jetHitsError(slice, pin), s_jetHitsErrorBits);
      // Bunch crossing number; Fifo overflow
      if (pin < s_bunchCrossingBits) {
        packerNeutral(pin, bunchCrossing() >> pin, 1);
      } else packerNeutral(pin, 0, 1);
      // Padding
      if (pin < REMOTE_MAIN) packerNeutral(pin, 0, s_paddingBits);
    }
    // ET Map + padding
    packerNeutral(REMOTE_MAIN, jetEtMap(slice), s_paddingBits);
    // Total forward (left + right)
    packerNeutral(LOCAL_MAIN, jetHits(slice, TOTAL_FORWARD), s_paddingBits);
    packerNeutral(TOTAL_MAIN, jetHits(slice, TOTAL_FORWARD) >> s_rightBit,
                                                             s_paddingBits);
    // Remote + Local forward
    int lastpin = TOTAL_MAIN + 1;
    packerNeutral(lastpin, jetHits(slice, REMOTE_FORWARD), s_fwdHitsBits);
    packerNeutral(lastpin, jetHitsError(slice, REMOTE_FORWARD),
                                                      s_jetHitsErrorBits);
    packerNeutral(lastpin, 0, 1);
    packerNeutral(lastpin, jetHits(slice, LOCAL_FORWARD),  s_fwdHitsBits);
    // G-Link parity
    for (int pin = 0; pin <= lastpin; ++pin) packerNeutralParity(pin);
  }
  return true;
}

// Pack uncompressed data

bool CmmJetSubBlock::packUncompressed()
{
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_hitsData.begin(); pos != m_hitsData.end(); ++pos) {
    if (*pos) packer(*pos, s_wordLength);
  }
  packerFlush();
  return true;
}

// Unpack neutral data

bool CmmJetSubBlock::unpackNeutral()
{
  resize();
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    int bunchCrossing = 0;
    unsigned int hits = 0;
    int error = 0;
    for (int pin = 0; pin <= TOTAL_MAIN; ++pin) {
      // Jem hits; remote, local and total main hits; parity error
      hits  = unpackerNeutral(pin, s_jetHitsBits);
      error = unpackerNeutral(pin, s_jetHitsErrorBits);
      setJetHits(slice, pin, hits, error);
      // Bunch crossing number; Fifo overflow
      if (pin < s_bunchCrossingBits) {
        bunchCrossing |= unpackerNeutral(pin, 1) << pin;
      } else unpackerNeutral(pin, 1);
      // Padding
      if (pin < REMOTE_MAIN) unpackerNeutral(pin, s_paddingBits);
    }
    setBunchCrossing(bunchCrossing);
    // ET Map + padding
    setJetEtMap(slice, unpackerNeutral(REMOTE_MAIN, s_paddingBits));
    // Total forward (left + right)
    hits  = unpackerNeutral(LOCAL_MAIN, s_paddingBits);
    hits |= unpackerNeutral(TOTAL_MAIN, s_paddingBits) << s_rightBit;
    setJetHits(slice, TOTAL_FORWARD, hits, 0);
    // Remote + Local forward
    int lastpin = TOTAL_MAIN + 1;
    hits  = unpackerNeutral(lastpin, s_fwdHitsBits);
    error = unpackerNeutral(lastpin, s_jetHitsErrorBits);
    setJetHits(slice, REMOTE_FORWARD, hits, error);
    unpackerNeutral(lastpin, 1);
    setJetHits(slice, LOCAL_FORWARD,
                      unpackerNeutral(lastpin, s_fwdHitsBits), 0);
    // G-Link parity errors
    for (int pin = 0; pin <= lastpin; ++pin) unpackerNeutralParityError(pin);
  }
  return unpackerSuccess();;
}

// Unpack uncompressed data

bool CmmJetSubBlock::unpackUncompressed()
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
