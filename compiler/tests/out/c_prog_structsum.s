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
sum_items:
  addi sp, sp, -48
  mov t0, a0
  mov t1, a1
  li t3, 0
  mov t2, t3
  li t5, 0
  mov t4, t5
  li t6, 0
  mov t2, t6
.L0:
  slt t7, t2, t1
  beq t7, zero, .L2
  li t8, 16
  mul t9, t2, t8
  st t9, 0, sp
  ld t10, 0, sp
  add t9, t0, t10
  st t9, 8, sp
  ld t9, 8, sp
  ld t10, 0, t9
  st t10, 16, sp
  ld t10, 16, sp
  add t9, t4, t10
  st t9, 24, sp
  ld t9, 24, sp
  mov t4, t9
.L1:
  li t9, 1
  st t9, 32, sp
  ld t10, 32, sp
  add t9, t2, t10
  st t9, 40, sp
  ld t9, 40, sp
  mov t2, t9
  j .L0
.L2:
  mov a0, t4
  addi sp, sp, 48
  ret
main:
  addi sp, sp, -584
  st r31, 0, sp
  addi t0, sp, 520
  li t1, 0
  li t2, 16
  mul t3, t1, t2
  add t4, t0, t3
  li t5, 10
  st t5, 0, t4
  addi t6, sp, 520
  li t7, 0
  li t8, 16
  mul t9, t7, t8
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t6, t10
  st t9, 16, sp
  li t9, 8
  st t9, 24, sp
  ld t9, 16, sp
  ld t10, 24, sp
  add t9, t9, t10
  st t9, 32, sp
  li t9, 65
  st t9, 40, sp
  ld t9, 32, sp
  ld t10, 40, sp
  stb t10, 0, t9
  addi t9, sp, 520
  st t9, 48, sp
  li t9, 1
  st t9, 56, sp
  li t9, 16
  st t9, 64, sp
  ld t9, 56, sp
  ld t10, 64, sp
  mul t9, t9, t10
  st t9, 72, sp
  ld t9, 48, sp
  ld t10, 72, sp
  add t9, t9, t10
  st t9, 80, sp
  li t9, 20
  st t9, 88, sp
  ld t9, 80, sp
  ld t10, 88, sp
  st t10, 0, t9
  addi t9, sp, 520
  st t9, 96, sp
  li t9, 1
  st t9, 104, sp
  li t9, 16
  st t9, 112, sp
  ld t9, 104, sp
  ld t10, 112, sp
  mul t9, t9, t10
  st t9, 120, sp
  ld t9, 96, sp
  ld t10, 120, sp
  add t9, t9, t10
  st t9, 128, sp
  li t9, 8
  st t9, 136, sp
  ld t9, 128, sp
  ld t10, 136, sp
  add t9, t9, t10
  st t9, 144, sp
  li t9, 66
  st t9, 152, sp
  ld t9, 144, sp
  ld t10, 152, sp
  stb t10, 0, t9
  addi t9, sp, 520
  st t9, 160, sp
  li t9, 2
  st t9, 168, sp
  li t9, 16
  st t9, 176, sp
  ld t9, 168, sp
  ld t10, 176, sp
  mul t9, t9, t10
  st t9, 184, sp
  ld t9, 160, sp
  ld t10, 184, sp
  add t9, t9, t10
  st t9, 192, sp
  li t9, 30
  st t9, 200, sp
  ld t9, 192, sp
  ld t10, 200, sp
  st t10, 0, t9
  addi t9, sp, 520
  st t9, 208, sp
  li t9, 2
  st t9, 216, sp
  li t9, 16
  st t9, 224, sp
  ld t9, 216, sp
  ld t10, 224, sp
  mul t9, t9, t10
  st t9, 232, sp
  ld t9, 208, sp
  ld t10, 232, sp
  add t9, t9, t10
  st t9, 240, sp
  li t9, 8
  st t9, 248, sp
  ld t9, 240, sp
  ld t10, 248, sp
  add t9, t9, t10
  st t9, 256, sp
  li t9, 67
  st t9, 264, sp
  ld t9, 256, sp
  ld t10, 264, sp
  stb t10, 0, t9
  addi t9, sp, 520
  st t9, 272, sp
  li t9, 3
  st t9, 280, sp
  li t9, 16
  st t9, 288, sp
  ld t9, 280, sp
  ld t10, 288, sp
  mul t9, t9, t10
  st t9, 296, sp
  ld t9, 272, sp
  ld t10, 296, sp
  add t9, t9, t10
  st t9, 304, sp
  li t9, 40
  st t9, 312, sp
  ld t9, 304, sp
  ld t10, 312, sp
  st t10, 0, t9
  addi t9, sp, 520
  st t9, 320, sp
  li t9, 3
  st t9, 328, sp
  li t9, 16
  st t9, 336, sp
  ld t9, 328, sp
  ld t10, 336, sp
  mul t9, t9, t10
  st t9, 344, sp
  ld t9, 320, sp
  ld t10, 344, sp
  add t9, t9, t10
  st t9, 352, sp
  li t9, 8
  st t9, 360, sp
  ld t9, 352, sp
  ld t10, 360, sp
  add t9, t9, t10
  st t9, 368, sp
  li t9, 68
  st t9, 376, sp
  ld t9, 368, sp
  ld t10, 376, sp
  stb t10, 0, t9
  addi t9, sp, 520
  st t9, 392, sp
  li t9, 4
  st t9, 400, sp
  ld t9, 392, sp
  mov a0, t9
  ld t9, 400, sp
  mov a1, t9
  jal ra, sum_items
  mov t9, a0
  st t9, 408, sp
  ld t9, 408, sp
  st t9, 384, sp
  li t9, 100
  st t9, 416, sp
  ld t9, 384, sp
  ld t10, 416, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 424, sp
  ld t9, 424, sp
  beq t9, zero, .L0
  li t9, 70
  st t9, 432, sp
  li t9, .Lputc0
  st t9, 440, sp
  ld t9, 440, sp
  ld t10, 432, sp
  stb t10, 0, t9
  ld t9, 440, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 448, sp
  li t9, 1
  st t9, 456, sp
  ld t9, 456, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 584
  ret
.L0:
  li t9, 79
  st t9, 464, sp
  li t9, .Lputc1
  st t9, 472, sp
  ld t9, 472, sp
  ld t10, 464, sp
  stb t10, 0, t9
  ld t9, 472, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 480, sp
  li t9, 75
  st t9, 488, sp
  li t9, .Lputc2
  st t9, 496, sp
  ld t9, 496, sp
  ld t10, 488, sp
  stb t10, 0, t9
  ld t9, 496, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 504, sp
  li t9, 0
  st t9, 512, sp
  ld t9, 512, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 584
  ret
