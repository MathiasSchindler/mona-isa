.data
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
  addi sp, sp, -352
  st r31, 0, sp
  li t0, 48
  mov a0, t0
  jal ra, isdigit
  mov t1, a0
  li t2, 0
  xor t3, t1, t2
  sltiu t3, t3, 1
  beq t3, zero, .L0
  li t4, 1
  mov r10, t4
  li r17, 3
  ecall
  li t5, 0
.L0:
  li t6, 65
  mov a0, t6
  jal ra, isdigit
  mov t7, a0
  beq t7, zero, .L2
  li t8, 1
  mov r10, t8
  li r17, 3
  ecall
  li t9, 0
  st t9, 8, sp
.L2:
  li t9, 65
  st t9, 16, sp
  ld t9, 16, sp
  mov a0, t9
  jal ra, isalpha
  mov t9, a0
  st t9, 24, sp
  li t9, 0
  st t9, 32, sp
  ld t9, 24, sp
  ld t10, 32, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 40, sp
  ld t9, 40, sp
  beq t9, zero, .L4
  li t9, 1
  st t9, 48, sp
  ld t9, 48, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 56, sp
.L4:
  li t9, 97
  st t9, 64, sp
  ld t9, 64, sp
  mov a0, t9
  jal ra, isalpha
  mov t9, a0
  st t9, 72, sp
  li t9, 0
  st t9, 80, sp
  ld t9, 72, sp
  ld t10, 80, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 88, sp
  ld t9, 88, sp
  beq t9, zero, .L6
  li t9, 1
  st t9, 96, sp
  ld t9, 96, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 104, sp
.L6:
  li t9, 48
  st t9, 112, sp
  ld t9, 112, sp
  mov a0, t9
  jal ra, isalpha
  mov t9, a0
  st t9, 120, sp
  ld t9, 120, sp
  beq t9, zero, .L8
  li t9, 1
  st t9, 128, sp
  ld t9, 128, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 136, sp
.L8:
  li t9, 32
  st t9, 144, sp
  ld t9, 144, sp
  mov a0, t9
  jal ra, isspace
  mov t9, a0
  st t9, 152, sp
  li t9, 0
  st t9, 160, sp
  ld t9, 152, sp
  ld t10, 160, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 168, sp
  ld t9, 168, sp
  beq t9, zero, .L10
  li t9, 1
  st t9, 176, sp
  ld t9, 176, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 184, sp
.L10:
  li t9, 10
  st t9, 192, sp
  ld t9, 192, sp
  mov a0, t9
  jal ra, isspace
  mov t9, a0
  st t9, 200, sp
  li t9, 0
  st t9, 208, sp
  ld t9, 200, sp
  ld t10, 208, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 216, sp
  ld t9, 216, sp
  beq t9, zero, .L12
  li t9, 1
  st t9, 224, sp
  ld t9, 224, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 232, sp
.L12:
  li t9, 65
  st t9, 240, sp
  ld t9, 240, sp
  mov a0, t9
  jal ra, isspace
  mov t9, a0
  st t9, 248, sp
  ld t9, 248, sp
  beq t9, zero, .L14
  li t9, 1
  st t9, 256, sp
  ld t9, 256, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 264, sp
.L14:
  li t9, 79
  st t9, 272, sp
  li t9, .Lputc0
  st t9, 280, sp
  ld t9, 280, sp
  ld t10, 272, sp
  stb t10, 0, t9
  ld t9, 280, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 288, sp
  li t9, 75
  st t9, 296, sp
  li t9, .Lputc1
  st t9, 304, sp
  ld t9, 304, sp
  ld t10, 296, sp
  stb t10, 0, t9
  ld t9, 304, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 312, sp
  li t9, 10
  st t9, 320, sp
  li t9, .Lputc2
  st t9, 328, sp
  ld t9, 328, sp
  ld t10, 320, sp
  stb t10, 0, t9
  ld t9, 328, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 336, sp
  li t9, 0
  st t9, 344, sp
  ld t9, 344, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 352
  ret
