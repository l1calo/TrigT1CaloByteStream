#ifndef TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H
#define TRIGT1CALOBYTESTREAM_PPMSUBBLOCK_H

#include <stdint.h>
#include <vector>

#include "TrigT1CaloByteStream/L1CaloSubBlock.h"

/** Sub-Block class for PPM data.
 *
 *  @author Peter Faulkner
 */

class PpmSubBlock : public L1CaloSubBlock {

 public:
   PpmSubBlock();
   virtual ~PpmSubBlock();

   /// Clear all data
   virtual void clear();
   /// Return reference to compression stats
   const std::vector<uint32_t>& compStats() const;

   /// Store PPM data for later packing
           void fillPpmData(int channel, const std::vector<int>& lut,
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

   /// Return the number of channels per sub-block
   static  int  channelsPerSubBlock(int version, int format);
           int  channelsPerSubBlock() const;

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
   // For neutral format
   static const int      s_fullWordLen  = 32;
   static const int      s_asicChannels = 4;
   static const int      s_dataBits     = 11;
   static const int      s_glinkPins    = 16;
   static const int      s_errorBits    = 10;

   //  Packing/unpacking for specific formats
   /// Pack compressed data version 1
   bool packCompressedV01();
   /// Pack neutral data
   bool packNeutral();
   /// Pack super-compressed data
   bool packSuperCompressed();
   /// Pack uncompressed data
   bool packUncompressed();
   /// Unpack compressed data version 1
   bool unpackCompressedV01();
   /// Unpack neutral data
   bool unpackNeutral();
   /// Unpack super-compressed data
   bool unpackSuperCompressed();
   /// Unpack uncompressed data
   bool unpackUncompressed();

   /// Vector for compression statistics
   std::vector<uint32_t> m_compStats;

   /// Vector for intermediate data
   std::vector<uint32_t> m_datamap;

};

#endif
