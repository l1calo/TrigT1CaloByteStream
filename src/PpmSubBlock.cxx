
#include "TrigT1CaloByteStream/PpmCrateMappings.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

// Constant definitions

const int      PpmSubBlock::s_wordLen;
const int      PpmSubBlock::s_lutBit;
const int      PpmSubBlock::s_bcidLutBit;
const int      PpmSubBlock::s_fadcBit;
const int      PpmSubBlock::s_bcidFadcBit;
const uint32_t PpmSubBlock::s_lutMask;
const uint32_t PpmSubBlock::s_bcidLutMask;
const uint32_t PpmSubBlock::s_fadcMask;
const uint32_t PpmSubBlock::s_bcidFadcMask;

const int      PpmSubBlock::s_fullWordLen;
const int      PpmSubBlock::s_asicChannels;
const int      PpmSubBlock::s_dataBits;
const int      PpmSubBlock::s_glinkPins;
const int      PpmSubBlock::s_errorBits;

PpmSubBlock::PpmSubBlock()
{
}

PpmSubBlock::~PpmSubBlock()
{
}

// Clear all data

void PpmSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_datamap.clear();
}

// Return reference to compression stats

const std::vector<uint32_t>& PpmSubBlock::compStats() const
{
  return m_compStats;
}

// Return unpacked data for given channel

void PpmSubBlock::ppmData(int channel, std::vector<int>& lut,
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
  int beg = (channel % channelsPerSubBlock()) * (sliceL + sliceF);
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
  }
}

// Store PPM data for later packing

void PpmSubBlock::fillPpmData(int channel, const std::vector<int>& lut,
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
  int offset = (channel % chanPerSubBlock) * slices;
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

// Packing/Unpacking routines

bool PpmSubBlock::pack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case L1CaloSubBlock::NEUTRAL:
	  rc = packNeutral();
	  break;
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = packUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
	  switch (seqno()) {
	    case 1:
	      rc = packCompressedV01();
	      break;
	    default:
	      break;
          }
	  break;
        case L1CaloSubBlock::SUPERCOMPRESSED:
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
        case L1CaloSubBlock::NEUTRAL:
	  rc = unpackNeutral();
	  break;
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = unpackUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
	  switch (seqno()) {
	    case 1:
	      rc = unpackCompressedV01();
	      break;
	    default:
	      break;
          }
	  break;
        case L1CaloSubBlock::SUPERCOMPRESSED:
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

// Pack compressed data version 1

bool PpmSubBlock::packCompressedV01()
{
  const int pedestal = 20;  // should come from elsewhere
  const int trigOffset = 2; // ditto
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  if (sliceL != 1 || sliceF != 5) return false;
  int slices = sliceL + sliceF;
  int channels = channelsPerSubBlock();
  if (m_datamap.empty()) m_datamap.resize(slices * channels);
  setStreamed();
  m_compStats.clear();
  m_compStats.resize(7);
  std::vector<uint32_t> fadcData(sliceF);
  std::vector<uint32_t> fadcBcid(sliceF);
  std::vector<uint32_t> fadcDout(sliceF-1);
  std::vector<uint32_t> fadcBout(sliceF-1);
  std::vector<int>      fadcLens(sliceF-1);
  for (int chan = 0; chan < channels; ++chan) {
    uint32_t lut = m_datamap[chan*slices];
    uint32_t lutData = (lut >> s_lutBit)     & s_lutMask;
    uint32_t lutBcid = (lut >> s_bcidLutBit) & s_bcidLutMask;
    int      lutLen  = minBits(lutData);
    uint32_t minFadc = 0;
    uint32_t firstFadc = 0;
    int      minOffset = 0;
    bool     fadcSame = true;
    for (int sl = 0; sl < sliceF; ++sl) {
      uint32_t fadc = m_datamap[chan*slices + 1 + sl];
      fadcData[sl] = (fadc >> s_fadcBit)     & s_fadcMask;
      fadcBcid[sl] = (fadc >> s_bcidFadcBit) & s_bcidFadcMask;
      if (sl == 0) {
        minFadc = fadcData[sl];
	firstFadc = fadc;
      }
      if (fadcData[sl] < minFadc) {
        minFadc   = fadcData[sl];
	minOffset = sl;
      }
      if (fadc != firstFadc) fadcSame = false;
    }
    uint32_t format = 0;
    if (lut == 0 && fadcSame) { // format 6
      uint32_t header = 0xf;
      packer(header, 4);
      format = 6;
    } else {
      bool     minFadcInRange = minFadc >= pedestal - 12 &&
                                minFadc <= pedestal + 3;
      uint32_t anyFadcBcid = 0;
      int      maxFadcLen = 0;
      int      idx = 0;
      int      idy = 0;
      for (int sl = 0; sl < sliceF; ++sl) {
        if (sl != minOffset) {
	  fadcDout[idx] = fadcData[sl] - minFadc;
	  fadcLens[idx] = minBits(fadcDout[idx]);
	  if (idx == 0 || fadcLens[idx] > maxFadcLen) {
	    maxFadcLen = fadcLens[idx];
	  }
	  ++idx;
        }
	if (sl != trigOffset) {
	  fadcBout[idy] = fadcBcid[sl];
	  anyFadcBcid  |= fadcBout[idy];
	  ++idy;
        }
      }
      if (lut == 0 && !anyFadcBcid && minFadcInRange && maxFadcLen < 4) {
        // formats 0,1
        uint32_t header = minOffset;
	if (maxFadcLen == 3) header += 5;
	packer(header, 4);
	minFadc -= pedestal - 12;
	packer(minFadc, 4);
	if (maxFadcLen < 2) maxFadcLen = 2;
	for (int idx = 0; idx < sliceF-1; ++idx) {
	  packer(fadcDout[idx], maxFadcLen);
        }
	format = maxFadcLen - 2;
      } else if (lutLen <= 3 && (lut == 0 || (lutData > 0 && lutBcid == 0x4))
                 && !anyFadcBcid && minFadcInRange && maxFadcLen <= 4) {
        // format 2
	uint32_t header = minOffset + 10;
	packer(header, 4);
	format = 2;
	packer(format - 2, 2);
        if (lutData) {
	  packer(1, 1);
	  packer(lutData, 3);
	} else packer(0, 1);
	minFadc -= pedestal - 12;
	packer(minFadc, 4);
	for (int idx = 0; idx < sliceF-1; ++idx) {
	  packer(fadcDout[idx], 4);
        }
      } else {
        // formats 3,4,5
	uint32_t header = minOffset + 10;
	packer(header, 4);
	int minFadcLen = minBits(minFadc);
	if (minFadcLen > maxFadcLen) maxFadcLen = minFadcLen;
	format = 5;
	if (maxFadcLen <= 8) format = 4;
	if (maxFadcLen <= 6) format = 3;
        packer(format - 2, 2);
	if (lut) packer(1, 1);
	else packer(0, 1);
	packer(anyFadcBcid, 1);
	if (lut) packer(lut, 11);
	if (anyFadcBcid) {
	  for (int idx = 0; idx < sliceF-1; ++idx) {
	    packer(fadcBout[idx], 1);
          }
        }
	if (minFadcInRange) {
	  packer(0, 1);
	  minFadc -= pedestal - 12;
	  packer(minFadc, 4);
        } else {
	  packer(1, 1);
          packer(minFadc, format * 2);
        }
	for (int idx = 0; idx < sliceF-1; ++idx) {
	  if (fadcLens[idx] <= 4) {
	    packer(0, 1);
	    packer(fadcDout[idx], 4);
          } else {
	    packer(1, 1);
	    packer(fadcDout[idx], format * 2);
          }
        }
      }
    }
    ++m_compStats[format];
  }
  packerFlush();
  return true;
}

// Pack neutral data

bool PpmSubBlock::packNeutral()
{
  int slices   = slicesLut() + slicesFadc();
  int channels = channelsPerSubBlock();
  if (m_datamap.empty()) m_datamap.resize(slices * channels);
  // Bunch crossing number ignored
  packer(0, s_fullWordLen);
  // Data is packed a bit at a time from consecutive pins
  for (int asic = 0; asic < s_asicChannels; ++asic) {
    for (int sl = 0; sl < slices; ++sl) {
      for (int bit = 0; bit < s_dataBits; ++bit) {
        uint32_t word = 0;
        for (int pin = 0; pin < s_glinkPins; ++pin) {
          word |= ((m_datamap[asic*slices*s_glinkPins
	                     + pin*slices + sl] >> bit) & 0x1) << pin;
        }
	packer(word, s_fullWordLen);
      }
    }
  }
  // Errors ignored
  for (int bit = 0; bit < s_errorBits; ++bit) {
    packer(0, s_fullWordLen);
  }
  // Parity bits ignored
  packer(0, s_fullWordLen);
  packerFlush();
  return true;
}

// Pack super-compressed data

bool PpmSubBlock::packSuperCompressed()
{
  return false;
}

// Pack uncompressed data

bool PpmSubBlock::packUncompressed()
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

// Unpack compressed data version 1

bool PpmSubBlock::unpackCompressedV01()
{
  const int pedestal = 20;  // should come from elsewhere
  const int trigOffset = 2; // ditto
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  if (sliceL != 1 || sliceF != 5) return false;
  int channels = channelsPerSubBlock();
  m_datamap.clear();
  setStreamed();
  unpackerInit();
  m_compStats.clear();
  m_compStats.resize(7);
  for (int chan = 0; chan < channels; ++chan) {
    int format = 0;
    int header = unpacker(4);
    if (header < 10) {
      // formats 0,1 - LUT zero, FADC around pedestal
      int minOffset = header % 5;
          format    = header / 5;
      // LUT = 0
      m_datamap.push_back(0);
      // FADC
      uint32_t minFadc = unpacker(4) + pedestal - 12;
      for (int sl = 0; sl < sliceF; ++sl) {
        if (sl == minOffset) m_datamap.push_back(minFadc << 1);
	else m_datamap.push_back((unpacker(format + 2) + minFadc) << 1);
      }
    } else if (header < 15) {
      // formats 2-5
      int minOffset = header - 10;
          format = unpacker(2) + 2;
      int anyLut = unpacker(1);
      uint32_t lut = 0;
      if (format == 2) {
        // LUT
	if (anyLut) {
	  lut = unpacker(3);
	  uint32_t bcidLut = 0x4;  // just peak-finding BCID set
	  lut |= bcidLut << s_bcidLutBit;
        }
	m_datamap.push_back(lut);
	// FADC as formats 0,1
        uint32_t minFadc = unpacker(4) + pedestal - 12;
        for (int sl = 0; sl < sliceF; ++sl) {
          if (sl == minOffset) m_datamap.push_back(minFadc << 1);
	  else m_datamap.push_back((unpacker(format + 2) + minFadc) << 1);
        }
      } else {
        // formats 3,4,5 - full LUT word, variable FADC
	int anyBcid = unpacker(1);
	// LUT
	if (anyLut) lut = unpacker(11);
        m_datamap.push_back(lut);
	// FADC
	std::vector<uint32_t> bcidBits;
	for (int sl = 0; sl < sliceF; ++sl) {
	  if (sl == trigOffset) { // take from LUT word
	    bcidBits.push_back((lut >> s_bcidLutBit) & 0x1);
	  } else {
	    if (anyBcid) bcidBits.push_back(unpacker(1));
	    else bcidBits.push_back(0);
	  }
        }
	int longField = unpacker(1);
	uint32_t minFadc = 0;
	if (longField) minFadc = unpacker(format * 2);
	else minFadc = unpacker(4) + pedestal - 12;
	for (int sl = 0; sl < sliceF; ++sl) {
	  uint32_t fadc = minFadc;
	  if (sl != minOffset) {
	    longField = unpacker(1);
	    if (longField) fadc += unpacker(format * 2);
	    else           fadc += unpacker(4);
          }
	  m_datamap.push_back((fadc << 1) | bcidBits[sl]);
        }
      }
    } else {
      // format 6 - zero data, header only
      format = 6;
      // LUT
      m_datamap.push_back(0);
      // FADC
      for (int sl = 0; sl < sliceF; ++sl) m_datamap.push_back(0);
    }
    ++m_compStats[format];
  }
  return unpackerSuccess();
}

// Unpack neutral data

bool PpmSubBlock::unpackNeutral()
{
  int slices = slicesLut() + slicesFadc();
  int channels = channelsPerSubBlock();
  m_datamap.resize(slices * channels);
  unpackerInit();
  // Skip BC number
  unpacker(s_fullWordLen);
  // Data is packed a bit at a time from consecutive pins
  for (int asic = 0; asic < s_asicChannels; ++asic) {
    for (int sl = 0; sl < slices; ++sl) {
      for (int bit = 0; bit < s_dataBits; ++bit) {
        uint32_t word = unpacker(s_glinkPins);
        for (int pin = 0; pin < s_glinkPins; ++pin) {
	  m_datamap[asic*slices*s_glinkPins + pin*slices + sl] |=
	    ((word >> pin) & 0x1) << bit;
        }
	// Skip remainder of data word
	unpacker(s_fullWordLen - s_glinkPins);
      }
    }
  }
  // Ignore errors and parity bits
  return unpackerSuccess();
}

// Unpack super-compressed data

bool PpmSubBlock::unpackSuperCompressed()
{
  return false;
}

// Unpack uncompressed data

bool PpmSubBlock::unpackUncompressed()
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

// Return the number of channels per sub-block

int PpmSubBlock::channelsPerSubBlock(int version, int format)
{
  int chan = 0;
  switch (version) {
    case 1:
      switch (format) {
        case L1CaloSubBlock::UNCOMPRESSED:
	  chan = PpmCrateMappings::channels()/s_asicChannels;
	  break;
        case L1CaloSubBlock::NEUTRAL:
        case L1CaloSubBlock::COMPRESSED:
        case L1CaloSubBlock::SUPERCOMPRESSED:
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
