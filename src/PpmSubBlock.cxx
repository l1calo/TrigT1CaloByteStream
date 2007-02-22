
#include "TrigT1CaloByteStream/PpmCompressionV00.h"
#include "TrigT1CaloByteStream/PpmCompressionV01.h"
#include "TrigT1CaloByteStream/PpmCrateMappings.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

// Constant definitions

const uint32_t PpmSubBlock::s_wordIdVal;
const int      PpmSubBlock::s_errorMarker;

const int      PpmSubBlock::s_wordLen;
const int      PpmSubBlock::s_lutBit;
const int      PpmSubBlock::s_bcidLutBit;
const int      PpmSubBlock::s_fadcBit;
const int      PpmSubBlock::s_bcidFadcBit;
const uint32_t PpmSubBlock::s_lutMask;
const uint32_t PpmSubBlock::s_bcidLutMask;
const uint32_t PpmSubBlock::s_fadcMask;
const uint32_t PpmSubBlock::s_bcidFadcMask;

const int      PpmSubBlock::s_glinkPins;
const int      PpmSubBlock::s_asicChannels;
const int      PpmSubBlock::s_dataBits;
const int      PpmSubBlock::s_errorBits;
const int      PpmSubBlock::s_bunchCrossingBits;

const uint32_t PpmSubBlock::s_errorMask;
const int      PpmSubBlock::s_glinkPinParityBit;
const int      PpmSubBlock::s_fpgaCorruptBit;
const int      PpmSubBlock::s_bunchMismatchBit;
const int      PpmSubBlock::s_eventMismatchBit;
const int      PpmSubBlock::s_asicFullBit;
const int      PpmSubBlock::s_timeoutBit;
const int      PpmSubBlock::s_mcmAbsentBit;
const int      PpmSubBlock::s_channelDisabledBit;

PpmSubBlock::PpmSubBlock() : m_globalError(0), m_globalDone(false),
                             m_lutOffset(-1), m_fadcOffset(-1),
			     m_pedestal(10)
{
}

PpmSubBlock::~PpmSubBlock()
{
}

// Clear all data

void PpmSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_globalError   = 0;
  m_globalDone    = false;
  m_lutOffset     = -1;
  m_fadcOffset    = -1;
  m_datamap.clear();
  m_errormap.clear();
}

// Store PPM header

void PpmSubBlock::setPpmHeader(int version, int format, int seqno, int crate,
                               int module, int slicesFadc, int slicesLut)
{
  setHeader(s_wordIdVal, version, format, seqno, crate, module,
                                                 slicesFadc, slicesLut);
}

// Store PPM error block header

void PpmSubBlock::setPpmErrorHeader(int version, int format, int crate,
                                    int module, int slicesFadc, int slicesLut)
{
  setHeader(s_wordIdVal, version, format, s_errorMarker, crate, module,
                                                 slicesFadc, slicesLut);
}

// Return the number of FADC slices

int PpmSubBlock::slicesFadc() const
{
  int slices = slices2();
  if (slices == 0 && format() == NEUTRAL) {
    slices = dataWords()/(s_asicChannels*s_dataBits) - slicesLut();
  }
  return slices;
}

// Return the number of LUT slices

int PpmSubBlock::slicesLut() const
{
  int slices = slices1();
  if (slices == 0 && format() == NEUTRAL) slices = 1;
  return slices;
}

// Store PPM data for later packing

void PpmSubBlock::fillPpmData(int chan, const std::vector<int>& lut,
                                        const std::vector<int>& fadc,
				        const std::vector<int>& bcidLut,
				        const std::vector<int>& bcidFadc)
{
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  int slices = sliceL + sliceF;
  int chanPerSubBlock = channelsPerSubBlock();
  int dataSize = m_datamap.size();
  if (dataSize == 0) {
    dataSize = slices * chanPerSubBlock;
    m_datamap.resize(dataSize);
  }
  int offset = (chan % chanPerSubBlock) * slices;
  if (offset + slices <= dataSize) {
    for (int pos = 0; pos < sliceL; ++pos) {
      uint32_t datum = (lut[pos] & s_lutMask) << s_lutBit;
      datum |= (bcidLut[pos] & s_bcidLutMask) << s_bcidLutBit;
      m_datamap[offset + pos] = datum;
    }
    offset += sliceL;
    for (int pos = 0; pos < sliceF; ++pos) {
      uint32_t datum = (fadc[pos] & s_fadcMask) << s_fadcBit;
      datum |= (bcidFadc[pos] & s_bcidFadcMask) << s_bcidFadcBit;
      m_datamap[offset + pos] = datum;
    }
  }
}

// Return unpacked data for given channel

void PpmSubBlock::ppmData(int chan, std::vector<int>& lut,
                                    std::vector<int>& fadc,
				    std::vector<int>& bcidLut,
				    std::vector<int>& bcidFadc) const
{
  lut.clear();
  fadc.clear();
  bcidLut.clear();
  bcidFadc.clear();
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  int beg = (chan % channelsPerSubBlock()) * (sliceL + sliceF);
  int end = beg + sliceL;
  if (size_t(end + sliceF) <= m_datamap.size()) {
    for (int pos = beg; pos < end; ++pos) {
      uint32_t word = m_datamap[pos];
      lut.push_back((word >> s_lutBit) & s_lutMask);
      bcidLut.push_back((word >> s_bcidLutBit) & s_bcidLutMask);
    }
    beg += sliceL;
    end += sliceF;
    for (int pos = beg; pos < end; ++pos) {
      uint32_t word = m_datamap[pos];
      fadc.push_back((word >> s_fadcBit) & s_fadcMask);
      bcidFadc.push_back((word >> s_bcidFadcBit) & s_bcidFadcMask);
    }
  } else {
    lut.resize(sliceL);
    fadc.resize(sliceF);
    bcidLut.resize(sliceL);
    bcidFadc.resize(sliceF);
  }
}

// Store an error word corresponding to a data channel

void PpmSubBlock::fillPpmError(int chan, int errorWord)
{
  if (m_errormap.empty()) m_errormap.resize(s_glinkPins);
  // Expand one ASIC channel disabled bit to four
  uint32_t chanDisabled = (errorWord & 0x1) << asic(chan);
  m_errormap[pin(chan)] |= (((errorWord >> 1) << s_asicChannels)
                                              | chanDisabled) & s_errorMask;
}

// Store an error word corresponding to a G-Link pin

void PpmSubBlock::fillPpmPinError(int pin, int errorWord)
{
  if (m_errormap.empty()) m_errormap.resize(s_glinkPins);
  m_errormap[pin] = errorWord & s_errorMask;
}

// Return the error word for a data channel

int PpmSubBlock::ppmError(int chan) const
{
  int err = 0;
  if ( !m_errormap.empty()) {
    // Replace the four ASIC channel disabled bits with just the one
    // corresponding to the data channel
    err = (((m_errormap[pin(chan)] & s_errorMask) >> s_asicChannels) << 1)
                                                  | channelDisabled(chan);
  }
  return err;
}

// Return the error word for a G-Link pin

int PpmSubBlock::ppmPinError(int pin) const
{
  int err = 0;
  if ( !m_errormap.empty()) err = m_errormap[pin] & s_errorMask;
  return err;
}

// Return global error bit

bool PpmSubBlock::errorBit(int bit) const
{
  if ( ! m_globalDone) {
    std::vector<uint32_t>::const_iterator pos;
    for (pos = m_errormap.begin(); pos != m_errormap.end(); ++pos) {
      m_globalError |= *pos;
    }
    m_globalDone  = true;
  }
  return m_globalError & (0x1 << bit);
}

// Packing/Unpacking routines

bool PpmSubBlock::pack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case NEUTRAL:
	  rc = packNeutral();
	  break;
        case UNCOMPRESSED:
	  switch (seqno()) {
	    case s_errorMarker:
	      rc = packUncompressedErrors();
	      break;
            default:
	      rc = packUncompressedData();
	      break;
          }
	  break;
        case COMPRESSED:
	  switch (seqno()) {
	    case 0:
	      rc = PpmCompressionV00::pack(*this);
	      break;
	    case 1:
	      rc = PpmCompressionV01::pack(*this);
	      break;
	    default:
	      break;
          }
	  break;
        case SUPERCOMPRESSED:
	  rc = packSuperCompressed();
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

bool PpmSubBlock::unpack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case NEUTRAL:
	  rc = unpackNeutral();
	  break;
        case UNCOMPRESSED:
	  switch (seqno()) {
	    case s_errorMarker:
	      rc = unpackUncompressedErrors();
	      break;
            default:
	      rc = unpackUncompressedData();
	      break;
          }
	  break;
        case COMPRESSED:
	  switch (seqno()) {
	    case 0:
	      rc = PpmCompressionV00::unpack(*this);
	      break;
	    case 1:
	      rc = PpmCompressionV01::unpack(*this);
	      break;
	    default:
	      break;
          }
	  break;
        case SUPERCOMPRESSED:
	  rc = unpackSuperCompressed();
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

// Pack neutral data

bool PpmSubBlock::packNeutral()
{
  int slices   = slicesLut() + slicesFadc();
  int channels = channelsPerSubBlock();
  if (m_datamap.empty()) m_datamap.resize(slices * channels);
  // Bunch crossing number
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    uint32_t bc = 0;
    if (pin < s_bunchCrossingBits) bc = (bunchCrossing() >> pin) & 0x1;
    packerNeutral(pin, bc, 1);
  }
  // Data
  std::vector<uint32_t>::const_iterator pos = m_datamap.begin();
  for (int asic = 0; asic < s_asicChannels; ++asic) {
    for (int pin = 0; pin < s_glinkPins; ++pin) {
      for (int sl = 0; sl < slices; ++sl) {
        packerNeutral(pin, *pos, s_dataBits);
	++pos;
      }
    }
  }
  // Errors, including GP
  if (m_errormap.empty()) m_errormap.resize(s_glinkPins);
  pos = m_errormap.begin();
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    packerNeutral(pin, *pos, s_errorBits);
    packerNeutralParity(pin);
    ++pos;
  }
  return true;
}

// Pack super-compressed data

bool PpmSubBlock::packSuperCompressed()
{
  return false;
}

// Pack uncompressed data

bool PpmSubBlock::packUncompressedData()
{
  int slices   = slicesLut() + slicesFadc();
  int channels = channelsPerSubBlock();
  if (m_datamap.empty()) m_datamap.resize(slices * channels);
  for (int sl = 0; sl < slices; ++sl) {
    for (int chan = 0; chan < channels; ++chan) {
      packer(m_datamap[sl + chan*slices], s_wordLen);
    }
  }
  packerFlush();
  return true;
}

// Pack uncompressed error data

bool PpmSubBlock::packUncompressedErrors()
{
  if (m_errormap.empty()) m_errormap.resize(s_glinkPins);
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    packer(m_errormap[pin], s_wordLen);
  }
  packerFlush();
  return true;
}

// Unpack neutral data

bool PpmSubBlock::unpackNeutral()
{
  int slices = slicesLut() + slicesFadc();
  m_datamap.clear();
  // Bunch Crossing number
  int bunchCrossing = 0;
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    int bc = unpackerNeutral(pin, 1);
    if (pin < s_bunchCrossingBits) bunchCrossing |= bc << pin;
  }
  setBunchCrossing(bunchCrossing);
  // Data
  for (int asic = 0; asic < s_asicChannels; ++asic) {
    for (int pin = 0; pin < s_glinkPins; ++pin) {
      for (int sl = 0; sl < slices; ++sl) {
        m_datamap.push_back(unpackerNeutral(pin, s_dataBits));
      }
    }
  }
  bool rc = unpackerSuccess();
  // Errors
  m_errormap.clear();
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    uint32_t error = unpackerNeutral(pin, s_errorBits);
    error |= unpackerNeutralParityError(pin) << s_errorBits;
    m_errormap.push_back(error);
  }
  return rc;
}

// Unpack super-compressed data

bool PpmSubBlock::unpackSuperCompressed()
{
  return false;
}

// Unpack uncompressed data

bool PpmSubBlock::unpackUncompressedData()
{
  int slices = slicesLut() + slicesFadc();
  int channels = channelsPerSubBlock();
  m_datamap.resize(slices * channels);
  unpackerInit();
  for (int sl = 0; sl < slices; ++sl) {
    for (int chan = 0; chan < channels; ++chan) {
      m_datamap[sl + chan*slices] = unpacker(s_wordLen);
    }
  }
  return unpackerSuccess();
}

// Unpack uncompressed error data

bool PpmSubBlock::unpackUncompressedErrors()
{
  unpackerInit();
  m_errormap.clear();
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    m_errormap.push_back(unpacker(s_wordLen));
  }
  return unpackerSuccess();
}

// Return the number of channels per sub-block

int PpmSubBlock::channelsPerSubBlock(int version, int format)
{
  int chan = 0;
  switch (version) {
    case 1:
      switch (format) {
        case UNCOMPRESSED:
	  chan = PpmCrateMappings::channels()/s_asicChannels;
	  break;
        case NEUTRAL:
        case COMPRESSED:
        case SUPERCOMPRESSED:
	  chan = PpmCrateMappings::channels();
	  break;
        default:
	  break;
      }
      break;
    default:
      break;
  }
  return chan;
}

int PpmSubBlock::channelsPerSubBlock() const
{
  return channelsPerSubBlock(version(), format());
}

// Check if a header word is for an error block

bool PpmSubBlock::errorBlock(uint32_t word)
{
  bool rc = false;
  if (format(word) == UNCOMPRESSED && 
       seqno(word) == s_errorMarker) rc = true;
  return rc;
}
