.data
.Lstr0:
  .byte 83
  .byte 61
  .byte 37
  .byte 115
  .byte 32
  .byte 67
  .byte 61
  .byte 37
  .byte 99
  .byte 32
  .byte 68
  .byte 61
  .byte 37
  .byte 100
  .byte 10
  .byte 0
.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  addi sp, sp, -136
  st r31, 0, sp
  addi t0, sp, 128
  li t1, 0
  li t2, 1
  mul t3, t1, t2
  add t4, t0, t3
  li t5, 79
  stb t5, 0, t4
  addi t6, sp, 128
  li t7, 1
  li t8, 1
  mul t9, t7, t8
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t6, t10
  st t9, 16, sp
  li t9, 75
  st t9, 24, sp
  ld t9, 16, sp
  ld t10, 24, sp
  stb t10, 0, t9
  addi t9, sp, 128
  st t9, 32, sp
  li t9, 2
  st t9, 40, sp
  li t9, 1
  st t9, 48, sp
  ld t9, 40, sp
  ld t10, 48, sp
  mul t9, t9, t10
  st t9, 56, sp
  ld t9, 32, sp
  ld t10, 56, sp
  add t9, t9, t10
  st t9, 64, sp
  li t9, 0
  st t9, 72, sp
  ld t9, 64, sp
  ld t10, 72, sp
  stb t10, 0, t9
  li t9, .Lstr0
  st t9, 80, sp
  addi t9, sp, 128
  st t9, 88, sp
  li t9, 79
  st t9, 96, sp
  li t9, 123
  st t9, 104, sp
  ld t9, 80, sp
  mov a0, t9
  ld t9, 88, sp
  mov a1, t9
  ld t9, 96, sp
  mov a2, t9
  ld t9, 104, sp
  mov a3, t9
  jal ra, printf
  mov t9, a0
  st t9, 112, sp
  li t9, 0
  st t9, 120, sp
  ld t9, 120, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 136
  ret
