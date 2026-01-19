#include "clib.h"

// Sieve of Eratosthenes up to 100
// Expected output (exact):
// count=25 last=97

int main(){
  int n = 100;
  char isprime[101];
  int i = 0;
  while (i <= n) {
    isprime[i] = 1;
    i = i + 1;
  }
  isprime[0] = 0;
  isprime[1] = 0;

  int p = 2;
  while (p * p <= n) {
    if (isprime[p]) {
      int j = p * p;
      while (j <= n) {
        isprime[j] = 0;
        j = j + p;
      }
    }
    p = p + 1;
  }

  int count = 0;
  int last = 0;
  i = 2;
  while (i <= n) {
    if (isprime[i]) {
      count = count + 1;
      last = i;
    }
    i = i + 1;
  }

  printf("count=%d ", count, 0, 0);
  printf("last=%d\n", last, 0, 0);
  return 0;
}
