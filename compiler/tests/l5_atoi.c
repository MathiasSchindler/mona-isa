#include "clib.h"

int main(){
  if (atoi("0") != 0) exit(1);
  if (atoi("42") != 42) exit(1);
  if (atoi("-7") != -7) exit(1);
  if (atoi("  15") != 15) exit(1);
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
