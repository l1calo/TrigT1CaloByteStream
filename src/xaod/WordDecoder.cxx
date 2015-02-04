#include "WordDecoder.h"

#include <cstdlib>


using namespace LVL1BS;


BitField::BitField(const std::string& name, uint8_t begin, uint8_t size):
    m_name(name) {
  m_shift = sizeof(uint32_t) * 8 - (begin+size);
  m_mask = std::strtol(std::string(size, '1').c_str(), NULL, 2);
}
