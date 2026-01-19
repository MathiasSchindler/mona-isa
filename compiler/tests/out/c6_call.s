.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
add:
  mov t0, a0
  mov t1, a1
  add t2, t0, t1
  mov a0, t2
  ret
main:
  addi sp, sp, -8
  st r31, 0, sp
  li t0, 2
  li t1, 3
  mov a0, t0
  mov a1, t1
  jal ra, add
  mov t2, a0
  mov a0, t2
  ld r31, 0, sp
  addi sp, sp, 8
  ret
