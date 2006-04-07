#ifndef TRIGT1CALOBYTESTREAM_L1CALOUSERHEADER_H
#define TRIGT1CALOBYTESTREAM_L1CALOUSERHEADER_H

#include <stdint.h>

/** L1Calo User Header class.
 *
 *  The User Header is the first word of the ROD data and contains
 *  Triggered slice offsets for all the sub-detector types.
 *
 *  @author Peter Faulkner
 */

class L1CaloUserHeader {

 public:

   /// Constructor - default just sets number of header words
   L1CaloUserHeader(uint32_t header = 0x1);

   /// Return packed header
   uint32_t header() const;

   /// Return number of header words (should be one)
   int words()   const;

   //  Return triggered slice offsets
   int jepCmm()  const;
   int cpCmm()   const;
   int jem()     const;
   int cpm()     const;
   int ppmLut()  const;
   int ppmFadc() const;

   //  Set triggered slice offsets
   void setJepCmm(int offset);
   void setCpCmm(int offset);
   void setJem(int offset);
   void setCpm(int offset);
   void setPpmLut(int offset);
   void setPpmFadc(int offset);

 private:
   //  Packed word bit positions
   static const int s_jepCmmBit  = 24;
   static const int s_cpCmmBit   = 20;
   static const int s_jemBit     = 16;
   static const int s_cpmBit     = 12;
   static const int s_ppmLutBit  = 8;
   static const int s_ppmFadcBit = 4;
   /// Field mask
   static const uint32_t s_mask  = 0xf;
   /// Packed Header
   uint32_t m_header;

};

inline uint32_t L1CaloUserHeader::header() const
{
  return m_header;
}

inline int L1CaloUserHeader::words() const
{
  return m_header & s_mask;
}

inline int L1CaloUserHeader::jepCmm() const
{
  return (m_header >> s_jepCmmBit) & s_mask;
}

inline int L1CaloUserHeader::cpCmm() const
{
  return (m_header >> s_cpCmmBit) & s_mask;
}

inline int L1CaloUserHeader::jem() const
{
  return (m_header >> s_jemBit) & s_mask;
}

inline int L1CaloUserHeader::cpm() const
{
  return (m_header >> s_cpmBit) & s_mask;
}

inline int L1CaloUserHeader::ppmLut() const
{
  return (m_header >> s_ppmLutBit) & s_mask;
}

inline int L1CaloUserHeader::ppmFadc() const
{
  return (m_header >> s_ppmFadcBit) & s_mask;
}

inline void L1CaloUserHeader::setJepCmm(int offset)
{
  m_header |= (s_mask & offset) << s_jepCmmBit;
}

inline void L1CaloUserHeader::setCpCmm(int offset)
{
  m_header |= (s_mask & offset) << s_cpCmmBit;
}

inline void L1CaloUserHeader::setJem(int offset)
{
  m_header |= (s_mask & offset) << s_jemBit;
}

inline void L1CaloUserHeader::setCpm(int offset)
{
  m_header |= (s_mask & offset) << s_cpmBit;
}

inline void L1CaloUserHeader::setPpmLut(int offset)
{
  m_header |= (s_mask & offset) << s_ppmLutBit;
}

inline void L1CaloUserHeader::setPpmFadc(int offset)
{
  m_header |= (s_mask & offset) << s_ppmFadcBit;
}

#endif
