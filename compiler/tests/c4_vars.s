.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t1, 3
  mov t0, t1
  li t3, 4
  mov t2, t3
  add t4, t0, t2
  mov a0, t4
  ret
