#include "clib.h"

int main(){
  if (!isdigit(48)) exit(1);
  if (isdigit(65)) exit(1);
  if (!isalpha(65)) exit(1);
  if (!isalpha(97)) exit(1);
  if (isalpha(48)) exit(1);
  if (!isspace(32)) exit(1);
  if (!isspace(10)) exit(1);
  if (isspace(65)) exit(1);
  putchar(79);
  putchar(75);
  putchar(10);
  return 0;
}
