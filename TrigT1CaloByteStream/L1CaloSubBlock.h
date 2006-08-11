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
   enum SubBlockHeadType { PPM_JEM_CPM, CMM };
   enum CmmFirmwareCode  { CMM_CP = 0, CMM_JET = 1, CMM_ENERGY = 2,
                           CMM_UNKNOWN = 3 };
   enum DataFormats      { NEUTRAL = 0, UNCOMPRESSED = 1, COMPRESSED = 2,
                           SUPERCOMPRESSED = 3 };

   L1CaloSubBlock();
   virtual ~L1CaloSubBlock();

   /// Clear all data
   virtual void clear();
   
   //  Return unpacked header data
   int wordId()      const;
   int version()     const;
   int format()      const;
   int seqno()       const;
   int slice()       const;
   int crate()       const;
   int module()      const;
   int timeslices()  const;
   int slicesFadc()  const;
   int slicesLut()   const;
   int cmmSumming()  const;
   int cmmFirmware() const;
   int cmmPosition() const;

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

   /// Store PPM header
   void setPpmHeader(int version, int format, int seqno, int crate,
                     int module, int slicesFadc, int slicesLut);
   /// Store JEM header
   void setJemHeader(int version, int format, int slice, int crate,
                     int module, int timeslices);
   /// Store CPM header
   void setCpmHeader(int version, int format, int slice, int crate,
                     int module, int timeslices);
   /// Store CMM header
   void setCmmHeader(int version, int format, int slice, int crate,
                     int summing, int firmware, int position, int timeslices);

   /// Store error status trailer
   void setStatus(uint32_t failingBCN, bool glinkTimeout, bool glinkDown,
                  bool daqOverflow, bool bcnMismatch, bool glinkProtocol,
		  bool glinkParity);

   /// To be implemented in derived classes to pack data words
   virtual bool pack();
   /// To be implemented in derived classes to unpack data words
   virtual bool unpack();

   /// Return error marker for PPM error block
   static int errorMarker();
 
   /// Word identification
   static SubBlockWordType wordType(uint32_t word);
   /// Header differentiation (PPM_JEM_CPM or CMM)
   static SubBlockHeadType headerType(uint32_t word);
   /// CMM differentiation (CMM_CP, CMM_JET, or CMM_ENERGY)
   static CmmFirmwareCode  cmmType(uint32_t word);

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
   /// Return unpacking success flag
   bool     unpackerSuccess();

 private:
   //  Constants.
   //  Header and status ID
   static const int      s_headerBit        = 30;
   static const int      s_statusBit        = 28;
   static const uint32_t s_errorMarker      = 63;
   static const uint32_t s_headerMask       = 0x3;
   static const uint32_t s_statusMask       = 0x1;
   static const uint32_t s_headerVal        = 0x3;
   static const uint32_t s_statusVal        = 0x1;
   //  Header word data positions and masks
   static const int      s_wordIdBit        = 28;
   static const int      s_versionBit       = 25;
   static const int      s_formatBit        = 22;
   static const int      s_seqnoBit         = 16;
   static const int      s_crateBit         = 12;
   static const int      s_moduleBit        = 8;
   static const int      s_slices2Bit       = 4;
   static const int      s_slices1Bit       = 0;
   static const uint32_t s_wordIdMask       = 0xf;
   static const uint32_t s_versionMask      = 0x7;
   static const uint32_t s_formatMask       = 0x7;
   static const uint32_t s_seqnoMask        = 0x3f;
   static const uint32_t s_crateMask        = 0xf;
   static const uint32_t s_moduleMask       = 0xf;
   static const uint32_t s_slices2Mask      = 0xf;
   static const uint32_t s_slices1Mask      = 0xf;
   static const uint32_t s_wordIdVal        = 0xc;
   static const uint32_t s_cmmWordIdVal     = 0xe;
   //  CMM fields packed in module field
   static const int      s_cmmSummingBit    = 3;
   static const int      s_cmmFirmwareBit   = 1;
   static const int      s_cmmPositionBit   = 0;
   static const uint32_t s_cmmSummingMask   = 0x1;
   static const uint32_t s_cmmFirmwareMask  = 0x3;
   static const uint32_t s_cmmPositionMask  = 0x1;
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
   static const int      s_maxWordBits      = 32;
   static const int      s_maxStreamedBits  = 31;
   static const uint32_t s_maxWordMask      = 0xffffffff;
   static const uint32_t s_maxStreamedMask  = 0x7fffffff;

   int  slices2() const;
   int  slices1() const;

   /// Store header data
   void setHeader(int wordId, int version, int format, int seqno, int crate,
                  int module, int slices2, int slices1);

   /// Sub-Block Header
   uint32_t m_header;
   /// Sub-Block Status Trailer
   uint32_t m_trailer;
   //  Used for bit-packing
   uint32_t m_bitword;
   int      m_currentBit;
   int      m_maxBits;
   uint32_t m_maxMask;
   bool     m_unpackerFlag;
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

inline int L1CaloSubBlock::slice() const
{
  return seqno();
}

inline int L1CaloSubBlock::crate() const
{
  return (m_header >> s_crateBit) & s_crateMask;
}

inline int L1CaloSubBlock::module() const
{
  return (m_header >> s_moduleBit) & s_moduleMask;
}

inline int L1CaloSubBlock::timeslices() const
{
  return slices1();
}

inline int L1CaloSubBlock::slicesFadc() const
{
  return slices2() > 0 ? slices2() : 5;
}

inline int L1CaloSubBlock::slicesLut() const
{
  return slices1() > 0 ? slices1() : 1;
}

inline int L1CaloSubBlock::cmmSumming() const
{
  return (module() >> s_cmmSummingBit) & s_cmmSummingMask;
}

inline int L1CaloSubBlock::cmmFirmware() const
{
  return (module() >> s_cmmFirmwareBit) & s_cmmFirmwareMask;
}

inline int L1CaloSubBlock::cmmPosition() const
{
  return (module() >> s_cmmPositionBit) & s_cmmPositionMask;
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

inline void L1CaloSubBlock::setStreamed()
{
  m_maxBits = s_maxStreamedBits;
  m_maxMask = s_maxStreamedMask;
}

inline bool L1CaloSubBlock::unpackerSuccess()
{
  return m_unpackerFlag;
}

inline int L1CaloSubBlock::slices2() const
{
  return (m_header >> s_slices2Bit) & s_slices2Mask;
}

inline int L1CaloSubBlock::slices1() const
{
  return (m_header >> s_slices1Bit) & s_slices1Mask;
}

#endif
