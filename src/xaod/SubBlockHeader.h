#ifndef TRIGT1CALOBYTESTREAM_SUBBLOCKHEADER_H
#define TRIGT1CALOBYTESTREAM_SUBBLOCKHEADER_H

#include <cstdint>

namespace LVL1BS {

/** L1Calo User Header class.
 *
 *  The User Header is the first word of the ROD data and contains
 *  Triggered slice offsets for all the sub-detector types.
 *
 *  @author alexander.mazurov@cern.ch
 */

class SubBlockHeader {
private:
  const uint32_t m_header;
public:

  /// Constructor - default just sets word ID and number of header words
  SubBlockHeader(uint32_t header);
  uint8_t type() const { return m_decoder.get<uint8_t>(m_header, 0); }
  uint8_t version() const { return m_decoder.get<uint8_t>(m_header, 1); }
  uint8_t format() const { return m_decoder.get<uint8_t>(m_header,2); }
  uint8_t seqNum() const { return m_decoder.get<uint8_t>(m_header, 3); }
  uint8_t crate() const { return m_decoder.get<uint8_t>(m_header, 4); }
  uint8_t module() const { return m_decoder.get<uint8_t>(m_header, 5); }
  uint8_t nSlice2() const { return m_decoder.get<uint8_t>(m_header, 6); }
  uint8_t nSlice1() const { return m_decoder.get<uint8_t>(m_header, 7); }

  bool isValid() const { return length() == 0xf;}


};



} // end namespace

#endif
