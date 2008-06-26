#include "precomp.h"
#include "rand.h"

ULONG RAND()
{
  static ULONG x = 123456789, y = 362436069, z = 521288629, w = 88675123, v = 886756453;
  ULONG t = (x ^ ( x >> 7));
  x = y; y = z; z = w; w = v;
  v = (v ^ (v << 6)) ^ (t ^ (t << 13));
  return (y + y + 1) * v;
}
