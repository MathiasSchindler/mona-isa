.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  li t0, 7
  mov a0, t0
  ret
