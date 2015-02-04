#include "WordDecoder.h"

#include <cstdlib>


using namespace LVL1BS;


BitField::BitField(const std::string& name, uint8_t begin, uint8_t size):
    m_name(name) {
  m_shift = begin;
  m_mask = (1 << size) - 1;
}
