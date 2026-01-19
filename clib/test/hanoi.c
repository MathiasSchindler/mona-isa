#include "clib.h"

// Tower of Hanoi solver for 4 disks.
// Expected output: 15 moves, ending with "move 1 A C".

static void move(int n, int from, int to, int aux){
  if (n == 0) return;
  move(n - 1, from, aux, to);
  printf("move %d %c %c\n", n, from, to);
  move(n - 1, aux, to, from);
}

int main(){
  move(4, 65, 67, 66); // A=65, B=66, C=67
  return 0;
}
