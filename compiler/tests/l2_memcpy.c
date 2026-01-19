#include "clib.h"

int main(){
  char src[3];
  char dst[3];
  src[0] = 79;
  src[1] = 75;
  src[2] = 0;
  memcpy(dst, src, 3);
  if (dst[0] != 79) exit(1);
  if (dst[1] != 75) exit(1);
  if (dst[2] != 0) exit(1);
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
