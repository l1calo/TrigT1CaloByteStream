
#include <algorithm>

#include "TrigT1CaloByteStream/PpmSubBlock.h"
#include "TrigT1CaloByteStream/PpmSortPermutations.h"

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

const int      PpmSubBlock::s_wordId;

const uint32_t PpmSubBlock::s_flagLutNoData;
const uint32_t PpmSubBlock::s_flagLutData;
const int      PpmSubBlock::s_flagLutLen;
const int      PpmSubBlock::s_dataLutLen;
const uint16_t PpmSubBlock::s_testMinorVersion;

PpmSubBlock::PpmSubBlock(int chan, uint16_t minorVersion,
                         PpmSortPermutations* sortPerms) :
    m_channels(chan), m_minorVersion(minorVersion), m_sortPerms(sortPerms)
{
  setWordId(s_wordId);
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
  int index = channel;
  if (index >= seqno()) index -= seqno();
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  int beg = index * (sliceL + sliceF);
  int end = beg + sliceL;
  lut.clear();
  bcidLut.clear();
  for (int pos = beg; pos < end; ++pos) {
    uint32_t word = m_datamap[pos];
    lut.push_back((word >> s_lutBit) & s_lutMask);
    bcidLut.push_back((word >> s_bcidLutBit) & s_bcidLutMask);
  }
  beg += sliceL;
  end += sliceF;
  fadc.clear();
  bcidFadc.clear();
  for (int pos = beg; pos < end; ++pos) {
    uint32_t word = m_datamap[pos];
    fadc.push_back((word >> s_fadcBit) & s_fadcMask);
    bcidFadc.push_back((word >> s_bcidFadcBit) & s_bcidFadcMask);
  }
}

// Store PPM data for later packing

void PpmSubBlock::fillPpmData(const std::vector<int>& lut,
                                    const std::vector<int>& fadc,
			            const std::vector<int>& bcidLut,
			            const std::vector<int>& bcidFadc)
{
  // Data are stored in the order received
  
  bool bcidOK = lut.size() == bcidLut.size();
  std::vector<int>::const_iterator pos;
  std::vector<int>::const_iterator posbcid;
  posbcid = bcidLut.begin();
  for (pos = lut.begin(); pos != lut.end(); ++pos) {
    uint32_t datum = ((*pos) & s_lutMask) << s_lutBit;
    if (bcidOK) {
      datum |= ((*posbcid) & s_bcidLutMask) << s_bcidLutBit;
      ++posbcid;
    }
    m_datamap.push_back(datum);
  }
  bcidOK = fadc.size() == bcidFadc.size();
  posbcid = bcidFadc.begin();
  for (pos = fadc.begin(); pos != fadc.end(); ++pos) {
    uint32_t datum = ((*pos) & s_fadcMask) << s_fadcBit;
    if (bcidOK) {
      datum |= ((*posbcid) & s_bcidFadcMask) << s_bcidFadcBit;
      ++posbcid;
    }
    m_datamap.push_back(datum);
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
  // This is a test version only, not the real thing
  if (m_minorVersion != s_testMinorVersion) return false;
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  if (m_datamap.size() != size_t((sliceL+sliceF) * m_channels)) return false;
  setStreamed();
  std::vector<uint32_t>::const_iterator pos = m_datamap.begin();
  for (int chan = 0; chan < m_channels; ++chan) {
    // LUT data
    for (int sl = 0; sl < sliceL; ++sl) {
      uint32_t word = *pos;
      if (word) {
        packer(s_flagLutData, s_flagLutLen);
	packer(word, s_dataLutLen);
      } else {
        packer(s_flagLutNoData, s_flagLutLen);
      }
      ++pos;
    }
    // FADC data
    // Not clear if BCID bit should be left in but it is here
    // Sort and get permutation code
    std::vector<uint32_t> fadc;
    for (int sl = 0; sl < sliceF; ++sl) {
      fadc.push_back((*pos)*sliceF + sl);
      ++pos;
    }
    std::sort(fadc.begin(), fadc.end());
    std::vector<int> permVec;
    uint32_t last = 0;
    for (int sl = 0; sl < sliceF; ++sl) {
      permVec.push_back(fadc[sl]%sliceF);
      uint32_t temp = fadc[sl] / sliceF;
      fadc[sl] = temp - last;
      last = temp;
    }
    uint32_t permCode = m_sortPerms->permutationCode(permVec);
    // get perm code field length
    int permLen = minBits(m_sortPerms->totalPerms(sliceF) - 1);
    if (permLen > 0) packer(permCode, permLen);
    // get compression code - unknown at present so just use the simplest
    // I can think of - field length codes as follows
    //      0  0 bits
    //     01  1 bit
    //    011  2 bits
    //   0111  3 bits, etc.
    std::vector<int> fadcLens;
    std::vector<uint32_t>::const_iterator fpos = fadc.begin();
    for (; fpos != fadc.end(); ++fpos) {
      int len = minBits(*fpos);
      for (int i = 0; i < len; ++i) packer(1, 1);
      packer(0, 1);
      fadcLens.push_back(len);
    }
    // and finally the data
    fpos = fadc.begin();
    std::vector<int>::const_iterator lpos = fadcLens.begin();
    for (; fpos != fadc.end(); ++fpos) {
      packer(*fpos, *lpos);
      ++lpos;
    }
  }
  packerFlush();
  return true;
}

// Unpack compressed data

bool PpmSubBlock::unpackCompressed()
{
  // This is a test version only, not the real thing
  if (m_minorVersion != s_testMinorVersion) return false;
  int sliceL = slicesLut();
  int sliceF = slicesFadc();
  setStreamed();
  unpackerInit();
  for (int chan = 0; chan < m_channels; ++chan) {
    // LUT data
    for (int sl = 0; sl < sliceL; ++sl) {
      uint32_t word = 0;
      uint32_t flag = unpacker(s_flagLutLen);
      if (flag == s_flagLutData) word = unpacker(s_dataLutLen);
      m_datamap.push_back(word);
    }
    // FADC data
    // get permutation code
    int permLen = minBits(m_sortPerms->totalPerms(sliceF) - 1);
    uint32_t permCode = unpacker(permLen);
    // translate to an ordering vector
    std::vector<int> permVec(sliceF);
    m_sortPerms->permutationVector(permCode, permVec);
    // get numbers of bits from compression code
    std::vector<int> fadcLens;
    for (int sl = 0; sl < sliceF; ++sl) {
      int len = 0;
      while (unpacker(1)) ++len;
      fadcLens.push_back(len);
    }
    // and get the data
    uint32_t accum = 0;
    std::vector<uint32_t> fadc(sliceF);
    for (int sl = 0; sl < sliceF; ++sl) {
      accum += unpacker(fadcLens[sl]);
      fadc[permVec[sl]] = accum;
    }
    std::vector<uint32_t>::const_iterator fpos = fadc.begin();
    for (; fpos != fadc.end(); ++fpos) m_datamap.push_back(*fpos);
  }
  return true;
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
  int slices = slicesLut() + slicesFadc();
  if (m_datamap.size() != size_t(slices * m_channels)) return false;
  for (int sl = 0; sl < slices; ++sl) {
    for (int chan = 0; chan < m_channels; ++chan) {
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
  m_datamap.resize(slices * m_channels);
  unpackerInit();
  for (int sl = 0; sl < slices; ++sl) {
    for (int chan = 0; chan < m_channels; ++chan) {
      m_datamap[sl + chan*slices] = unpacker(s_wordLen);
    }
  }
  return true;
}

