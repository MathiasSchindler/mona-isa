.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t1, 1
  mov t0, t1
  li t2, 5
  mov t0, t2
  mov a0, t0
  ret
