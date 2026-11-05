#pragma once
#include <cstdint>
#include <cstddef>
namespace trmnl {
inline uint16_t u16(const uint8_t* p) { return p[0] | (uint16_t(p[1]) << 8); }
inline uint32_t u32(const uint8_t* p) { return u16(p) | (uint32_t(u16(p + 2)) << 16); }
struct Bitmap { uint32_t offset, stride; int32_t width, height; bool top, zeroBlack; };
inline bool parse(const uint8_t* h, size_t length, uint64_t fileSize, Bitmap& b) {
  if (length < 62) return false;
  b.width = int32_t(u32(h + 18));
  const int32_t height = int32_t(u32(h + 22));
  if (h[0] != 'B' || h[1] != 'M' || u32(h + 14) != 40 ||
      u16(h + 26) != 1 || u16(h + 28) != 1 || u32(h + 30) != 0 ||
      b.width < 1 || b.width > 1600 || height == 0 || height < -1600 || height > 1600) return false;
  b.top = height < 0;
  b.height = b.top ? -height : height;
  b.offset = u32(h + 10);
  b.stride = ((b.width + 31) / 32) * 4;
  b.zeroBlack = unsigned(h[54]) + h[55] + h[56] < unsigned(h[58]) + h[59] + h[60];
  return b.offset >= 62 && uint64_t(b.offset) + uint64_t(b.stride) * b.height <= fileSize;
}
}
