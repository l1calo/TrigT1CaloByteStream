
#include "TrigT1CaloByteStream/PpmErrorBlock.h"

// Constant definitions

const uint32_t PpmErrorBlock::s_errorMask;
const int      PpmErrorBlock::s_wordLen;
const int      PpmErrorBlock::s_fpgaCorruptBit;
const int      PpmErrorBlock::s_bunchMismatchBit;
const int      PpmErrorBlock::s_eventMismatchBit;
const int      PpmErrorBlock::s_asicFullBit;
const int      PpmErrorBlock::s_timeoutBit;
const int      PpmErrorBlock::s_mcmAbsentBit;
const int      PpmErrorBlock::s_channelDisabledBit;

const int      PpmErrorBlock::s_wordId;

const uint32_t PpmErrorBlock::s_flagNoData;
const uint32_t PpmErrorBlock::s_flagData;
const int      PpmErrorBlock::s_flagLen;
const int      PpmErrorBlock::s_dataLen;

PpmErrorBlock::PpmErrorBlock(int pins) : m_glinkPins(pins),
                                  m_globalError(0), m_globalDone(false)
{
  setWordId(s_wordId);
}

PpmErrorBlock::~PpmErrorBlock()
{
}

// Clear all data

void PpmErrorBlock::clear()
{
  L1CaloSubBlock::clear();
  m_globalError = 0;
  m_globalDone  = false;
  m_datamap.clear();
}

// Return unpacked error data for given G-Link pin

int PpmErrorBlock::ppmError(int pin) const
{
  int errorWord = 0;
  if (pin >= 0 && size_t(pin) < m_datamap.size()) {
    errorWord = m_datamap[pin] & s_errorMask;
  }
  return errorWord;
}

// Store Error data for later packing

void PpmErrorBlock::fillPpmError(int errorWord)
{
  uint32_t datum = errorWord & s_errorMask;
  m_datamap.push_back(datum);
}

// Return error bit

bool PpmErrorBlock::errorBit(int pin, int bit) const
{
  bool error = false;
  if (pin >= 0 && size_t(pin) < m_datamap.size()) {
    error = m_datamap[pin] & (0x1 << bit);
  }
  return error;
}

// Return global error bit

bool PpmErrorBlock::errorBit(int bit) const
{
  if ( ! m_globalDone) {
    std::vector<uint32_t>::const_iterator pos;
    for (pos = m_datamap.begin(); pos != m_datamap.end(); ++pos) {
      m_globalError |= *pos;
    }
    m_globalDone  = true;
  }
  return m_globalError & (0x1 << bit);
}

// Packing/Unpacking routines

bool PpmErrorBlock::pack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = packUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
        case L1CaloSubBlock::SUPERCOMPRESSED:
	  rc = packCompressed();
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

bool PpmErrorBlock::unpack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case L1CaloSubBlock::UNCOMPRESSED:
	  rc = unpackUncompressed();
	  break;
        case L1CaloSubBlock::COMPRESSED:
        case L1CaloSubBlock::SUPERCOMPRESSED:
	  rc = unpackCompressed();
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

// Pack compressed and super-compressed data

bool PpmErrorBlock::packCompressed()
{
  uint32_t anyErrors = 0;
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_datamap.begin(); pos != m_datamap.end(); ++pos) {
    anyErrors |= *pos;
  }
  if (anyErrors) {
    setStreamed();
    uint32_t word = 0;
    for (pos = m_datamap.begin(); pos != m_datamap.end(); ++pos) {
      word = *pos;
      if (word) {
        packer(s_flagData, s_flagLen);
        packer(word, s_dataLen);
      } else {
        packer(s_flagNoData, s_flagLen);
      }
    }
    packerFlush();
  }
  return true;
}

// Unpack compressed and super-compressed data

bool PpmErrorBlock::unpackCompressed()
{
  setStreamed();
  unpackerInit();
  for (int pin = 0; pin < m_glinkPins; ++pin) {
    uint32_t datum = 0;
    uint32_t flag = unpacker(s_flagLen);
    if (flag == s_flagData) datum = unpacker(s_dataLen);
    m_datamap.push_back(datum);
  }
  return true;
}

// Pack uncompressed data

bool PpmErrorBlock::packUncompressed()
{
  bool sizeOK = m_datamap.size() == size_t(m_glinkPins);
  for (int pin = 0; pin < m_glinkPins; ++pin) {
    if (sizeOK) packer(m_datamap[pin], s_wordLen);
    else packer(0, s_wordLen);
  }
  packerFlush();
  return true;
}

// Unpack uncompressed data

bool PpmErrorBlock::unpackUncompressed()
{
  unpackerInit();
  for (int pin = 0; pin < m_glinkPins; ++pin) {
    m_datamap.push_back(unpacker(s_wordLen));
  }
  return true;
}
