#ifndef TRIGT1CALOBYTESTREAM_PPMCOMPRESSIONV00_H
#define TRIGT1CALOBYTESTREAM_PPMCOMPRESSIONV00_H


/** PPM Compressed Format Version 0 packing and unpacking utilities.
 *
 *  Based on:
 *
 *  "ATLAS L1Calo Pre-processor compressed S-Link data formats",
 *   Version 1.1, D.P.C.Sankey.
 *
 *  @author Peter Faulkner
 */

class PpmSubBlock;

class PpmCompressionV00 {

 public:
   PpmCompressionV00();
   ~PpmCompressionV00();

   /// Pack data
   static bool pack(PpmSubBlock& subBlock);
   /// Unpack data
   static bool unpack(PpmSubBlock& subBlock);

 private:
   static const int s_formats      = 6;
   static const int s_pedestal     = 20;
   static const int s_lowerRange   = 12;
   static const int s_upperRange   = 3;
   static const int s_peakOnly     = 4;
   static const int s_lutDataBits  = 8;
   static const int s_lutBcidBits  = 3;

};

#endif
