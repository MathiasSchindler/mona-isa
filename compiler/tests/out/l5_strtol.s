.data
.Lstr0:
  .byte 49
  .byte 50
  .byte 51
  .byte 120
  .byte 0
.Lputc0:
  .byte 0
.Lputc1:
  .byte 0
.Lputc2:
  .byte 0
.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  addi sp, sp, -160
  st r31, 0, sp
  li t1, 0
  mov t0, t1
  li t3, .Lstr0
  addi t4, sp, 152
  st t0, 0, t4
  li t5, 10
  mov a0, t3
  mov a1, t4
  mov a2, t5
  jal ra, strtol
  mov t6, a0
  mov t2, t6
  li t7, 123
  xor t8, t2, t7
  sltu t8, zero, t8
  beq t8, zero, .L0
  li t9, 1
  st t9, 8, sp
  ld t9, 8, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 16, sp
.L0:
  addi t9, sp, 152
  st t9, 24, sp
  ld t9, 24, sp
  ld t10, 0, t9
  st t10, 32, sp
  li t9, 3
  st t9, 40, sp
  ld t9, 32, sp
  ld t10, 40, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 48, sp
  ld t9, 48, sp
  beq t9, zero, .L2
  li t9, 1
  st t9, 56, sp
  ld t9, 56, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 64, sp
.L2:
  li t9, 79
  st t9, 72, sp
  li t9, .Lputc0
  st t9, 80, sp
  ld t9, 80, sp
  ld t10, 72, sp
  stb t10, 0, t9
  ld t9, 80, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 88, sp
  li t9, 75
  st t9, 96, sp
  li t9, .Lputc1
  st t9, 104, sp
  ld t9, 104, sp
  ld t10, 96, sp
  stb t10, 0, t9
  ld t9, 104, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 112, sp
  li t9, 10
  st t9, 120, sp
  li t9, .Lputc2
  st t9, 128, sp
  ld t9, 128, sp
  ld t10, 120, sp
  stb t10, 0, t9
  ld t9, 128, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 136, sp
  li t9, 0
  st t9, 144, sp
  ld t9, 144, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 160
  ret
