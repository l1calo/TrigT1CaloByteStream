
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

const int      PpmErrorBlock::s_glinkPins;

const uint32_t PpmErrorBlock::s_flagNoData;
const uint32_t PpmErrorBlock::s_flagData;
const int      PpmErrorBlock::s_flagLen;
const int      PpmErrorBlock::s_dataLen;

PpmErrorBlock::PpmErrorBlock() : m_globalError(0), m_globalDone(false),
                                 m_datamap(s_glinkPins)
{
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
  m_datamap.resize(s_glinkPins);
}

// Return unpacked error data for given channel

int PpmErrorBlock::ppmError(int chan) const
{
  return m_datamap[pin(chan)] & s_errorMask;
}

// Store Error data for later packing

void PpmErrorBlock::fillPpmError(int chan, int errorWord)
{
  m_datamap[pin(chan)] = errorWord & s_errorMask;
}

// Return error bit

bool PpmErrorBlock::errorBit(int chan, int bit) const
{
  return m_datamap[pin(chan)] & (0x1 << bit);
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
  return false;
}

// Unpack compressed and super-compressed data

bool PpmErrorBlock::unpackCompressed()
{
  return false;
}

// Pack uncompressed data

bool PpmErrorBlock::packUncompressed()
{
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    packer(m_datamap[pin], s_wordLen);
  }
  packerFlush();
  return true;
}

// Unpack uncompressed data

bool PpmErrorBlock::unpackUncompressed()
{
  unpackerInit();
  for (int pin = 0; pin < s_glinkPins; ++pin) {
    m_datamap.push_back(unpacker(s_wordLen));
  }
  return unpackerSuccess();
}
