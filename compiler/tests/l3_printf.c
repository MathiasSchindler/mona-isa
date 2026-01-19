#include "clib.h"

int main(){
  char msg[3];
  msg[0] = 79;
  msg[1] = 75;
  msg[2] = 0;
  printf("S=%s C=%c D=%d\n", msg, 79, 123);
  return 0;
}
