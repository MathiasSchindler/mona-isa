.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t0, 1
  li t1, 2
  add t2, t0, t1
  li t3, 3
  add t4, t2, t3
  mov a0, t4
  ret
