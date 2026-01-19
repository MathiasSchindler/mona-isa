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
  li t1, 1
  mov t0, t1
  li t3, 2
  mov t2, t3
  mov a0, t0
  mov a1, t2
  jal ra, add
  mov t4, a0
  li t5, 3
  mov a0, t4
  mov a1, t5
  jal ra, add
  mov t6, a0
  mov a0, t6
  ld r31, 0, sp
  addi sp, sp, 8
  ret
