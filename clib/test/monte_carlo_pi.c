#include "clib.h"

// Monte Carlo estimation of Pi using integer math.
// Expected output format:
// pi~3.xxx

static int next_rand(int seed){
  int x = seed * 1103515245 + 12345;
  if (x < 0) x = 0 - x;
  return x;
}

static int clamp_mod(int x, int m){
  while (x >= m) x = x - m;
  return x;
}

int main(){
  int total = 20000;
  int inside = 0;
  int seed = 1234567;
  int scale = 10000;
  int i = 0;
  while (i < total) {
    seed = next_rand(seed);
    int x = seed / 65536;
    x = clamp_mod(x, scale);
    seed = next_rand(seed);
    int y = seed / 65536;
    y = clamp_mod(y, scale);
    int r2 = x * x + y * y;
    if (r2 <= scale * scale) inside = inside + 1;
    i = i + 1;
  }

  int pi_scaled = (inside * 4 * 1000) / total;
  int intpart = pi_scaled / 1000;
  int rem = pi_scaled - intpart * 1000;
  printf("pi~%d.%d\n", intpart, rem, 0);
  return 0;
}
