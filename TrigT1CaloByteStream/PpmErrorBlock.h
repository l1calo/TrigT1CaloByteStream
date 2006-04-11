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
   PpmErrorBlock(int pins);
   virtual ~PpmErrorBlock();

   /// Clear all data
   virtual void clear();

   /// Store an error word
   void fillPpmError(int errorWord);

   /// Return an error word
   int  ppmError(int pin) const;

   //  Return individual error bits
   bool fpgaCorrupt(int pin)     const;
   bool bunchMismatch(int pin)   const;
   bool eventMismatch(int pin)   const;
   bool asicFull(int pin)        const;
   bool timeout(int pin)         const;
   bool mcmAbsent(int pin)       const;
   bool channelDisabled(int pin) const;
   //  Ditto ORed over all pins
   bool fpgaCorrupt()     const;
   bool bunchMismatch()   const;
   bool eventMismatch()   const;
   bool asicFull()        const;
   bool timeout()         const;
   bool mcmAbsent()       const;
   bool channelDisabled() const;

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
   /// Header Word ID for PPM sub-block
   static const int      s_wordId     = 0xc;
   //  Compressed format packing flags
   static const uint32_t s_flagNoData = 0x0;
   static const uint32_t s_flagData   = 0x1;
   static const int      s_flagLen    = 1;
   static const int      s_dataLen    = 10;

   /// Error bit extraction
   bool errorBit(int pin, int bit) const;
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

   /// The number of error fields per block
   int m_glinkPins;

   //  Global error flags
   mutable uint32_t m_globalError;
   mutable bool     m_globalDone;

   /// Intermediate data store
   std::vector<uint32_t> m_datamap;

};

inline bool PpmErrorBlock::fpgaCorrupt(int pin) const
{
  return errorBit(pin, s_fpgaCorruptBit);
}

inline bool PpmErrorBlock::bunchMismatch(int pin) const
{
  return errorBit(pin, s_bunchMismatchBit);
}

inline bool PpmErrorBlock::eventMismatch(int pin) const
{
  return errorBit(pin, s_eventMismatchBit);
}

inline bool PpmErrorBlock::asicFull(int pin) const
{
  return errorBit(pin, s_asicFullBit);
}

inline bool PpmErrorBlock::timeout(int pin) const
{
  return errorBit(pin, s_timeoutBit);
}

inline bool PpmErrorBlock::mcmAbsent(int pin) const
{
  return errorBit(pin, s_mcmAbsentBit);
}

inline bool PpmErrorBlock::channelDisabled(int pin) const
{
  return errorBit(pin, s_channelDisabledBit);
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

#endif
