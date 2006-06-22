#ifndef TRIGT1CALOBYTESTREAM_JEMSUBBLOCK_H
#define TRIGT1CALOBYTESTREAM_JEMSUBBLOCK_H

#include <stdint.h>
#include <vector>

#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/JemJetElement.h"

/** Sub-Block class for JEM data.
 *
 *  Based on "ATLAS Level-1 Calorimeter Trigger Read-out Driver"
 *           Version 1.06d
 *
 *  @author Peter Faulkner
 */

class JemSubBlock : public L1CaloSubBlock {

 public:
   JemSubBlock();
   virtual ~JemSubBlock();

   /// Clear all data
   virtual void clear();

   /// Store jet element data
           void fillJetElement(const JemJetElement& jetEle);
   /// Store jet hit counts
           void setJetHits(int hits);
   /// Store energy subsum data
           void setEnergySubsums(int ex, int ey, int et);

   /// Return jet element for given channel
           JemJetElement jetElement(int channel) const;
   /// Return jet hit counts
           int  jetHits()   const;
   /// Return energy subsum Ex
           int  ex()        const;
   /// Return energy subsum Ey
           int  ey()        const;
   /// Return energy subsum Et
           int  et()        const;

   /// Pack data
   virtual bool pack();
   /// Unpack data
   virtual bool unpack();

 private:
   /// Data word length
   static const int      s_wordLength      = 32;
   //  Jet Element data word bit positions and masks
   static const int      s_dataIdBit       = 30;
   static const int      s_jeWordId        = 0x1;
   static const uint32_t s_dataIdMask      = 0x3;
   //  Jet hit counts bit positions and masks
   static const int      s_threshBit       = 0;
   static const int      s_sourceIdBit     = 25;
   static const int      s_mainThreshId    = 20;
   static const int      s_mainFwdThreshId = 21;
   static const int      s_threshWordId    = 0x2;
   static const uint32_t s_threshMask      = 0xffffff;
   static const uint32_t s_sourceIdMask    = 0x1f;
   //  Energy subsum data bit positions and masks
   static const int      s_exBit           = 0;
   static const int      s_eyBit           = 8;
   static const int      s_etBit           = 16;
   static const int      s_setBit          = 24;
   static const int      s_setBitVal       = 0x1;
   static const int      s_subsumId        = 22;
   static const uint32_t s_exMask          = 0xff;
   static const uint32_t s_eyMask          = 0xff;
   static const uint32_t s_etMask          = 0xff;
   static const uint32_t s_setBitMask      = 0x1;

   int sourceId(uint32_t word) const;
   int dataId(uint32_t word)   const;

   /// Pack uncompressed data
   bool packUncompressed();
   /// Unpack uncompressed data
   bool unpackUncompressed();

   /// Jet element data
   std::vector<uint32_t> m_jeData;
   /// Jet hit counts
   uint32_t m_jetHits;
   /// Energy subsum data
   uint32_t m_energySubsums;
   /// Number of jet element channels
   int m_channels;

};

inline int JemSubBlock::jetHits() const
{
  return (m_jetHits >> s_threshBit) & s_threshMask;
}

inline int JemSubBlock::ex() const
{
  return (m_energySubsums >> s_exBit) & s_exMask;
}

inline int JemSubBlock::ey() const
{
  return (m_energySubsums >> s_eyBit) & s_eyMask;
}

inline int JemSubBlock::et() const
{
  return (m_energySubsums >> s_etBit) & s_etMask;
}

inline int JemSubBlock::sourceId(uint32_t word) const
{
  return (word >> s_sourceIdBit) & s_sourceIdMask;
}

inline int JemSubBlock::dataId(uint32_t word) const
{
  return (word >> s_dataIdBit) & s_dataIdMask;
}

#endif
