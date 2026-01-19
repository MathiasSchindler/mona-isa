#include "clib.h"

int main(){
  char buf[4];
  memset(buf, 65, 3);
  buf[3] = 0;
  if (buf[0] != 65) exit(1);
  if (buf[1] != 65) exit(1);
  if (buf[2] != 65) exit(1);
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
