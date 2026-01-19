.data
.Lstr0:
  .byte 48
  .byte 0
.Lstr1:
  .byte 52
  .byte 50
  .byte 0
.Lstr2:
  .byte 45
  .byte 55
  .byte 0
.Lstr3:
  .byte 32
  .byte 32
  .byte 49
  .byte 53
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
  addi sp, sp, -224
  st r31, 0, sp
  li t0, .Lstr0
  mov a0, t0
  jal ra, atoi
  mov t1, a0
  li t2, 0
  xor t3, t1, t2
  sltu t3, zero, t3
  beq t3, zero, .L0
  li t4, 1
  mov r10, t4
  li r17, 3
  ecall
  li t5, 0
.L0:
  li t6, .Lstr1
  mov a0, t6
  jal ra, atoi
  mov t7, a0
  li t8, 42
  xor t9, t7, t8
  sltu t9, zero, t9
  st t9, 8, sp
  ld t9, 8, sp
  beq t9, zero, .L2
  li t9, 1
  st t9, 16, sp
  ld t9, 16, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 24, sp
.L2:
  li t9, .Lstr2
  st t9, 32, sp
  ld t9, 32, sp
  mov a0, t9
  jal ra, atoi
  mov t9, a0
  st t9, 40, sp
  li t9, 7
  st t9, 48, sp
  li t9, 0
  st t9, 56, sp
  ld t9, 56, sp
  ld t10, 48, sp
  sub t9, t9, t10
  st t9, 64, sp
  ld t9, 40, sp
  ld t10, 64, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 72, sp
  ld t9, 72, sp
  beq t9, zero, .L4
  li t9, 1
  st t9, 80, sp
  ld t9, 80, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 88, sp
.L4:
  li t9, .Lstr3
  st t9, 96, sp
  ld t9, 96, sp
  mov a0, t9
  jal ra, atoi
  mov t9, a0
  st t9, 104, sp
  li t9, 15
  st t9, 112, sp
  ld t9, 104, sp
  ld t10, 112, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 120, sp
  ld t9, 120, sp
  beq t9, zero, .L6
  li t9, 1
  st t9, 128, sp
  ld t9, 128, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 136, sp
.L6:
  li t9, 79
  st t9, 144, sp
  li t9, .Lputc0
  st t9, 152, sp
  ld t9, 152, sp
  ld t10, 144, sp
  stb t10, 0, t9
  ld t9, 152, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 160, sp
  li t9, 75
  st t9, 168, sp
  li t9, .Lputc1
  st t9, 176, sp
  ld t9, 176, sp
  ld t10, 168, sp
  stb t10, 0, t9
  ld t9, 176, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 184, sp
  li t9, 10
  st t9, 192, sp
  li t9, .Lputc2
  st t9, 200, sp
  ld t9, 200, sp
  ld t10, 192, sp
  stb t10, 0, t9
  ld t9, 200, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 208, sp
  li t9, 0
  st t9, 216, sp
  ld t9, 216, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 224
  ret
