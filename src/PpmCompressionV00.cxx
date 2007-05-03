
#include <stdint.h>
#include <vector>

#include "TrigT1CaloByteStream/PpmCompressionV00.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

namespace LVL1BS {

// Static constants

const int PpmCompressionV00::s_formats;
const int PpmCompressionV00::s_lowerRange;
const int PpmCompressionV00::s_upperRange;
const int PpmCompressionV00::s_peakOnly;
const int PpmCompressionV00::s_lutDataBits;
const int PpmCompressionV00::s_lutBcidBits;

// Pack data

bool PpmCompressionV00::pack(PpmSubBlock& subBlock)
{
  const int sliceL = subBlock.slicesLut();
  const int sliceF = subBlock.slicesFadc();
  if (sliceL != 1 || sliceF != 5) return false;
  const int trigOffset = subBlock.fadcOffset();
  const int pedestal   = subBlock.pedestal();
  const int channels   = subBlock.channelsPerSubBlock();
  subBlock.setStreamed();
  std::vector<uint32_t> compStats(s_formats);
  std::vector<int> fadcDout(sliceF-1);
  std::vector<int> fadcBout(sliceF-1);
  std::vector<int> fadcLens(sliceF-1);
  for (int chan = 0; chan < channels; ++chan) {
    std::vector<int> lutData;
    std::vector<int> lutBcid;
    std::vector<int> fadcData;
    std::vector<int> fadcBcid;
    subBlock.ppmData(chan, lutData, fadcData, lutBcid, fadcBcid);
    const int lutLen = subBlock.minBits(lutData[0]);
    int minFadc = 0;
    int minOffset = 0;
    for (int sl = 0; sl < sliceF; ++sl) {
      if (sl == 0) minFadc = fadcData[sl];
      if (fadcData[sl] < minFadc) {
        minFadc   = fadcData[sl];
	minOffset = sl;
      }
    }
    const bool minFadcInRange = minFadc >= pedestal - s_lowerRange &&
                                minFadc <= pedestal + s_upperRange;
    int format = 0;
    int anyFadcBcid = 0;
    int maxFadcLen = 0;
    int idx = 0;
    int idy = 0;
    for (int sl = 0; sl < sliceF; ++sl) {
      if (sl != minOffset) {
        fadcDout[idx] = fadcData[sl] - minFadc;
	fadcLens[idx] = subBlock.minBits(fadcDout[idx]);
	if (idx == 0 || fadcLens[idx] > maxFadcLen) {
	  maxFadcLen = fadcLens[idx];
	}
	++idx;
      }
      if (sl != trigOffset) {
        fadcBout[idy] = fadcBcid[sl];
	anyFadcBcid  |= fadcBout[idy];
	++idy;
      }
    }
    if (lutData[0] == 0 && lutBcid[0] == 0 &&
                        !anyFadcBcid && minFadcInRange && maxFadcLen < 4) {
      // formats 0,1
      int header = minOffset;
      if (maxFadcLen == 3) header += 5;
      subBlock.packer(header, 4);
      minFadc -= pedestal - s_lowerRange;
      subBlock.packer(minFadc, 4);
      if (maxFadcLen < 2) maxFadcLen = 2;
      for (int idx = 0; idx < sliceF-1; ++idx) {
	subBlock.packer(fadcDout[idx], maxFadcLen);
      }
      format = maxFadcLen - 2;
    } else if (lutLen <= 3 && ((lutData[0] == 0 && lutBcid[0] == 0) ||
                               (lutData[0]  > 0 && lutBcid[0] == s_peakOnly))
               && !anyFadcBcid && minFadcInRange && maxFadcLen <= 4) {
      // format 2
      const int header = minOffset + 10;
      subBlock.packer(header, 4);
      format = 2;
      subBlock.packer(format - 2, 2);
      if (lutData[0]) {
        subBlock.packer(1, 1);
        subBlock.packer(lutData[0], 3);
      } else subBlock.packer(0, 1);
      minFadc -= pedestal - s_lowerRange;
      subBlock.packer(minFadc, 4);
      for (int idx = 0; idx < sliceF-1; ++idx) {
	subBlock.packer(fadcDout[idx], 4);
      }
    } else {
      // formats 3,4,5
      const int header = minOffset + 10;
      subBlock.packer(header, 4);
      if ( !minFadcInRange) {
        const int minFadcLen = subBlock.minBits(minFadc);
        if (minFadcLen > maxFadcLen) maxFadcLen = minFadcLen;
      }
      format = 5;
      if (maxFadcLen <= 8) format = 4;
      if (maxFadcLen <= 6) format = 3;
      subBlock.packer(format - 2, 2);
      if (lutData[0] || lutBcid[0]) subBlock.packer(1, 1);
      else subBlock.packer(0, 1);
      subBlock.packer(anyFadcBcid, 1);
      if (lutData[0] || lutBcid[0]) {
        subBlock.packer(lutData[0], s_lutDataBits);
        subBlock.packer(lutBcid[0], s_lutBcidBits);
      }
      if (anyFadcBcid) {
        for (int idx = 0; idx < sliceF-1; ++idx) {
          subBlock.packer(fadcBout[idx], 1);
        }
      }
      if (minFadcInRange) {
	subBlock.packer(0, 1);
	minFadc -= pedestal - s_lowerRange;
	subBlock.packer(minFadc, 4);
      } else {
	subBlock.packer(1, 1);
        subBlock.packer(minFadc, format * 2);
      }
      for (int idx = 0; idx < sliceF-1; ++idx) {
	if (fadcLens[idx] <= 4) {
	  subBlock.packer(0, 1);
	  subBlock.packer(fadcDout[idx], 4);
        } else {
	  subBlock.packer(1, 1);
	  subBlock.packer(fadcDout[idx], format * 2);
        }
      }
    }
    ++compStats[format];
  }
  subBlock.packerFlush();
  subBlock.setCompStats(compStats);
  return true;
}

// Unpack data

bool PpmCompressionV00::unpack(PpmSubBlock& subBlock)
{
  const int sliceL = subBlock.slicesLut();
  const int sliceF = subBlock.slicesFadc();
  if (sliceL != 1 || sliceF != 5) return false;
  const int trigOffset = subBlock.fadcOffset();
  const int pedestal   = subBlock.pedestal();
  const int channels   = subBlock.channelsPerSubBlock();
  subBlock.setStreamed();
  subBlock.unpackerInit();
  std::vector<uint32_t> compStats(s_formats);
  for (int chan = 0; chan < channels; ++chan) {
    std::vector<int> lutData;
    std::vector<int> lutBcid;
    std::vector<int> fadcData;
    std::vector<int> fadcBcid;
    int format = 0;
    const int header = subBlock.unpacker(4);
    if (header < 10) {
      // formats 0,1 - LUT zero, FADC around pedestal
      const int minOffset = header % 5;
                format    = header / 5;
      // LUT = 0
      lutData.push_back(0);
      lutBcid.push_back(0);
      // FADC
      const uint32_t minFadc = subBlock.unpacker(4) + pedestal - s_lowerRange;
      for (int sl = 0; sl < sliceF; ++sl) {
        if (sl == minOffset) fadcData.push_back(minFadc);
	else fadcData.push_back(subBlock.unpacker(format + 2) + minFadc);
	fadcBcid.push_back(0);
      }
    } else {
      // formats 2-5
      const int minOffset = header - 10;
                format = subBlock.unpacker(2) + 2;
      const int anyLut = subBlock.unpacker(1);
      int lut  = 0;
      int bcid = 0;
      if (format == 2) {
        // LUT
	if (anyLut) {
	  lut  = subBlock.unpacker(3);
	  bcid = s_peakOnly;  // just peak-finding BCID set
        }
	lutData.push_back(lut);
	lutBcid.push_back(bcid);
	// FADC as formats 0,1
        const int minFadc = subBlock.unpacker(4) + pedestal - s_lowerRange;
        for (int sl = 0; sl < sliceF; ++sl) {
          if (sl == minOffset) fadcData.push_back(minFadc);
	  else fadcData.push_back(subBlock.unpacker(format + 2) + minFadc);
	  fadcBcid.push_back(0);
        }
      } else {
        // formats 3,4,5 - full LUT word, variable FADC
	const int anyBcid = subBlock.unpacker(1);
	// LUT
	if (anyLut) {
	  lut  = subBlock.unpacker(s_lutDataBits);
	  bcid = subBlock.unpacker(s_lutBcidBits);
	}
        lutData.push_back(lut);
	lutBcid.push_back(bcid);
	// FADC
	for (int sl = 0; sl < sliceF; ++sl) {
	  int fbcid = 0;
	  if (sl == trigOffset) fbcid = bcid & 0x1; // take from LUT word
	  else if (anyBcid) fbcid = subBlock.unpacker(1);
	  fadcBcid.push_back(fbcid);
        }
	int minFadc = 0;
	if (subBlock.unpacker(1)) minFadc = subBlock.unpacker(format * 2);
	else minFadc = subBlock.unpacker(4) + pedestal - s_lowerRange;
	for (int sl = 0; sl < sliceF; ++sl) {
	  int fadc = minFadc;
	  if (sl != minOffset) {
	    if (subBlock.unpacker(1)) fadc += subBlock.unpacker(format * 2);
	    else                      fadc += subBlock.unpacker(4);
          }
	  fadcData.push_back(fadc);
        }
      }
    }
    subBlock.fillPpmData(chan, lutData, fadcData, lutBcid, fadcBcid);
    ++compStats[format];
  }
  subBlock.setCompStats(compStats);
  return subBlock.unpackerSuccess();
}

} // end namespace
