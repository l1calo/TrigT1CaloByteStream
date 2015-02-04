#include "SubBlockHeader.h"

using namespace LVL1BS;

SubBlockHeader::SubBlockHeader(uint32_t header):
  m_header(header), m_decoder(
      {
        BitField("Type", 28, 4),
        BitField("Version", 25, 3),
        BitField("Format", 22, 3),
        BitField("SeqNum", 16, 6),
        BitField("Crate", 12, 4),
        BitField("Module", 8, 4),
        BitField("nSlice2", 3, 5),
        BitField("nSlice1", 0, 3)
     }) {
}
