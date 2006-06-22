
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

const uint32_t PpmSubBlock::s_flagLutNoData;
const uint32_t PpmSubBlock::s_flagLutData;
const int      PpmSubBlock::s_flagLutLen;
const int      PpmSubBlock::s_dataLutLen;

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
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = packUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
	  rc = packCompressed();
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
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = unpackUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
	  rc = unpackCompressed();
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

// Pack compressed data

bool PpmSubBlock::packCompressed()
{
  return false;
}

// Unpack compressed data

bool PpmSubBlock::unpackCompressed()
{
  return false;
}

// Pack super-compressed data

bool PpmSubBlock::packSuperCompressed()
{
  return false;
}

// Unpack super-compressed data

bool PpmSubBlock::unpackSuperCompressed()
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
	  chan = PpmCrateMappings::channels()/4;
	  break;
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
