#ifndef TRIGT1CALOBYTESTREAM_PPMERRORBLOCK_H
#define TRIGT1CALOBYTESTREAM_PPMERRORBLOCK_H

#include <stdint.h>
#include <vector>

#include "TrigT1CaloByteStream/L1CaloSubBlock.h"

/** Sub-Block class for PPM error data.
 *
 *  @author Peter Faulkner
 */

class PpmErrorBlock : public L1CaloSubBlock {

 public:
   PpmErrorBlock();
   virtual ~PpmErrorBlock();

   /// Clear all data
   virtual void clear();

   /// Store an error word
           void fillPpmError(int chan, int errorWord);

   /// Return an error word
           int  ppmError(int chan)        const;

   //  Return individual error bits
           bool fpgaCorrupt(int chan)     const;
           bool bunchMismatch(int chan)   const;
           bool eventMismatch(int chan)   const;
           bool asicFull(int chan)        const;
           bool timeout(int chan)         const;
           bool mcmAbsent(int chan)       const;
           bool channelDisabled(int chan) const;
   //  Ditto ORed over all pins
           bool fpgaCorrupt()             const;
           bool bunchMismatch()           const;
           bool eventMismatch()           const;
           bool asicFull()                const;
           bool timeout()                 const;
           bool mcmAbsent()               const;
           bool channelDisabled()         const;

   /// Pack data
   virtual bool pack();
   /// Unpack data
   virtual bool unpack();

 private:
   //  Error word mask and bit positions
   static const uint32_t s_errorMask          = 0x3ff;
   static const int      s_wordLen            = 16;
   static const int      s_fpgaCorruptBit     = 6;
   static const int      s_bunchMismatchBit   = 5;
   static const int      s_eventMismatchBit   = 4;
   static const int      s_asicFullBit        = 3;
   static const int      s_timeoutBit         = 2;
   static const int      s_mcmAbsentBit       = 1;
   static const int      s_channelDisabledBit = 0;
   /// The number of error fields (G-Link pins)
   static const int      s_glinkPins          = 16;
   //  Compressed format packing flags
   static const uint32_t s_flagNoData         = 0x0;
   static const uint32_t s_flagData           = 0x1;
   static const int      s_flagLen            = 1;
   static const int      s_dataLen            = 10;

   /// Return the G-Link pin corresponding to a channel
   int  pin(int chan) const;

   /// Error bit extraction
   bool errorBit(int chan, int bit) const;
   /// Global error bit extraction
   bool errorBit(int bit) const;

   /// Pack compressed or super-compressed data
   bool packCompressed();
   /// Pack uncompressed data
   bool packUncompressed();
   /// unpack compressed or super-compressed data
   bool unpackCompressed();
   /// unpack uncompressed data
   bool unpackUncompressed();

   //  Global error flags
   mutable uint32_t m_globalError;
   mutable bool     m_globalDone;

   /// Intermediate data store
   std::vector<uint32_t> m_datamap;

};

inline bool PpmErrorBlock::fpgaCorrupt(int chan) const
{
  return errorBit(chan, s_fpgaCorruptBit);
}

inline bool PpmErrorBlock::bunchMismatch(int chan) const
{
  return errorBit(chan, s_bunchMismatchBit);
}

inline bool PpmErrorBlock::eventMismatch(int chan) const
{
  return errorBit(chan, s_eventMismatchBit);
}

inline bool PpmErrorBlock::asicFull(int chan) const
{
  return errorBit(chan, s_asicFullBit);
}

inline bool PpmErrorBlock::timeout(int chan) const
{
  return errorBit(chan, s_timeoutBit);
}

inline bool PpmErrorBlock::mcmAbsent(int chan) const
{
  return errorBit(chan, s_mcmAbsentBit);
}

inline bool PpmErrorBlock::channelDisabled(int chan) const
{
  return errorBit(chan, s_channelDisabledBit);
}

inline bool PpmErrorBlock::fpgaCorrupt() const
{
  return errorBit(s_fpgaCorruptBit);
}

inline bool PpmErrorBlock::bunchMismatch() const
{
  return errorBit(s_bunchMismatchBit);
}

inline bool PpmErrorBlock::eventMismatch() const
{
  return errorBit(s_eventMismatchBit);
}

inline bool PpmErrorBlock::asicFull() const
{
  return errorBit(s_asicFullBit);
}

inline bool PpmErrorBlock::timeout() const
{
  return errorBit(s_timeoutBit);
}

inline bool PpmErrorBlock::mcmAbsent() const
{
  return errorBit(s_mcmAbsentBit);
}

inline bool PpmErrorBlock::channelDisabled() const
{
  return errorBit(s_channelDisabledBit);
}

inline int  PpmErrorBlock::pin(int chan) const
{
  return chan % s_glinkPins;
}

#endif
