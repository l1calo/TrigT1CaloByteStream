#ifndef TRIGT1CALOBYTESTREAM_L1CALOSUBBLOCK_H
#define TRIGT1CALOBYTESTREAM_L1CALOSUBBLOCK_H

#include <stdint.h>
#include <vector>

#include "ByteStreamCnvSvcBase/FullEventAssembler.h"

#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"

/** L1Calo Sub-Block base class.
 *
 *  Provides common functionality for all L1Calo Sub-Block derived types.
 *
 *  @author Peter Faulkner
 */

class L1CaloSubBlock {

 public:
   enum SubBlockWordType { DATA_HEADER, DATA_WORD, ERROR_HEADER, ERROR_STATUS };
   enum DataFormats      { NEUTRAL = 0, UNCOMPRESSED = 1, COMPRESSED = 2,
                           SUPERCOMPRESSED = 3 };

   L1CaloSubBlock();
   virtual ~L1CaloSubBlock();

   /// Clear all data
   virtual void clear();
   
   //  Return unpacked header data
   int wordId()     const;
   int version()    const;
   int format()     const;
   int seqno()      const;
   int crate()      const;
   int module()     const;
   int slicesFadc() const;
   int slicesLut()  const;

   //  Return unpacked error status data
   uint32_t failingBCN()     const;
   bool     glinkTimeout()   const;
   bool     glinkDown()      const;
   bool     daqOverflow()    const;
   bool     bcnMismatch()    const;
   bool     glinkProtocol()  const;
   bool     glinkParity()    const;

   /// Input complete packed sub-block from ROD array
   // (OFFLINE_FRAGMENTS_NAMESPACE::PointerType = uint32_t*)
   OFFLINE_FRAGMENTS_NAMESPACE::PointerType read(
                      const OFFLINE_FRAGMENTS_NAMESPACE::PointerType beg,
	              const OFFLINE_FRAGMENTS_NAMESPACE::PointerType end);

   /// Output complete packed sub-block to ROD vector
   // (FullEventAssembler<L1CaloSrcIdMap>::RODDATA = vector<uint32_t>)
   void write(FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD) const;

   /// Store header
   void setHeader(int iversion, int iformat, int iseqno, int icrate,
                  int imodule, int islicesFadc, int islicesLut);

   /// Store error status trailer
   void setStatus(uint32_t ifailingBCN, bool bglinkTimeout, bool bglinkDown,
                  bool bdaqOverflow, bool bbcnMismatch, bool bglinkProtocol,
		  bool bglinkParity);

   /// Set header Word ID
   void setWordId(int iwordId);

   /// To be implemented in derived classes to pack data words
   virtual bool pack();
   /// To be implemented in derived classes to unpack data words
   virtual bool unpack();

   /// Return error marker for PPM error block
   static int errorMarker();
 
   /// Word identification
   static SubBlockWordType wordType(uint32_t word);

 protected: 
   //  Packing utilities
   /// Return the minimum number of bits needed for given data
   int      minBits(uint32_t datum);
   /// Pack given data into given number of bits
   void     packer(uint32_t datum, int nbits);
   /// Flush the current data word padded with zeros
   void     packerFlush();
   /// Set continuous bit streaming for compressed formats
   void     setStreamed();
   /// Unpack given number of bits of data
   uint32_t unpacker(int nbits);
   /// Initialise unpacker
   void     unpackerInit();

 private:
   //  Constants.
   //  Header and status ID
   static const int      s_headerBit   = 30;
   static const int      s_statusBit   = 28;
   static const uint32_t s_errorMarker = 63;
   static const uint32_t s_headerMask  = 0x3;
   static const uint32_t s_statusMask  = 0x1;
   static const uint32_t s_headerVal   = 0x3;
   static const uint32_t s_statusVal   = 0x1;
   //  Header word data positions and masks
   static const int      s_wordIdBit      = 28;
   static const int      s_versionBit     = 25;
   static const int      s_formatBit      = 22;
   static const int      s_seqnoBit       = 16;
   static const int      s_crateBit       = 12;
   static const int      s_moduleBit      = 8;
   static const int      s_slicesFadcBit  = 4;
   static const int      s_slicesLutBit   = 0;
   static const uint32_t s_wordIdMask     = 0xf;
   static const uint32_t s_versionMask    = 0x7;
   static const uint32_t s_formatMask     = 0x7;
   static const uint32_t s_seqnoMask      = 0x3f;
   static const uint32_t s_crateMask      = 0xf;
   static const uint32_t s_moduleMask     = 0xf;
   static const uint32_t s_slicesFadcMask = 0xf;
   static const uint32_t s_slicesLutMask  = 0xf;
   //  Status word data positions and masks
   static const int      s_failingBcnBit    = 22;
   static const int      s_glinkTimeoutBit  = 7;
   static const int      s_glinkDownBit     = 6;
   static const int      s_daqOverflowBit   = 3;
   static const int      s_bcnMismatchBit   = 2;
   static const int      s_glinkProtocolBit = 1;
   static const int      s_glinkParityBit   = 0;
   static const uint32_t s_failingBcnMask   = 0x3f;
   //  Packing word sizes and masks
   static const int      s_maxWordBits     = 32;
   static const int      s_maxStreamedBits = 31;
   static const uint32_t s_maxWordMask     = 0xffffffff;
   static const uint32_t s_maxStreamedMask = 0x7fffffff;

   /// Sub-Block Header
   uint32_t m_header;
   /// Sub-Block Status Trailer
   uint32_t m_trailer;
   /// Header word ID
   int      m_wordId;
   //  Used for bit-packing
   uint32_t m_bitword;
   int      m_currentBit;
   int      m_maxBits;
   uint32_t m_maxMask;
   std::vector<uint32_t>::const_iterator m_dataPos;
   std::vector<uint32_t>::const_iterator m_dataPosEnd;
   /// Sub-Block data
   std::vector<uint32_t> m_data;

};

inline int L1CaloSubBlock::wordId() const
{
  return (m_header >> s_wordIdBit) & s_wordIdMask;
}

inline int L1CaloSubBlock::version() const
{
  return (m_header >> s_versionBit) & s_versionMask;
}

inline int L1CaloSubBlock::format() const
{
  return (m_header >> s_formatBit) & s_formatMask;
}

inline int L1CaloSubBlock::seqno() const
{
  return (m_header >> s_seqnoBit) & s_seqnoMask;
}

inline int L1CaloSubBlock::crate() const
{
  return (m_header >> s_crateBit) & s_crateMask;
}

inline int L1CaloSubBlock::module() const
{
  return (m_header >> s_moduleBit) & s_moduleMask;
}

inline int L1CaloSubBlock::slicesFadc() const
{
  return (m_header >> s_slicesFadcBit) & s_slicesFadcMask;
}

inline int L1CaloSubBlock::slicesLut() const
{
  return (m_header >> s_slicesLutBit) & s_slicesLutMask;
}

inline uint32_t L1CaloSubBlock::failingBCN() const
{
  return (m_trailer >> s_failingBcnBit) & s_failingBcnMask;
}

inline bool L1CaloSubBlock::glinkTimeout() const
{
  return m_trailer & (0x1 << s_glinkTimeoutBit);
}

inline bool L1CaloSubBlock::glinkDown() const
{
  return m_trailer & (0x1 << s_glinkDownBit);
}

inline bool L1CaloSubBlock::daqOverflow() const
{
  return m_trailer & (0x1 << s_daqOverflowBit);
}

inline bool L1CaloSubBlock::bcnMismatch() const
{
  return m_trailer & (0x1 << s_bcnMismatchBit);
}

inline bool L1CaloSubBlock::glinkProtocol() const
{
  return m_trailer & (0x1 << s_glinkProtocolBit);
}

inline bool L1CaloSubBlock::glinkParity() const
{
  return m_trailer & (0x1 << s_glinkParityBit);
}

inline void L1CaloSubBlock::setWordId(int iwordId)
{
  m_wordId = iwordId;
}

inline void L1CaloSubBlock::setStreamed()
{
  m_maxBits = s_maxStreamedBits;
  m_maxMask = s_maxStreamedMask;
}

#endif
