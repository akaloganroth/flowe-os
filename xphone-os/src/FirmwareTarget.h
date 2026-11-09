#pragma once
#include <cstddef>
#include <cstdint>
namespace firmware_target {
inline bool matches(const uint8_t* header, size_t size, uint16_t expectedChip) {
  return size >= 24 && header[0] == 0xE9 &&
         (uint16_t(header[12]) | (uint16_t(header[13]) << 8)) == expectedChip;
}
}
