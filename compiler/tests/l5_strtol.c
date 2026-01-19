#include "clib.h"

int main(){
  int end = 0;
  int v = strtol("123x", &end, 10);
  if (v != 123) exit(1);
  if (end != 3) exit(1);
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
