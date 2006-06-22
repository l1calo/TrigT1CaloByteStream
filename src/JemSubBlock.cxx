#include <utility>

#include "TrigT1CaloByteStream/JemCrateMappings.h"
#include "TrigT1CaloByteStream/JemSubBlock.h"

// Constant definitions

const int      JemSubBlock::s_wordLength;

const int      JemSubBlock::s_dataIdBit;
const int      JemSubBlock::s_jeWordId;
const uint32_t JemSubBlock::s_dataIdMask;

const int      JemSubBlock::s_threshBit;
const int      JemSubBlock::s_sourceIdBit;
const int      JemSubBlock::s_mainThreshId;
const int      JemSubBlock::s_mainFwdThreshId;
const int      JemSubBlock::s_threshWordId;
const uint32_t JemSubBlock::s_threshMask;
const uint32_t JemSubBlock::s_sourceIdMask;

const int      JemSubBlock::s_exBit;
const int      JemSubBlock::s_eyBit;
const int      JemSubBlock::s_etBit;
const int      JemSubBlock::s_setBit;
const int      JemSubBlock::s_setBitVal;
const int      JemSubBlock::s_subsumId;
const uint32_t JemSubBlock::s_exMask;
const uint32_t JemSubBlock::s_eyMask;
const uint32_t JemSubBlock::s_etMask;
const uint32_t JemSubBlock::s_setBitMask;


JemSubBlock::JemSubBlock() : m_jetHits(0), m_energySubsums(0)
{
  m_channels = JemCrateMappings::channels();
  m_jeData.resize(m_channels, 0);
}

JemSubBlock::~JemSubBlock()
{
}

// Clear all data

void JemSubBlock::clear()
{
  L1CaloSubBlock::clear();
  m_jeData.clear();
  m_jeData.resize(m_channels, 0);
  m_jetHits       = 0;
  m_energySubsums = 0;
}

// Store jet element data

void JemSubBlock::fillJetElement(const JemJetElement& jetEle)
{
  if (jetEle.data()) {
    int channel = jetEle.channel();
    if (channel < m_channels) m_jeData[channel] = jetEle.data();
  }
}

// Store jet hit counts

void JemSubBlock::setJetHits(int hits)
{
  uint32_t word = 0;
  if (hits) {
    int sourceId = s_mainThreshId;
    if (JemCrateMappings::forward(module())) sourceId = s_mainFwdThreshId;
    word |= (hits           & s_threshMask)   << s_threshBit;
    word |= (sourceId       & s_sourceIdMask) << s_sourceIdBit;
    word |= (s_threshWordId & s_dataIdMask)   << s_dataIdBit;
  }
  m_jetHits = word;
}

// Store energy subsum data

void JemSubBlock::setEnergySubsums(int ex, int ey, int et)
{
  uint32_t word = 0;
  word |= (ex & s_exMask) << s_exBit;
  word |= (ey & s_eyMask) << s_eyBit;
  word |= (et & s_etMask) << s_etBit;
  if (word) {
    word |= (s_setBitVal    & s_setBitMask)   << s_setBit;
    word |= (s_subsumId     & s_sourceIdMask) << s_sourceIdBit;
    word |= (s_threshWordId & s_dataIdMask)   << s_dataIdBit;
  }
  m_energySubsums = word;
}

// Return jet element for given channel

JemJetElement JemSubBlock::jetElement(int channel) const
{
  if (channel >= 0 && channel < m_channels) {
    return JemJetElement(m_jeData[channel]);
  }
  else return JemJetElement(0);
}

// Packing/Unpacking routines

bool JemSubBlock::pack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case L1CaloSubBlock::UNCOMPRESSED:
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

bool JemSubBlock::unpack()
{
  bool rc = false;
  switch (version()) {
    case 1:
      switch (format()) {
        case L1CaloSubBlock::UNCOMPRESSED:
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

// Pack uncompressed data

bool JemSubBlock::packUncompressed()
{
  // Jet element data
  std::vector<uint32_t>::const_iterator pos;
  for (pos = m_jeData.begin(); pos != m_jeData.end(); ++pos) {
    if (*pos) packer(*pos, s_wordLength);
  }
        
  // Hits and Subsum data
  if (m_jetHits)       packer(m_jetHits,       s_wordLength);
  if (m_energySubsums) packer(m_energySubsums, s_wordLength);
  packerFlush();
  return true;
}

// Unpack uncompressed data

bool JemSubBlock::unpackUncompressed()
{
  m_jeData.clear();
  m_jeData.resize(m_channels, 0);
  m_jetHits       = 0;
  m_energySubsums = 0;
  unpackerInit();
  uint32_t word = unpacker(s_wordLength);
  while (unpackerSuccess()) {
    int id = dataId(word);
    // Jet element data
    if (id == s_jeWordId) {
      JemJetElement jetEle(word);
      int channel = jetEle.channel();
      if (channel < m_channels) m_jeData[channel] = word;
      else return false;
    // Other data
    } else if (id == s_threshWordId) {
      switch (sourceId(word)) {
	// Jet hit counts/thresholds
        case s_mainThreshId:
        case s_mainFwdThreshId:
          m_jetHits = word;
          break;
	// Energy subsums
        case s_subsumId:
	  m_energySubsums = word;
	  break;
        default:
	  return false;
	  break;
      }
    } else return false;
    word = unpacker(s_wordLength);
  }
  return true;
}
