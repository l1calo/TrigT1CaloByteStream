#ifndef TRIGT1CALOBYTESTREAM_PPMCOMPRESSIONV01_H
#define TRIGT1CALOBYTESTREAM_PPMCOMPRESSIONV01_H

namespace LVL1BS {

/** PPM Compressed Format Version 1 packing and unpacking utilities.
 *
 *  Based on:
 *
 *  "ATLAS L1Calo Pre-processor compressed S-Link data formats",
 *   Version 1.1, D.P.C.Sankey.
 *
 *  @author Peter Faulkner
 */

class PpmSubBlock;

class PpmCompressionV01 {

 public:
   PpmCompressionV01();
   ~PpmCompressionV01();

   /// Pack data
   static bool pack(PpmSubBlock& subBlock);
   /// Unpack data
   static bool unpack(PpmSubBlock& subBlock);

 private:
   static const int s_formats      = 7;
   static const int s_lowerRange   = 12;
   static const int s_upperRange   = 3;
   static const int s_peakOnly     = 4;
   static const int s_lutDataBits  = 8;
   static const int s_lutBcidBits  = 3;
   static const int s_fadcDataBits = 10;
   static const int s_glinkPins    = 16;
   static const int s_statusBits   = 5;
   static const int s_errorBits    = 6;
   static const int s_statusMask   = 0x1f;

};

} // end namespace

#endif
