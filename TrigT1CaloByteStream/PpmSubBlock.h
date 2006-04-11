#ifndef TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H
#define TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H

#include <stdint.h>
#include <vector>

#include "TrigT1CaloByteStream/L1CaloSubBlock.h"

class PpmSortPermutations;

/** Sub-Block class for PPM data.
 *
 *  @author Peter Faulkner
 */

class PpmSubBlock : public L1CaloSubBlock {

 public:
   PpmSubBlock(int chan, uint16_t minorVersion, PpmSortPermutations* sortPerms);
   virtual ~PpmSubBlock();

   /// Clear all data
   virtual void clear();
   /// Store PPM data for later packing
           void fillPpmData(const std::vector<int>& lut,
                            const std::vector<int>& fadc,
                            const std::vector<int>& bcidLut,
		            const std::vector<int>& bcidFadc);
   /// Return unpacked data for given channel
           void ppmData(int channel, std::vector<int>& lut,
                                     std::vector<int>& fadc,
				     std::vector<int>& bcidLut,
				     std::vector<int>& bcidFadc) const;
   /// Pack data
   virtual bool pack();
   /// Unpack data
   virtual bool unpack();

 private:
   //  Data word positions and masks
   static const int      s_wordLen      = 16;
   static const int      s_lutBit       = 0;
   static const int      s_bcidLutBit   = 8;
   static const int      s_fadcBit      = 1;
   static const int      s_bcidFadcBit  = 0;
   static const uint32_t s_lutMask      = 0xff;
   static const uint32_t s_bcidLutMask  = 0x7;
   static const uint32_t s_fadcMask     = 0x3ff;
   static const uint32_t s_bcidFadcMask = 0x1;
   /// Header Word ID for PPM sub-block
   static const int      s_wordId       = 0xc;
   //  Compressed format packing flags
   static const uint32_t s_flagLutNoData = 0x0;
   static const uint32_t s_flagLutData   = 0x1;
   static const int      s_flagLutLen    = 1;
   static const int      s_dataLutLen    = 11;
   static const uint16_t s_testMinorVersion = 0x1000;

   //  Packing/unpacking for specific formats
   /// Pack compressed data
   bool packCompressed();
   /// Pack super-compressed data
   bool packSuperCompressed();
   /// Pack uncompressed data
   bool packUncompressed();
   /// Unpack compressed data
   bool unpackCompressed();
   /// Unpack super-compressed data
   bool unpackSuperCompressed();
   /// Unpack uncompressed data
   bool unpackUncompressed();

   /// The number of data channels per block
   int m_channels;
   /// The minor version from ROD header
   uint16_t m_minorVersion;
   /// Pointer to sort permutation provider
   PpmSortPermutations* m_sortPerms;

   /// Vector for intermediate data
   std::vector<uint32_t> m_datamap;

};

#endif
