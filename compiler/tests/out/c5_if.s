.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t1, 3
  mov t0, t1
  li t2, 5
  slt t3, t0, t2
  beq t3, zero, .L0
  li t4, 1
  mov a0, t4
  ret
  j .L1
.L0:
  li t5, 2
  mov a0, t5
  ret
.L1:
