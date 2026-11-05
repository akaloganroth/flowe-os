#include "../src/net/TrmnlBitmap.h"
#include <cassert>
#include <cstdio>
#include <cstring>
void put(uint8_t* h, int at, uint32_t n) { for (int i=0;i<4;i++) h[at+i]=n>>(8*i); }
int main() {
  uint8_t h[62]{};
  h[0]='B'; h[1]='M'; put(h,10,62); put(h,14,40);
  put(h,18,800); put(h,22,480); h[26]=1; h[28]=1;
  h[58]=h[59]=h[60]=255;
  trmnl::Bitmap b;
  assert(trmnl::parse(h,62,48062,b));
  assert(b.width==800 && b.height==480 && b.stride==100 && !b.top && b.zeroBlack);
  assert(!trmnl::parse(h,61,48062,b));
  assert(!trmnl::parse(h,62,48061,b));
  put(h,22,uint32_t(-480)); assert(trmnl::parse(h,62,48062,b) && b.top);
  put(h,18,793); assert(trmnl::parse(h,62,48062,b) && b.stride==100);
  h[54]=h[55]=h[56]=255; h[58]=h[59]=h[60]=0;
  assert(trmnl::parse(h,62,48062,b) && !b.zeroBlack);
  put(h,18,1601); assert(!trmnl::parse(h,62,400000,b));
  put(h,18,800); put(h,22,0x80000000); assert(!trmnl::parse(h,62,400000,b));
  put(h,22,480); h[28]=24; assert(!trmnl::parse(h,62,400000,b));
  h[28]=1; put(h,30,1); assert(!trmnl::parse(h,62,400000,b));
  put(h,30,0); put(h,10,0xfffffff0); assert(!trmnl::parse(h,62,400000,b));
  put(h,10,54); assert(!trmnl::parse(h,62,400000,b));
  puts("TRMNL bitmap validation passed");
}
