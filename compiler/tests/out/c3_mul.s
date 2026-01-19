.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t0, 6
  li t1, 7
  mul t2, t0, t1
  mov a0, t2
  ret
