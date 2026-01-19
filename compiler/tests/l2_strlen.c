#include "clib.h"

int main(){
  char s[4];
  s[0] = 97;
  s[1] = 98;
  s[2] = 99;
  s[3] = 0;
  if (strlen(s) != 3) {
    exit(1);
  }
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
