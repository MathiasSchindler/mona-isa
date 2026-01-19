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
.data
.Lclibputc0:
  .byte 0
.Lclibputc1:
  .byte 0
.Lclibputc2:
  .byte 0
.Lclibputc3:
  .byte 0
.Lclibstr0:
  .byte 0
.Lclibputc4:
  .byte 0
.Lclibputc5:
  .byte 0
.Lclibputc6:
  .byte 0
.Lclibputc7:
  .byte 0
.Lclibputc8:
  .byte 0
.text
strlen:
  addi sp, sp, -40
  mov t0, a0
  li t2, 0
  mov t1, t2
  li t3, 0
  xor t4, t0, t3
  sltiu t4, t4, 1
  beq t4, zero, .Lclib0
  li t5, 0
  mov a0, t5
  addi sp, sp, 40
  ret
.Lclib0:
.Lclib2:
  li t6, 1
  mul t7, t1, t6
  add t8, t0, t7
  ldb t10, 0, t8
  st t10, 0, sp
  li t9, 0
  st t9, 8, sp
  ld t9, 0, sp
  ld t10, 8, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 16, sp
  ld t9, 16, sp
  beq t9, zero, .Lclib3
  li t9, 1
  st t9, 24, sp
  ld t10, 24, sp
  add t9, t1, t10
  st t9, 32, sp
  ld t9, 32, sp
  mov t1, t9
  j .Lclib2
.Lclib3:
  mov a0, t1
  addi sp, sp, 40
  ret
memcpy:
  addi sp, sp, -48
  mov t0, a0
  mov t1, a1
  mov t2, a2
  li t4, 0
  mov t3, t4
.Lclib4:
  slt t5, t3, t2
  beq t5, zero, .Lclib5
  li t6, 1
  mul t7, t3, t6
  add t8, t0, t7
  li t9, 1
  st t9, 0, sp
  ld t10, 0, sp
  mul t9, t3, t10
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t1, t10
  st t9, 16, sp
  ld t9, 16, sp
  ldb t10, 0, t9
  st t10, 24, sp
  ld t10, 24, sp
  stb t10, 0, t8
  li t9, 1
  st t9, 32, sp
  ld t10, 32, sp
  add t9, t3, t10
  st t9, 40, sp
  ld t9, 40, sp
  mov t3, t9
  j .Lclib4
.Lclib5:
  mov a0, t0
  addi sp, sp, 48
  ret
memset:
  addi sp, sp, -24
  mov t0, a0
  mov t1, a1
  mov t2, a2
  mov t3, t1
  li t5, 0
  mov t4, t5
.Lclib6:
  slt t6, t4, t2
  beq t6, zero, .Lclib7
  li t7, 1
  mul t8, t4, t7
  add t9, t0, t8
  st t9, 0, sp
  ld t9, 0, sp
  stb t3, 0, t9
  li t9, 1
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t4, t10
  st t9, 16, sp
  ld t9, 16, sp
  mov t4, t9
  j .Lclib6
.Lclib7:
  mov a0, t0
  addi sp, sp, 24
  ret
print_int:
  addi sp, sp, -360
  mov t0, a0
  li t2, 0
  mov t1, t2
  li t4, 0
  mov t3, t4
  li t5, 0
  xor t6, t0, t5
  sltiu t6, t6, 1
  beq t6, zero, .Lclib8
  li t7, 48
  li t8, .Lclibputc0
  stb t7, 0, t8
  li r10, 1
  mov r11, t8
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 0, sp
  li t9, 1
  st t9, 8, sp
  ld t9, 8, sp
  mov a0, t9
  addi sp, sp, 360
  ret
.Lclib8:
  li t9, 0
  st t9, 16, sp
  ld t10, 16, sp
  slt t9, t0, t10
  st t9, 24, sp
  ld t9, 24, sp
  beq t9, zero, .Lclib10
  li t9, 45
  st t9, 32, sp
  li t9, .Lclibputc1
  st t9, 40, sp
  ld t9, 40, sp
  ld t10, 32, sp
  stb t10, 0, t9
  ld t9, 40, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 48, sp
  li t9, 1
  st t9, 56, sp
  ld t10, 56, sp
  add t9, t3, t10
  st t9, 64, sp
  ld t9, 64, sp
  mov t3, t9
  li t9, 0
  st t9, 72, sp
  ld t9, 72, sp
  sub t9, t9, t0
  st t9, 80, sp
  ld t9, 80, sp
  mov t0, t9
.Lclib10:
.Lclib12:
  li t9, 0
  st t9, 88, sp
  ld t10, 88, sp
  slt t9, t10, t0
  st t9, 96, sp
  ld t9, 96, sp
  beq t9, zero, .Lclib13
  li t9, 10
  st t9, 112, sp
  ld t10, 112, sp
  div t9, t0, t10
  st t9, 120, sp
  ld t9, 120, sp
  st t9, 104, sp
  li t9, 10
  st t9, 136, sp
  ld t9, 104, sp
  ld t10, 136, sp
  mul t9, t9, t10
  st t9, 144, sp
  ld t10, 144, sp
  sub t9, t0, t10
  st t9, 152, sp
  ld t9, 152, sp
  st t9, 128, sp
  addi t9, sp, 328
  st t9, 160, sp
  li t9, 1
  st t9, 168, sp
  ld t10, 168, sp
  mul t9, t1, t10
  st t9, 176, sp
  ld t9, 160, sp
  ld t10, 176, sp
  add t9, t9, t10
  st t9, 184, sp
  li t9, 48
  st t9, 192, sp
  ld t9, 128, sp
  ld t10, 192, sp
  add t9, t9, t10
  st t9, 200, sp
  ld t9, 184, sp
  ld t10, 200, sp
  stb t10, 0, t9
  li t9, 1
  st t9, 208, sp
  ld t10, 208, sp
  add t9, t1, t10
  st t9, 216, sp
  ld t9, 216, sp
  mov t1, t9
  ld t9, 104, sp
  mov t0, t9
  j .Lclib12
.Lclib13:
.Lclib14:
  li t9, 0
  st t9, 224, sp
  ld t10, 224, sp
  slt t9, t10, t1
  st t9, 232, sp
  ld t9, 232, sp
  beq t9, zero, .Lclib15
  li t9, 1
  st t9, 240, sp
  ld t10, 240, sp
  sub t9, t1, t10
  st t9, 248, sp
  ld t9, 248, sp
  mov t1, t9
  addi t9, sp, 328
  st t9, 256, sp
  li t9, 1
  st t9, 264, sp
  ld t10, 264, sp
  mul t9, t1, t10
  st t9, 272, sp
  ld t9, 256, sp
  ld t10, 272, sp
  add t9, t9, t10
  st t9, 280, sp
  ld t9, 280, sp
  ldb t10, 0, t9
  st t10, 288, sp
  li t9, .Lclibputc2
  st t9, 296, sp
  ld t9, 296, sp
  ld t10, 288, sp
  stb t10, 0, t9
  ld t9, 296, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 304, sp
  li t9, 1
  st t9, 312, sp
  ld t10, 312, sp
  add t9, t3, t10
  st t9, 320, sp
  ld t9, 320, sp
  mov t3, t9
  j .Lclib14
.Lclib15:
  mov a0, t3
  addi sp, sp, 360
  ret
vprintf:
  addi sp, sp, -624
  st r31, 0, sp
  mov t0, a0
  mov t1, a1
  mov t2, a2
  li t4, 0
  mov t3, t4
  li t6, 0
  mov t5, t6
  li t8, 0
  mov t7, t8
  li t9, 0
  st t9, 8, sp
  ld t10, 8, sp
  xor t9, t0, t10
  sltiu t9, t9, 1
  st t9, 16, sp
  ld t9, 16, sp
  beq t9, zero, .Lclib16
  li t9, 0
  st t9, 24, sp
  ld t9, 24, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 624
  ret
.Lclib16:
.Lclib18:
  li t9, 1
  st t9, 32, sp
  ld t10, 32, sp
  mul t9, t3, t10
  st t9, 40, sp
  ld t10, 40, sp
  add t9, t0, t10
  st t9, 48, sp
  ld t9, 48, sp
  ldb t10, 0, t9
  st t10, 56, sp
  li t9, 0
  st t9, 64, sp
  ld t9, 56, sp
  ld t10, 64, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 72, sp
  ld t9, 72, sp
  beq t9, zero, .Lclib19
  li t9, 1
  st t9, 88, sp
  ld t10, 88, sp
  mul t9, t3, t10
  st t9, 96, sp
  ld t10, 96, sp
  add t9, t0, t10
  st t9, 104, sp
  ld t9, 104, sp
  ldb t10, 0, t9
  st t10, 112, sp
  ld t9, 112, sp
  st t9, 80, sp
  li t9, 37
  st t9, 120, sp
  ld t9, 80, sp
  ld t10, 120, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 128, sp
  ld t9, 128, sp
  beq t9, zero, .Lclib20
  li t9, 1
  st t9, 136, sp
  ld t10, 136, sp
  add t9, t3, t10
  st t9, 144, sp
  ld t9, 144, sp
  mov t3, t9
  li t9, 1
  st t9, 160, sp
  ld t10, 160, sp
  mul t9, t3, t10
  st t9, 168, sp
  ld t10, 168, sp
  add t9, t0, t10
  st t9, 176, sp
  ld t9, 176, sp
  ldb t10, 0, t9
  st t10, 184, sp
  ld t9, 184, sp
  st t9, 152, sp
  li t9, 0
  st t9, 192, sp
  ld t9, 152, sp
  ld t10, 192, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 200, sp
  ld t9, 200, sp
  beq t9, zero, .Lclib22
  j .Lclib19
.Lclib22:
  li t9, 37
  st t9, 208, sp
  ld t9, 152, sp
  ld t10, 208, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 216, sp
  ld t9, 216, sp
  beq t9, zero, .Lclib24
  li t9, 37
  st t9, 224, sp
  li t9, .Lclibputc3
  st t9, 232, sp
  ld t9, 232, sp
  ld t10, 224, sp
  stb t10, 0, t9
  ld t9, 232, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 240, sp
  li t9, 1
  st t9, 248, sp
  ld t10, 248, sp
  add t9, t7, t10
  st t9, 256, sp
  ld t9, 256, sp
  mov t7, t9
  j .Lclib25
.Lclib24:
  slt t9, t5, t2
  st t9, 264, sp
  ld t9, 264, sp
  beq t9, zero, .Lclib26
  li t9, 8
  st t9, 280, sp
  ld t10, 280, sp
  mul t9, t5, t10
  st t9, 288, sp
  ld t10, 288, sp
  add t9, t1, t10
  st t9, 296, sp
  ld t9, 296, sp
  ld t10, 0, t9
  st t10, 304, sp
  ld t9, 304, sp
  st t9, 272, sp
  li t9, 115
  st t9, 312, sp
  ld t9, 152, sp
  ld t10, 312, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 320, sp
  ld t9, 320, sp
  beq t9, zero, .Lclib28
  ld t9, 272, sp
  st t9, 328, sp
  li t9, 0
  st t9, 336, sp
  ld t9, 328, sp
  ld t10, 336, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 344, sp
  ld t9, 344, sp
  beq t9, zero, .Lclib30
  li t9, .Lclibstr0
  st t9, 352, sp
  ld t9, 352, sp
  st t9, 328, sp
.Lclib30:
.Lclib32:
  ld t9, 328, sp
  ldb t10, 0, t9
  st t10, 360, sp
  ld t9, 360, sp
  beq t9, zero, .Lclib33
  ld t9, 328, sp
  ldb t10, 0, t9
  st t10, 368, sp
  li t9, .Lclibputc4
  st t9, 376, sp
  ld t9, 376, sp
  ld t10, 368, sp
  stb t10, 0, t9
  ld t9, 376, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 384, sp
  li t9, 1
  st t9, 392, sp
  ld t10, 392, sp
  add t9, t7, t10
  st t9, 400, sp
  ld t9, 400, sp
  mov t7, t9
  li t9, 1
  st t9, 408, sp
  ld t9, 328, sp
  ld t10, 408, sp
  add t9, t9, t10
  st t9, 416, sp
  ld t9, 416, sp
  st t9, 328, sp
  j .Lclib32
.Lclib33:
  j .Lclib29
.Lclib28:
  li t9, 99
  st t9, 424, sp
  ld t9, 152, sp
  ld t10, 424, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 432, sp
  ld t9, 432, sp
  beq t9, zero, .Lclib34
  li t9, .Lclibputc5
  st t9, 440, sp
  ld t9, 440, sp
  ld t10, 272, sp
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
  ld t10, 456, sp
  add t9, t7, t10
  st t9, 464, sp
  ld t9, 464, sp
  mov t7, t9
  j .Lclib35
.Lclib34:
  li t9, 100
  st t9, 472, sp
  ld t9, 152, sp
  ld t10, 472, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 480, sp
  ld t9, 480, sp
  beq t9, zero, .Lclib36
  ld t9, 272, sp
  mov a0, t9
  jal ra, print_int
  mov t9, a0
  st t9, 488, sp
  ld t10, 488, sp
  add t9, t7, t10
  st t9, 496, sp
  ld t9, 496, sp
  mov t7, t9
  j .Lclib37
.Lclib36:
  li t9, 37
  st t9, 504, sp
  li t9, .Lclibputc6
  st t9, 512, sp
  ld t9, 512, sp
  ld t10, 504, sp
  stb t10, 0, t9
  ld t9, 512, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 520, sp
  li t9, .Lclibputc7
  st t9, 528, sp
  ld t9, 528, sp
  ld t10, 152, sp
  stb t10, 0, t9
  ld t9, 528, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 536, sp
  li t9, 2
  st t9, 544, sp
  ld t10, 544, sp
  add t9, t7, t10
  st t9, 552, sp
  ld t9, 552, sp
  mov t7, t9
.Lclib37:
.Lclib35:
.Lclib29:
  li t9, 1
  st t9, 560, sp
  ld t10, 560, sp
  add t9, t5, t10
  st t9, 568, sp
  ld t9, 568, sp
  mov t5, t9
.Lclib26:
.Lclib25:
  j .Lclib21
.Lclib20:
  li t9, .Lclibputc8
  st t9, 576, sp
  ld t9, 576, sp
  ld t10, 80, sp
  stb t10, 0, t9
  ld t9, 576, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 584, sp
  li t9, 1
  st t9, 592, sp
  ld t10, 592, sp
  add t9, t7, t10
  st t9, 600, sp
  ld t9, 600, sp
  mov t7, t9
.Lclib21:
  li t9, 1
  st t9, 608, sp
  ld t10, 608, sp
  add t9, t3, t10
  st t9, 616, sp
  ld t9, 616, sp
  mov t3, t9
  j .Lclib18
.Lclib19:
  mov a0, t7
  ld r31, 0, sp
  addi sp, sp, 624
  ret
printf:
  addi sp, sp, -136
  st r31, 0, sp
  mov t0, a0
  mov t1, a1
  mov t2, a2
  mov t3, a3
  addi t4, sp, 112
  li t5, 0
  li t6, 8
  mul t7, t5, t6
  add t8, t4, t7
  st t1, 0, t8
  addi t9, sp, 112
  st t9, 8, sp
  li t9, 1
  st t9, 16, sp
  li t9, 8
  st t9, 24, sp
  ld t9, 16, sp
  ld t10, 24, sp
  mul t9, t9, t10
  st t9, 32, sp
  ld t9, 8, sp
  ld t10, 32, sp
  add t9, t9, t10
  st t9, 40, sp
  ld t9, 40, sp
  st t2, 0, t9
  addi t9, sp, 112
  st t9, 48, sp
  li t9, 2
  st t9, 56, sp
  li t9, 8
  st t9, 64, sp
  ld t9, 56, sp
  ld t10, 64, sp
  mul t9, t9, t10
  st t9, 72, sp
  ld t9, 48, sp
  ld t10, 72, sp
  add t9, t9, t10
  st t9, 80, sp
  ld t9, 80, sp
  st t3, 0, t9
  addi t9, sp, 112
  st t9, 88, sp
  li t9, 3
  st t9, 96, sp
  mov a0, t0
  ld t9, 88, sp
  mov a1, t9
  ld t9, 96, sp
  mov a2, t9
  jal ra, vprintf
  mov t9, a0
  st t9, 104, sp
  ld t9, 104, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 136
  ret
isdigit:
  addi sp, sp, -8
  mov t0, a0
  li t2, 48
  slt t3, t0, t2
  xori t3, t3, 1
  beq t3, zero, .Lclib38
  li t4, 57
  slt t5, t4, t0
  xori t5, t5, 1
  beq t5, zero, .Lclib38
  li t6, 1
  mov t1, t6
  j .Lclib39
.Lclib38:
  li t7, 0
  mov t1, t7
.Lclib39:
  beq t1, zero, .Lclib40
  li t8, 1
  mov a0, t8
  addi sp, sp, 8
  ret
.Lclib40:
  li t9, 0
  st t9, 0, sp
  ld t9, 0, sp
  mov a0, t9
  addi sp, sp, 8
  ret
isalpha:
  addi sp, sp, -72
  mov t0, a0
  li t2, 65
  slt t3, t0, t2
  xori t3, t3, 1
  beq t3, zero, .Lclib42
  li t4, 90
  slt t5, t4, t0
  xori t5, t5, 1
  beq t5, zero, .Lclib42
  li t6, 1
  mov t1, t6
  j .Lclib43
.Lclib42:
  li t7, 0
  mov t1, t7
.Lclib43:
  beq t1, zero, .Lclib44
  li t8, 1
  mov a0, t8
  addi sp, sp, 72
  ret
.Lclib44:
  li t9, 97
  st t9, 8, sp
  ld t10, 8, sp
  slt t9, t0, t10
  xori t9, t9, 1
  st t9, 16, sp
  ld t9, 16, sp
  beq t9, zero, .Lclib46
  li t9, 122
  st t9, 24, sp
  ld t10, 24, sp
  slt t9, t10, t0
  xori t9, t9, 1
  st t9, 32, sp
  ld t9, 32, sp
  beq t9, zero, .Lclib46
  li t9, 1
  st t9, 40, sp
  ld t9, 40, sp
  st t9, 0, sp
  j .Lclib47
.Lclib46:
  li t9, 0
  st t9, 48, sp
  ld t9, 48, sp
  st t9, 0, sp
.Lclib47:
  ld t9, 0, sp
  beq t9, zero, .Lclib48
  li t9, 1
  st t9, 56, sp
  ld t9, 56, sp
  mov a0, t9
  addi sp, sp, 72
  ret
.Lclib48:
  li t9, 0
  st t9, 64, sp
  ld t9, 64, sp
  mov a0, t9
  addi sp, sp, 72
  ret
isspace:
  addi sp, sp, -40
  mov t0, a0
  li t1, 32
  xor t2, t0, t1
  sltiu t2, t2, 1
  beq t2, zero, .Lclib50
  li t3, 1
  mov a0, t3
  addi sp, sp, 40
  ret
.Lclib50:
  li t4, 9
  xor t5, t0, t4
  sltiu t5, t5, 1
  beq t5, zero, .Lclib52
  li t6, 1
  mov a0, t6
  addi sp, sp, 40
  ret
.Lclib52:
  li t7, 10
  xor t8, t0, t7
  sltiu t8, t8, 1
  beq t8, zero, .Lclib54
  li t9, 1
  st t9, 0, sp
  ld t9, 0, sp
  mov a0, t9
  addi sp, sp, 40
  ret
.Lclib54:
  li t9, 13
  st t9, 8, sp
  ld t10, 8, sp
  xor t9, t0, t10
  sltiu t9, t9, 1
  st t9, 16, sp
  ld t9, 16, sp
  beq t9, zero, .Lclib56
  li t9, 1
  st t9, 24, sp
  ld t9, 24, sp
  mov a0, t9
  addi sp, sp, 40
  ret
.Lclib56:
  li t9, 0
  st t9, 32, sp
  ld t9, 32, sp
  mov a0, t9
  addi sp, sp, 40
  ret
strtol:
  addi sp, sp, -736
  mov t0, a0
  mov t1, a1
  mov t2, a2
  li t4, 0
  mov t3, t4
  li t6, 1
  mov t5, t6
  li t8, 0
  mov t7, t8
  li t9, 0
  st t9, 0, sp
  ld t10, 0, sp
  xor t9, t0, t10
  sltiu t9, t9, 1
  st t9, 8, sp
  ld t9, 8, sp
  beq t9, zero, .Lclib58
  beq t1, zero, .Lclib60
  li t9, 0
  st t9, 16, sp
  ld t10, 16, sp
  st t10, 0, t1
.Lclib60:
  li t9, 0
  st t9, 24, sp
  ld t9, 24, sp
  mov a0, t9
  addi sp, sp, 736
  ret
.Lclib58:
  li t9, 10
  st t9, 32, sp
  ld t10, 32, sp
  xor t9, t2, t10
  sltu t9, zero, t9
  st t9, 40, sp
  ld t9, 40, sp
  beq t9, zero, .Lclib62
  beq t1, zero, .Lclib64
  li t9, 0
  st t9, 48, sp
  ld t10, 48, sp
  st t10, 0, t1
.Lclib64:
  li t9, 0
  st t9, 56, sp
  ld t9, 56, sp
  mov a0, t9
  addi sp, sp, 736
  ret
.Lclib62:
.Lclib66:
  li t9, 1
  st t9, 88, sp
  ld t10, 88, sp
  mul t9, t3, t10
  st t9, 96, sp
  ld t10, 96, sp
  add t9, t0, t10
  st t9, 104, sp
  ld t9, 104, sp
  ldb t10, 0, t9
  st t10, 112, sp
  li t9, 32
  st t9, 120, sp
  ld t9, 112, sp
  ld t10, 120, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 128, sp
  ld t9, 128, sp
  beq t9, zero, .Lclib74
  li t9, 1
  st t9, 136, sp
  ld t9, 136, sp
  st t9, 80, sp
  j .Lclib76
.Lclib74:
  li t9, 1
  st t9, 144, sp
  ld t10, 144, sp
  mul t9, t3, t10
  st t9, 152, sp
  ld t10, 152, sp
  add t9, t0, t10
  st t9, 160, sp
  ld t9, 160, sp
  ldb t10, 0, t9
  st t10, 168, sp
  li t9, 9
  st t9, 176, sp
  ld t9, 168, sp
  ld t10, 176, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 184, sp
  ld t9, 184, sp
  beq t9, zero, .Lclib75
  li t9, 1
  st t9, 192, sp
  ld t9, 192, sp
  st t9, 80, sp
  j .Lclib76
.Lclib75:
  li t9, 0
  st t9, 200, sp
  ld t9, 200, sp
  st t9, 80, sp
.Lclib76:
  ld t9, 80, sp
  beq t9, zero, .Lclib71
  li t9, 1
  st t9, 208, sp
  ld t9, 208, sp
  st t9, 72, sp
  j .Lclib73
.Lclib71:
  li t9, 1
  st t9, 216, sp
  ld t10, 216, sp
  mul t9, t3, t10
  st t9, 224, sp
  ld t10, 224, sp
  add t9, t0, t10
  st t9, 232, sp
  ld t9, 232, sp
  ldb t10, 0, t9
  st t10, 240, sp
  li t9, 10
  st t9, 248, sp
  ld t9, 240, sp
  ld t10, 248, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 256, sp
  ld t9, 256, sp
  beq t9, zero, .Lclib72
  li t9, 1
  st t9, 264, sp
  ld t9, 264, sp
  st t9, 72, sp
  j .Lclib73
.Lclib72:
  li t9, 0
  st t9, 272, sp
  ld t9, 272, sp
  st t9, 72, sp
.Lclib73:
  ld t9, 72, sp
  beq t9, zero, .Lclib68
  li t9, 1
  st t9, 280, sp
  ld t9, 280, sp
  st t9, 64, sp
  j .Lclib70
.Lclib68:
  li t9, 1
  st t9, 288, sp
  ld t10, 288, sp
  mul t9, t3, t10
  st t9, 296, sp
  ld t10, 296, sp
  add t9, t0, t10
  st t9, 304, sp
  ld t9, 304, sp
  ldb t10, 0, t9
  st t10, 312, sp
  li t9, 13
  st t9, 320, sp
  ld t9, 312, sp
  ld t10, 320, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 328, sp
  ld t9, 328, sp
  beq t9, zero, .Lclib69
  li t9, 1
  st t9, 336, sp
  ld t9, 336, sp
  st t9, 64, sp
  j .Lclib70
.Lclib69:
  li t9, 0
  st t9, 344, sp
  ld t9, 344, sp
  st t9, 64, sp
.Lclib70:
  ld t9, 64, sp
  beq t9, zero, .Lclib67
  li t9, 1
  st t9, 352, sp
  ld t10, 352, sp
  add t9, t3, t10
  st t9, 360, sp
  ld t9, 360, sp
  mov t3, t9
  j .Lclib66
.Lclib67:
  li t9, 1
  st t9, 368, sp
  ld t10, 368, sp
  mul t9, t3, t10
  st t9, 376, sp
  ld t10, 376, sp
  add t9, t0, t10
  st t9, 384, sp
  ld t9, 384, sp
  ldb t10, 0, t9
  st t10, 392, sp
  li t9, 45
  st t9, 400, sp
  ld t9, 392, sp
  ld t10, 400, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 408, sp
  ld t9, 408, sp
  beq t9, zero, .Lclib77
  li t9, 1
  st t9, 416, sp
  li t9, 0
  st t9, 424, sp
  ld t9, 424, sp
  ld t10, 416, sp
  sub t9, t9, t10
  st t9, 432, sp
  ld t9, 432, sp
  mov t5, t9
  li t9, 1
  st t9, 440, sp
  ld t10, 440, sp
  add t9, t3, t10
  st t9, 448, sp
  ld t9, 448, sp
  mov t3, t9
  j .Lclib78
.Lclib77:
  li t9, 1
  st t9, 456, sp
  ld t10, 456, sp
  mul t9, t3, t10
  st t9, 464, sp
  ld t10, 464, sp
  add t9, t0, t10
  st t9, 472, sp
  ld t9, 472, sp
  ldb t10, 0, t9
  st t10, 480, sp
  li t9, 43
  st t9, 488, sp
  ld t9, 480, sp
  ld t10, 488, sp
  xor t9, t9, t10
  sltiu t9, t9, 1
  st t9, 496, sp
  ld t9, 496, sp
  beq t9, zero, .Lclib79
  li t9, 1
  st t9, 504, sp
  ld t10, 504, sp
  add t9, t3, t10
  st t9, 512, sp
  ld t9, 512, sp
  mov t3, t9
.Lclib79:
.Lclib78:
.Lclib81:
  li t9, 1
  st t9, 528, sp
  ld t10, 528, sp
  mul t9, t3, t10
  st t9, 536, sp
  ld t10, 536, sp
  add t9, t0, t10
  st t9, 544, sp
  ld t9, 544, sp
  ldb t10, 0, t9
  st t10, 552, sp
  li t9, 48
  st t9, 560, sp
  ld t9, 552, sp
  ld t10, 560, sp
  slt t9, t9, t10
  xori t9, t9, 1
  st t9, 568, sp
  ld t9, 568, sp
  beq t9, zero, .Lclib83
  li t9, 1
  st t9, 576, sp
  ld t10, 576, sp
  mul t9, t3, t10
  st t9, 584, sp
  ld t10, 584, sp
  add t9, t0, t10
  st t9, 592, sp
  ld t9, 592, sp
  ldb t10, 0, t9
  st t10, 600, sp
  li t9, 57
  st t9, 608, sp
  ld t9, 600, sp
  ld t10, 608, sp
  slt t9, t10, t9
  xori t9, t9, 1
  st t9, 616, sp
  ld t9, 616, sp
  beq t9, zero, .Lclib83
  li t9, 1
  st t9, 624, sp
  ld t9, 624, sp
  st t9, 520, sp
  j .Lclib84
.Lclib83:
  li t9, 0
  st t9, 632, sp
  ld t9, 632, sp
  st t9, 520, sp
.Lclib84:
  ld t9, 520, sp
  beq t9, zero, .Lclib82
  li t9, 10
  st t9, 640, sp
  ld t10, 640, sp
  mul t9, t7, t10
  st t9, 648, sp
  li t9, 1
  st t9, 656, sp
  ld t10, 656, sp
  mul t9, t3, t10
  st t9, 664, sp
  ld t10, 664, sp
  add t9, t0, t10
  st t9, 672, sp
  ld t9, 672, sp
  ldb t10, 0, t9
  st t10, 680, sp
  li t9, 48
  st t9, 688, sp
  ld t9, 680, sp
  ld t10, 688, sp
  sub t9, t9, t10
  st t9, 696, sp
  ld t9, 648, sp
  ld t10, 696, sp
  add t9, t9, t10
  st t9, 704, sp
  ld t9, 704, sp
  mov t7, t9
  li t9, 1
  st t9, 712, sp
  ld t10, 712, sp
  add t9, t3, t10
  st t9, 720, sp
  ld t9, 720, sp
  mov t3, t9
  j .Lclib81
.Lclib82:
  beq t1, zero, .Lclib85
  st t3, 0, t1
.Lclib85:
  mul t9, t7, t5
  st t9, 728, sp
  ld t9, 728, sp
  mov a0, t9
  addi sp, sp, 736
  ret
atoi:
  addi sp, sp, -8
  st r31, 0, sp
  mov t0, a0
  li t1, 0
  li t2, 10
  mov a0, t0
  mov a1, t1
  mov a2, t2
  jal ra, strtol
  mov t3, a0
  mov a0, t3
  ld r31, 0, sp
  addi sp, sp, 8
  ret
