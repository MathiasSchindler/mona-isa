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
  addi sp, sp, -448
  st r31, 0, sp
  addi t0, sp, 432
  li t1, 0
  li t2, 1
  mul t3, t1, t2
  add t4, t0, t3
  li t5, 79
  stb t5, 0, t4
  addi t6, sp, 432
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
  addi t9, sp, 432
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
  addi t9, sp, 440
  st t9, 80, sp
  addi t9, sp, 432
  st t9, 88, sp
  li t9, 3
  st t9, 96, sp
  ld t9, 80, sp
  mov a0, t9
  ld t9, 88, sp
  mov a1, t9
  ld t9, 96, sp
  mov a2, t9
  jal ra, memcpy
  mov t9, a0
  st t9, 104, sp
  addi t9, sp, 440
  st t9, 112, sp
  li t9, 0
  st t9, 120, sp
  li t9, 1
  st t9, 128, sp
  ld t9, 120, sp
  ld t10, 128, sp
  mul t9, t9, t10
  st t9, 136, sp
  ld t9, 112, sp
  ld t10, 136, sp
  add t9, t9, t10
  st t9, 144, sp
  ld t9, 144, sp
  ldb t10, 0, t9
  st t10, 152, sp
  li t9, 79
  st t9, 160, sp
  ld t9, 152, sp
  ld t10, 160, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 168, sp
  ld t9, 168, sp
  beq t9, zero, .L0
  li t9, 1
  st t9, 176, sp
  ld t9, 176, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 184, sp
.L0:
  addi t9, sp, 440
  st t9, 192, sp
  li t9, 1
  st t9, 200, sp
  li t9, 1
  st t9, 208, sp
  ld t9, 200, sp
  ld t10, 208, sp
  mul t9, t9, t10
  st t9, 216, sp
  ld t9, 192, sp
  ld t10, 216, sp
  add t9, t9, t10
  st t9, 224, sp
  ld t9, 224, sp
  ldb t10, 0, t9
  st t10, 232, sp
  li t9, 75
  st t9, 240, sp
  ld t9, 232, sp
  ld t10, 240, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 248, sp
  ld t9, 248, sp
  beq t9, zero, .L2
  li t9, 1
  st t9, 256, sp
  ld t9, 256, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 264, sp
.L2:
  addi t9, sp, 440
  st t9, 272, sp
  li t9, 2
  st t9, 280, sp
  li t9, 1
  st t9, 288, sp
  ld t9, 280, sp
  ld t10, 288, sp
  mul t9, t9, t10
  st t9, 296, sp
  ld t9, 272, sp
  ld t10, 296, sp
  add t9, t9, t10
  st t9, 304, sp
  ld t9, 304, sp
  ldb t10, 0, t9
  st t10, 312, sp
  li t9, 0
  st t9, 320, sp
  ld t9, 312, sp
  ld t10, 320, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 328, sp
  ld t9, 328, sp
  beq t9, zero, .L4
  li t9, 1
  st t9, 336, sp
  ld t9, 336, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 344, sp
.L4:
  li t9, 79
  st t9, 352, sp
  li t9, .Lputc0
  st t9, 360, sp
  ld t9, 360, sp
  ld t10, 352, sp
  stb t10, 0, t9
  ld t9, 360, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 368, sp
  li t9, 75
  st t9, 376, sp
  li t9, .Lputc1
  st t9, 384, sp
  ld t9, 384, sp
  ld t10, 376, sp
  stb t10, 0, t9
  ld t9, 384, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 392, sp
  li t9, 10
  st t9, 400, sp
  li t9, .Lputc2
  st t9, 408, sp
  ld t9, 408, sp
  ld t10, 400, sp
  stb t10, 0, t9
  ld t9, 408, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 416, sp
  li t9, 0
  st t9, 424, sp
  ld t9, 424, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 448
  ret
