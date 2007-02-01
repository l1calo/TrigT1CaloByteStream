
#include "TrigT1CaloByteStream/CpmCrateMappings.h"
#include "TrigT1CaloByteStream/CpmSubBlock.h"

// Constant definitions

const int      CpmSubBlock::s_wordIdVal;

const int      CpmSubBlock::s_wordLength;

const int      CpmSubBlock::s_ttDataABit;
const int      CpmSubBlock::s_ttDataBBit;
const int      CpmSubBlock::s_parityABit;
const int      CpmSubBlock::s_parityBBit;
const int      CpmSubBlock::s_linkDownABit;
const int      CpmSubBlock::s_linkDownBBit;
const int      CpmSubBlock::s_pairBit;
const int      CpmSubBlock::s_fpgaBit;
const int      CpmSubBlock::s_dataIdBit;
const int      CpmSubBlock::s_ttWordId;
const uint32_t CpmSubBlock::s_ttDataMask;
const uint32_t CpmSubBlock::s_pairPinMask;
const uint32_t CpmSubBlock::s_dataIdMask;

const int      CpmSubBlock::s_indicatorBit;
const int      CpmSubBlock::s_threshWordId;
const uint32_t CpmSubBlock::s_threshMask;

const int      CpmSubBlock::s_pairsPerPin;
const int      CpmSubBlock::s_wordsPerPin;
const int      CpmSubBlock::s_ttBits;
const int      CpmSubBlock::s_errBits;
const int      CpmSubBlock::s_hitBits;
const int      CpmSubBlock::s_hitWords;
const int      CpmSubBlock::s_glinkPins;
const int      CpmSubBlock::s_glinkBitsPerSlice;


CpmSubBlock::CpmSubBlock()
{
  m_channels = CpmCrateMappings::channels();
}

CpmSubBlock::~CpmSubBlock()
{
}

// Clear all data

void CpmSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_ttData.clear();
  m_hitData.clear();
}

// Store CPM header

void CpmSubBlock::setCpmHeader(int version, int format, int slice, int crate,
                               int module, int timeslices)
{
  setHeader(s_wordIdVal, version, format, slice, crate, module, 0, timeslices);
}

// Store trigger tower data

void CpmSubBlock::fillTowerData(int slice, int channel, int em, int had,
                                                        int emErr, int hadErr)
{
  if (channel < m_channels && (em || emErr || had || hadErr)) {
    resize(m_ttData, m_channels);
    int dat = em;
    int err = emErr;
    for (int pinOffset = 0; pinOffset < 2; ++pinOffset) {
      if (dat || err) {
        int pin  = 2 * (channel/s_wordsPerPin) + pinOffset;
        int pair = (channel % s_wordsPerPin) / 2;
        int pos  = pin * s_pairsPerPin + pair;
        int ix   = index(slice) * m_channels + pos;
        uint32_t word = m_ttData[ix];
        if (channel % 2 == 0) {
          word |= (dat & s_ttDataMask) << s_ttDataABit;
	  word |= (err & 0x1)          << s_parityABit;
	  word |= ((err >> 1) & 0x1)   << s_linkDownABit;
        } else {
          word |= (dat & s_ttDataMask) << s_ttDataBBit;
	  word |= (err & 0x1)          << s_parityBBit;
	  word |= ((err >> 1) & 0x1)   << s_linkDownBBit;
        }
        word |= pair       << s_pairBit;
        word |= pin        << s_fpgaBit;
        word |= s_ttWordId << s_dataIdBit;
        m_ttData[ix] = word;
      }
      dat = had;
      err = hadErr;
    }
  }
}

// Store hit counts

void CpmSubBlock::setHits(int slice, unsigned int hit0, unsigned int hit1)
{
  if (hit0 || hit1) {
    resize(m_hitData, 2);
    int ix = index(slice)*2;
    unsigned int hits = hit0;
    for (int indicator = 0; indicator < 2; ++indicator) {
      if (hits) {
        uint32_t word = (hits & s_threshMask);
        word |= indicator      << s_indicatorBit;
        word |= s_threshWordId << s_dataIdBit;
        m_hitData[ix + indicator] = word;
      }
      hits = hit1;
    }
  }
}

// Return Em data for given channel

int CpmSubBlock::emData(int slice, int channel) const
{
  return data(slice, channel, 0);
}

// Return Had data for given channel

int CpmSubBlock::hadData(int slice, int channel) const
{
  return data(slice, channel, 1);
}

// Return Em error for given channel

int CpmSubBlock::emError(int slice, int channel) const
{
  return error(slice, channel, 0);
}

// Return Had error for given channel

int CpmSubBlock::hadError(int slice, int channel) const
{
  return error(slice, channel, 1);
}

// Return first hit counts word

unsigned int CpmSubBlock::hits0(int slice) const
{
  return hits(slice, 0);
}

// Return second hit counts word

unsigned int CpmSubBlock::hits1(int slice) const
{
  return hits(slice, 1);
}

// Return number of timeslices

int CpmSubBlock::timeslices() const
{
  int slices = slices1();
  if (slices == 0 && format() == NEUTRAL) {
    slices = dataWords() / s_glinkBitsPerSlice;
  }
  return slices;
}

// Packing/Unpacking routines

bool CpmSubBlock::pack()
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

bool CpmSubBlock::unpack()
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

// Return data for given channel and pin offset

int CpmSubBlock::data(int slice, int channel, int offset) const
{
  int dat = 0;
  if (slice >= 0 && slice < timeslices() &&
      channel >= 0 && channel < m_channels && !m_ttData.empty()) {
    int pin  = 2 * (channel/s_wordsPerPin) + offset;
    int pair = (channel % s_wordsPerPin) / 2;
    int pos  = pin * s_pairsPerPin + pair;
    int ix   = index(slice) * m_channels + pos;
    uint32_t word = m_ttData[ix];
    if (channel % 2 == 0) {
           dat = (word >> s_ttDataABit) & s_ttDataMask;
    } else dat = (word >> s_ttDataBBit) & s_ttDataMask;
  }
  return dat;
}

// Return error for given channel and pin offset

int CpmSubBlock::error(int slice, int channel, int offset) const
{
  int err = 0;
  if (slice >= 0 && slice < timeslices() &&
      channel >= 0 && channel < m_channels && !m_ttData.empty()) {
    int pin  = 2 * (channel/s_wordsPerPin) + offset;
    int pair = (channel % s_wordsPerPin) / 2;
    int pos  = pin * s_pairsPerPin + pair;
    int ix   = index(slice) * m_channels + pos;
    uint32_t word = m_ttData[ix];
    if (channel % 2 == 0) {
      err  =  (word >> s_parityABit)   & 0x1;
      err |= ((word >> s_linkDownABit) & 0x1) << 1;
    } else {
      err  =  (word >> s_parityBBit)   & 0x1;
      err |= ((word >> s_linkDownBBit) & 0x1) << 1;
    }
  }
  return err;
}

// Return hit counts with given offset

unsigned int CpmSubBlock::hits(int slice, int offset) const
{
  unsigned int hit = 0;
  if (slice >= 0 && slice < timeslices() && !m_hitData.empty()) {
    hit = m_hitData[index(slice)*2 + offset] & s_threshMask;
  }
  return hit;
}

// Return data index appropriate to format

int CpmSubBlock::index(int slice) const
{
  int ix = 0;
  if (format() == NEUTRAL) ix = slice;
  return ix;
}

// Resize a data vector according to format

void CpmSubBlock::resize(std::vector<uint32_t>& vec, int channels)
{
  if (vec.empty()) {
    int size = channels;
    if (format() == NEUTRAL) size *= timeslices();
    vec.resize(size);
  }
}

// Pack neutral data

bool CpmSubBlock::packNeutral()
{
  resize(m_ttData, m_channels);
  resize(m_hitData, 2);
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    unsigned int hit0 = hits0(slice);
    unsigned int hit1 = hits1(slice);
    for (int pin = 0; pin < s_glinkPins; ++pin) {
      // Trigger tower data
      for (int pair = 0; pair < s_pairsPerPin; ++pair) {
        for (int i = 0; i < 2; ++i) {
          int channel = s_wordsPerPin*(pin/2) + 2*pair + i;
	  if ((pin & 0x1)) { // Odd pins Had, even Em
	    packerNeutral(pin, hadData(slice,  channel), s_ttBits);
	    packerNeutral(pin, hadError(slice, channel), s_errBits);
          } else {
	    packerNeutral(pin, emData(slice,  channel), s_ttBits);
	    packerNeutral(pin, emError(slice, channel), s_errBits);
	  }
        }
      }
      // Hits and Bunch Crossing number
      if (pin < s_hitWords) {
        packerNeutral(pin, hit0 >> pin*s_hitBits, s_hitBits);
      } else if (pin < 2*s_hitWords) {
        packerNeutral(pin, hit1 >> (pin - s_hitWords)*s_hitBits, s_hitBits);
      } else {
        packerNeutral(pin, bunchCrossing() >> (pin - 2*s_hitWords)*s_hitBits,
	                                                         s_hitBits);
      }
      // G-Link parity
      packerNeutralParity(pin);
    }
  }
  return true;
}

// Pack uncompressed data

bool CpmSubBlock::packUncompressed()
{
  // Trigger tower data
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_ttData.begin(); pos != m_ttData.end(); ++pos) {
    if (*pos) packer(*pos, s_wordLength);
  }
        
  // Hits data
  for (pos = m_hitData.begin(); pos != m_hitData.end(); ++pos) {
    if (*pos) packer(*pos, s_wordLength);
  }
  packerFlush();
  return true;
}

// Unpack neutral data

bool CpmSubBlock::unpackNeutral()
{
  resize(m_ttData, m_channels);
  resize(m_hitData, 2);
  int slices = timeslices();
  for (int slice = 0; slice < slices; ++slice) {
    unsigned int hit0 = 0;
    unsigned int hit1 = 0;
    int bunchCrossing = 0;
    for (int pin = 0; pin < s_glinkPins; ++pin) {
      // Trigger tower data
      for (int pair = 0; pair < s_pairsPerPin; ++pair) {
	for (int i = 0; i < 2; ++i) {
          int channel = s_wordsPerPin*(pin/2) + 2*pair + i;
	  int em     = 0;
	  int had    = 0;
	  int emErr  = 0;
	  int hadErr = 0;
	  if ((pin & 0x1)) { // Odd pins Had, even Em
	    em     = emData(slice, channel);
	    had    = unpackerNeutral(pin, s_ttBits);
	    emErr  = emError(slice, channel);
	    hadErr = unpackerNeutral(pin, s_errBits);
	  } else {
	    em     = unpackerNeutral(pin, s_ttBits);
	    had    = hadData(slice, channel);
	    emErr  = unpackerNeutral(pin, s_errBits);
	    hadErr = hadError(slice, channel);
          }
	  fillTowerData(slice, channel, em, had, emErr, hadErr);
        }
      }
      // Hits and Bunch Crossing number
      if (pin < s_hitWords) {
        hit0 |= unpackerNeutral(pin, s_hitBits) << pin*s_hitBits;
      } else if (pin < 2*s_hitWords) {
        hit1 |= unpackerNeutral(pin, s_hitBits) << (pin - s_hitWords)*s_hitBits;
      } else {
	bunchCrossing |= unpackerNeutral(pin, s_hitBits)
	                                << (pin - 2*s_hitWords)*s_hitBits;
      }
      // G-Link parity error
      unpackerNeutralParityError(pin);
    }
    setHits(slice, hit0, hit1);
    setBunchCrossing(bunchCrossing);
  }
  return unpackerSuccess();
}

// Unpack uncompressed data

bool CpmSubBlock::unpackUncompressed()
{
  resize(m_ttData, m_channels);
  resize(m_hitData, 2);
  unpackerInit();
  uint32_t word = unpacker(s_wordLength);
  while (unpackerSuccess()) {
    int id = dataId(word);
    // Trigger tower data
    if (id == s_ttWordId) {
      int ix = (word >> s_pairBit) & s_pairPinMask;
      if (ix < m_channels) m_ttData[ix] = word;
      else return false;
    // Hits
    } else if (id == s_threshWordId) {
      int indicator = (word >> s_indicatorBit) & 0x1;
      m_hitData[indicator] = word;
    } else return false;
    word = unpacker(s_wordLength);
  }
  return true;
}
