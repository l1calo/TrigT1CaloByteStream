#ifndef TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H
#define TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H

#include <stdint.h>
#include <vector>

#include "L1CaloSubBlock.h"

namespace LVL1BS {

/** Sub-Block class for PPM data.
 *
 *  @author Peter Faulkner
 */

class PpmSubBlock : public L1CaloSubBlock {

 public:
   PpmSubBlock();
   ~PpmSubBlock();

   /// Clear all data
   void clear();

   /// Store PPM header
   void setPpmHeader(int version, int format, int seqno, int crate,
	             int module, int slicesFadc, int slicesLut);
   /// Store PPM error block header
   void setPpmErrorHeader(int version, int format, int crate,
	                  int module, int slicesFadc, int slicesLut);

   //  Return PPM-specific header data
   int  slicesFadc() const;
   int  slicesLut()  const;

   /// Store PPM data for later packing
   void fillPpmData(int chan, const std::vector<uint_least8_t>& lut,
                              const std::vector<uint_least16_t>& fadc,
                              const std::vector<int>& bcidLut,
		              const std::vector<int>& bcidFadc);
   /// Return unpacked data for given channel
   void ppmData(int chan,
        std::vector<uint_least8_t>& lutCp,
        std::vector<uint_least8_t>& lutJep,
        std::vector<uint_least16_t>& fadc,
        std::vector<uint_least8_t>& bcidCp,
        std::vector<uint_least8_t>& satJep,
			  std::vector<uint_least8_t>& bcidFadc,
        std::vector<int_least16_t>& correction,
        std::vector<uint_least8_t>& correctionEnabled
    );

   /// Store an error word corresponding to a data channel
   void fillPpmError(int chan, int errorWord);
   /// Store an error word corresponding to a G-Link pin
   void fillPpmPinError(int pin, int errorWord);

   /// Return the error word for a data channel
   int  ppmError(int chan)        const;
   /// Return the error word for a G-Link pin
   int  ppmPinError(int pin)      const;

   //  Return individual error bits
   bool glinkPinParity(int chan)  const;
   bool fpgaCorrupt(int chan)     const;
   bool bunchMismatch(int chan)   const;
   bool eventMismatch(int chan)   const;
   bool asicFull(int chan)        const;
   bool timeout(int chan)         const;
   bool mcmAbsent(int chan)       const;
   bool channelDisabled(int chan) const;
   bool channelDisabledA(int pin) const;
   bool channelDisabledB(int pin) const;
   bool channelDisabledC(int pin) const;
   bool channelDisabledD(int pin) const;
   //  Ditto ORed over all pins
   bool glinkPinParity()          const;
   bool fpgaCorrupt()             const;
   bool bunchMismatch()           const;
   bool eventMismatch()           const;
   bool asicFull()                const;
   bool timeout()                 const;
   bool mcmAbsent()               const;
   bool channelDisabledA()        const;
   bool channelDisabledB()        const;
   bool channelDisabledC()        const;
   bool channelDisabledD()        const;

   //  Set triggered slice offsets, pedestal value
   void setLutOffset(int offset);
   void setFadcOffset(int offset);
   void setPedestal(int pedval);
   void setFadcBaseline(int baseline);
   void setFadcThreshold(int threshold);
   void setRunNumber(int run);
   void setRodVersion(uint32_t rodVersion);

   //  Return triggered slice offsets, pedestal value
   int  lutOffset()               const;
   int  fadcOffset()              const;
   int  pedestal()                const;
   int  fadcBaseline()            const;
   int  fadcThreshold()           const;
   int  runNumber()               const;
   uint16_t rodMinorVersion()     const;
   bool isRun2() const;

   /// Pack data
   bool pack();
   /// Unpack data
   bool unpack();

   /// Return the number of channels per sub-block
   int channelsPerSubBlock(int version, int format);
   int channelsPerSubBlock();

   /// Check if a header word is for an error block
   static bool errorBlock(uint32_t word);

   /// Set compression stats
   void setCompStats(const std::vector<uint32_t>& stats);
   /// Return reference to compression stats
   const std::vector<uint32_t>& compStats() const;

 private:
   //  Header word data
   static const uint32_t s_wordIdVal    = 0xc;
   static const int      s_errorMarker  = 63;
   //  Data word positions and masks
   static const int      s_wordLen      = 16;
   static const int      s_lutBit       = 0;
   static const int      s_bcidLutBit   = 8;

   static const int      s_fadcBit      = 1;
   static const int      s_fadcBitV2    = 0;

   static const int      s_bcidFadcBit  = 0;
   static const int      s_bcidFadcBitV2  = 10;

   static const uint32_t s_lutMask      = 0xff;
   static const uint32_t s_bcidLutMask  = 0x7;
   static const uint32_t s_fadcMask     = 0x3ff;
   static const uint32_t s_bcidFadcMask = 0x1;
   //  For neutral format
   static const int      s_channels          = 64;
   static const int      s_glinkPins         = 16;
   static const int      s_asicChannels      = 4;
   static const int      s_dataBits          = 11;
   static const int      s_errorBits         = 10;
   static const int      s_bunchCrossingBits = 12;
   //  Error word masks and bit positions
   static const uint32_t s_errorMask          = 0x7ff;
   static const int      s_glinkPinParityBit  = 10;
   static const int      s_fpgaCorruptBit     = 9;
   static const int      s_bunchMismatchBit   = 8;
   static const int      s_eventMismatchBit   = 7;
   static const int      s_asicFullBit        = 6;
   static const int      s_timeoutBit         = 5;
   static const int      s_mcmAbsentBit       = 4;
   static const int      s_channelDisabledBit = 0;

   static const uint16_t s_run2minorVersion   = 0x1004;

   /// Return the ASIC channel corresponding to a data channel
   int  asic(int chan) const;
   /// Return the G-Link pin corresponding to a data channel
   int  pin(int chan) const;

   /// Error bit extraction
   bool errorBit(int pin, int bit) const;
   /// Global error bit extraction
   bool errorBit(int bit) const;

   //  Packing/unpacking for specific formats
   /// Pack neutral data
   bool packNeutral();
   /// Pack uncompressed data
   bool packUncompressedData();
   /// Pack uncompressed error data
   bool packUncompressedErrors();
   /// Unpack neutral data
   bool unpackNeutral();
   /// Unpack uncompressed data
   bool unpackUncompressedData();
   /// Unpack uncompressed error data
   bool unpackUncompressedErrors();

   //  Global error flags
   mutable uint32_t m_globalError;
   mutable bool     m_globalDone;

   //  Triggered slice offsets, pedestal value
   int m_lutOffset;
   int m_fadcOffset;
   int m_pedestal;
   int m_fadcBaseline;
   int m_fadcThreshold;
   int m_runNumber;
   uint32_t m_rodVersion;

   /// Vector for compression statistics
   std::vector<uint32_t> m_compStats;

   /// Vector for intermediate data
   std::vector<uint32_t> m_datamap;

   /// Vector for intermediate error data
   std::vector<uint32_t> m_errormap;

  /// Return unpacked data for given channel
 void ppmDataRun1(int chan,
      std::vector<uint_least8_t>& lut,
      std::vector<uint_least16_t>& fadc,
      std::vector<uint_least8_t>& bcidLut,
      std::vector<uint_least8_t>& bcidFadc
  );

  void ppmDataRun2(
    const int chan, 
    std::vector<uint_least8_t>& lutCp,
    std::vector<uint_least8_t>& lutJep,
    std::vector<uint_least16_t>& fadc,
    std::vector<uint_least8_t>& bcidLutCp,
    std::vector<uint_least8_t>& satLutJep,
    std::vector<uint_least8_t>& bcidFadc,
    std::vector<int_least16_t>& correction,
    std::vector<uint_least8_t>& correctionEnabled
   );
};

inline bool PpmSubBlock::glinkPinParity(const int chan) const
{
  return errorBit(pin(chan), s_glinkPinParityBit);
}

inline bool PpmSubBlock::fpgaCorrupt(const int chan) const
{
  return errorBit(pin(chan), s_fpgaCorruptBit);
}

inline bool PpmSubBlock::bunchMismatch(const int chan) const
{
  return errorBit(pin(chan), s_bunchMismatchBit);
}

inline bool PpmSubBlock::eventMismatch(const int chan) const
{
  return errorBit(pin(chan), s_eventMismatchBit);
}

inline bool PpmSubBlock::asicFull(const int chan) const
{
  return errorBit(pin(chan), s_asicFullBit);
}

inline bool PpmSubBlock::timeout(const int chan) const
{
  return errorBit(pin(chan), s_timeoutBit);
}

inline bool PpmSubBlock::mcmAbsent(const int chan) const
{
  return errorBit(pin(chan), s_mcmAbsentBit);
}

inline bool PpmSubBlock::channelDisabled(const int chan) const
{
  return errorBit(pin(chan), s_channelDisabledBit + asic(chan));
}

inline bool PpmSubBlock::channelDisabledA(const int pin) const
{
  return errorBit(pin, s_channelDisabledBit);
}

inline bool PpmSubBlock::channelDisabledB(const int pin) const
{
  return errorBit(pin, s_channelDisabledBit + 1);
}

inline bool PpmSubBlock::channelDisabledC(const int pin) const
{
  return errorBit(pin, s_channelDisabledBit + 2);
}

inline bool PpmSubBlock::channelDisabledD(const int pin) const
{
  return errorBit(pin, s_channelDisabledBit + 3);
}

inline bool PpmSubBlock::glinkPinParity() const
{
  return errorBit(s_glinkPinParityBit);
}

inline bool PpmSubBlock::fpgaCorrupt() const
{
  return errorBit(s_fpgaCorruptBit);
}

inline bool PpmSubBlock::bunchMismatch() const
{
  return errorBit(s_bunchMismatchBit);
}

inline bool PpmSubBlock::eventMismatch() const
{
  return errorBit(s_eventMismatchBit);
}

inline bool PpmSubBlock::asicFull() const
{
  return errorBit(s_asicFullBit);
}

inline bool PpmSubBlock::timeout() const
{
  return errorBit(s_timeoutBit);
}

inline bool PpmSubBlock::mcmAbsent() const
{
  return errorBit(s_mcmAbsentBit);
}

inline bool PpmSubBlock::channelDisabledA() const
{
  return errorBit(s_channelDisabledBit);
}

inline bool PpmSubBlock::channelDisabledB() const
{
  return errorBit(s_channelDisabledBit + 1);
}

inline bool PpmSubBlock::channelDisabledC() const
{
  return errorBit(s_channelDisabledBit + 2);
}

inline bool PpmSubBlock::channelDisabledD() const
{
  return errorBit(s_channelDisabledBit + 3);
}

inline void PpmSubBlock::setLutOffset(const int offset)
{
  m_lutOffset = offset;
}

inline void PpmSubBlock::setFadcOffset(const int offset)
{
  m_fadcOffset = offset;
}

inline void PpmSubBlock::setPedestal(const int pedval)
{
  m_pedestal = pedval;
}

inline void PpmSubBlock::setFadcBaseline(const int baseline)
{
  m_fadcBaseline = baseline;
}

inline void PpmSubBlock::setFadcThreshold(const int threshold)
{
  m_fadcThreshold = threshold;
}

inline void PpmSubBlock::setRunNumber(const int run)
{
  m_runNumber = run;
}

inline void PpmSubBlock::setRodVersion(const uint32_t rodVersion)
{
  m_rodVersion = rodVersion;
}

inline uint16_t PpmSubBlock::rodMinorVersion() const
{
  return m_rodVersion & 0xffff;
}

inline bool PpmSubBlock::isRun2() const
{
  return rodMinorVersion() >= s_run2minorVersion;
}

inline int PpmSubBlock::lutOffset() const
{
  return (m_lutOffset < 0) ? slicesLut()/2 : m_lutOffset;
}

inline int PpmSubBlock::fadcOffset() const
{
  return (m_fadcOffset < 0) ? slicesFadc()/2 : m_fadcOffset;
}

inline int PpmSubBlock::pedestal() const
{
  return m_pedestal;
}

inline int PpmSubBlock::fadcBaseline() const
{
  return m_fadcBaseline;
}

inline int PpmSubBlock::fadcThreshold() const
{
  return m_fadcThreshold;
}

inline int PpmSubBlock::runNumber() const
{
  return m_runNumber;
}

inline const std::vector<uint32_t>& PpmSubBlock::compStats() const
{
  return m_compStats;
}

inline void PpmSubBlock::setCompStats(const std::vector<uint32_t>& stats)
{
  m_compStats = stats;
}

inline int  PpmSubBlock::asic(const int chan) const
{
  return chan / s_glinkPins;
}

inline int  PpmSubBlock::pin(const int chan) const
{
  return chan % s_glinkPins;
}

inline bool PpmSubBlock::errorBit(const int pin, const int bit) const
{
  return m_errormap[pin] & (0x1 << bit);
}

} // end namespace

#endif
