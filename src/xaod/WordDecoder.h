#ifndef TRIGT1CALOBYTESTREAM_WORDDECODER_H
#define TRIGT1CALOBYTESTREAM_WORDDECODER_H

#include <string>
#include <vector>

namespace LVL1BS {

class BitField {
private:
  std::string m_name;
  uint32_t m_mask;
  uint8_t m_shift;
public:
  BitField(const std::string& name, uint8_t begin, uint8_t size);

  template<typename T> T get(const uint32_t& word) const {
    return T((word >> m_shift) & m_mask);
  }

};


class WordDecoder {
private:
  std::vector<BitField> m_bitFields;
public:
  WordDecoder(const std::vector<BitField>& bitFields):m_bitFields(bitFields){};
  template<typename T> T get(const uint32_t& word, uint8_t index) const{
    return m_bitFields[index].get<T>(word);
  }
};

}

#endif
