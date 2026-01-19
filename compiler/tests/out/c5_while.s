.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t1, 0
  mov t0, t1
  li t3, 0
  mov t2, t3
.L0:
  li t4, 5
  slt t5, t0, t4
  beq t5, zero, .L1
  add t6, t2, t0
  mov t2, t6
  li t7, 1
  add t8, t0, t7
  mov t0, t8
  j .L0
.L1:
  mov a0, t2
  ret
