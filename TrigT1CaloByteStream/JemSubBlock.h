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

   /// Store JEM header
           void setJemHeader(int version, int format, int slice, int crate,
                             int module, int timeslices);
   /// Store jet element data
           void fillJetElement(int slice, const JemJetElement& jetEle);
   /// Store jet hit counts
           void setJetHits(int slice, unsigned int hits);
   /// Store energy subsum data
           void setEnergySubsums(int slice, unsigned int ex,
	                         unsigned int ey, unsigned int et);

   /// Return jet element for given channel
           JemJetElement jetElement(int slice, int channel) const;
   /// Return jet hit counts
           unsigned int jetHits(int slice) const;
   /// Return energy subsum Ex
           unsigned int ex(int slice)      const;
   /// Return energy subsum Ey
           unsigned int ey(int slice)      const;
   /// Return energy subsum Et
           unsigned int et(int slice)      const;
   /// Return number of timeslices
           int  timeslices()               const;

   //  Bunch Crossing number (neutral format only)
           void setBunchCrossing(int bc);
	   int  bunchCrossing()            const;

   /// Pack data
   virtual bool pack();
   /// Unpack data
   virtual bool unpack();

 private:
   /// JEM header word ID
   static const int      s_wordIdVal       = 0xc;
   /// Data word length
   static const int      s_wordLength      = 32;
   //  Jet Element data word bit positions and masks
   static const int      s_dataIdBit       = 30;
   static const int      s_jeWordId        = 0x1;
   static const uint32_t s_dataIdMask      = 0x3;
   //  Jet hit counts bit positions and masks
   static const int      s_threshBit       = 0;
   static const int      s_sourceIdBit     = 25;
   static const int      s_jetParityBit    = 24;
   static const int      s_jetParity       = 0x0;
   static const int      s_mainThreshId    = 20;
   static const int      s_mainFwdThreshId = 21;
   static const int      s_threshWordId    = 0x2;
   static const uint32_t s_threshMask      = 0xffffff;
   static const uint32_t s_sourceIdMask    = 0x1f;
   //  Energy subsum data bit positions and masks
   static const int      s_exBit           = 0;
   static const int      s_eyBit           = 8;
   static const int      s_etBit           = 16;
   static const int      s_energyParityBit = 24;
   static const int      s_energyParity    = 0x1;
   static const int      s_subsumId        = 22;
   static const uint32_t s_exMask          = 0xff;
   static const uint32_t s_eyMask          = 0xff;
   static const uint32_t s_etMask          = 0xff;
   //  Neutral format data lengths
   static const int      s_pairsPerPin       = 3;
   static const int      s_jetElementBits    = 9;
   static const int      s_jePaddingBits     = 22;
   static const int      s_jetHitsBits       = 24;
   static const int      s_energyBits        = 8;
   static const int      s_bunchCrossingBits = 12;
   static const int      s_hitPaddingBits    = 4;
   static const int      s_glinkBitsPerSlice = 67;

   int sourceId(uint32_t word) const;
   int dataId(uint32_t word)   const;
   int index(int slice)        const;
   void resize(std::vector<uint32_t>& vec, int channels = 1);

   /// Pack neutral data
   bool packNeutral();
   /// Pack uncompressed data
   bool packUncompressed();
   /// Unpack neutral data
   bool unpackNeutral();
   /// Unpack uncompressed data
   bool unpackUncompressed();

   /// Jet element data
   std::vector<uint32_t> m_jeData;
   /// Jet hit counts
   std::vector<uint32_t> m_jetHits;
   /// Energy subsum data
   std::vector<uint32_t> m_energySubsums;
   /// Number of jet element channels
   int m_channels;
   /// Bunch Crossing number (neutral format only)
   int m_bunchCrossing;

};

inline void JemSubBlock::setBunchCrossing(int bc)
{
  m_bunchCrossing = bc;
}

inline int JemSubBlock::bunchCrossing() const
{
  return m_bunchCrossing;
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
