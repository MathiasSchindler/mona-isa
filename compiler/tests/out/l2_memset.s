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
  addi sp, sp, -344
  st r31, 0, sp
  addi t0, sp, 336
  li t1, 65
  li t2, 3
  mov a0, t0
  mov a1, t1
  mov a2, t2
  jal ra, memset
  mov t3, a0
  addi t4, sp, 336
  li t5, 3
  li t6, 1
  mul t7, t5, t6
  add t8, t4, t7
  li t9, 0
  st t9, 8, sp
  ld t10, 8, sp
  stb t10, 0, t8
  addi t9, sp, 336
  st t9, 16, sp
  li t9, 0
  st t9, 24, sp
  li t9, 1
  st t9, 32, sp
  ld t9, 24, sp
  ld t10, 32, sp
  mul t9, t9, t10
  st t9, 40, sp
  ld t9, 16, sp
  ld t10, 40, sp
  add t9, t9, t10
  st t9, 48, sp
  ld t9, 48, sp
  ldb t10, 0, t9
  st t10, 56, sp
  li t9, 65
  st t9, 64, sp
  ld t9, 56, sp
  ld t10, 64, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 72, sp
  ld t9, 72, sp
  beq t9, zero, .L0
  li t9, 1
  st t9, 80, sp
  ld t9, 80, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 88, sp
.L0:
  addi t9, sp, 336
  st t9, 96, sp
  li t9, 1
  st t9, 104, sp
  li t9, 1
  st t9, 112, sp
  ld t9, 104, sp
  ld t10, 112, sp
  mul t9, t9, t10
  st t9, 120, sp
  ld t9, 96, sp
  ld t10, 120, sp
  add t9, t9, t10
  st t9, 128, sp
  ld t9, 128, sp
  ldb t10, 0, t9
  st t10, 136, sp
  li t9, 65
  st t9, 144, sp
  ld t9, 136, sp
  ld t10, 144, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 152, sp
  ld t9, 152, sp
  beq t9, zero, .L2
  li t9, 1
  st t9, 160, sp
  ld t9, 160, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 168, sp
.L2:
  addi t9, sp, 336
  st t9, 176, sp
  li t9, 2
  st t9, 184, sp
  li t9, 1
  st t9, 192, sp
  ld t9, 184, sp
  ld t10, 192, sp
  mul t9, t9, t10
  st t9, 200, sp
  ld t9, 176, sp
  ld t10, 200, sp
  add t9, t9, t10
  st t9, 208, sp
  ld t9, 208, sp
  ldb t10, 0, t9
  st t10, 216, sp
  li t9, 65
  st t9, 224, sp
  ld t9, 216, sp
  ld t10, 224, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 232, sp
  ld t9, 232, sp
  beq t9, zero, .L4
  li t9, 1
  st t9, 240, sp
  ld t9, 240, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 248, sp
.L4:
  li t9, 79
  st t9, 256, sp
  li t9, .Lputc0
  st t9, 264, sp
  ld t9, 264, sp
  ld t10, 256, sp
  stb t10, 0, t9
  ld t9, 264, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 272, sp
  li t9, 75
  st t9, 280, sp
  li t9, .Lputc1
  st t9, 288, sp
  ld t9, 288, sp
  ld t10, 280, sp
  stb t10, 0, t9
  ld t9, 288, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 296, sp
  li t9, 10
  st t9, 304, sp
  li t9, .Lputc2
  st t9, 312, sp
  ld t9, 312, sp
  ld t10, 304, sp
  stb t10, 0, t9
  ld t9, 312, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 320, sp
  li t9, 0
  st t9, 328, sp
  ld t9, 328, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 344
  ret
