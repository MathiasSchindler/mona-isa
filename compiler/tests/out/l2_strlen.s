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
  addi sp, sp, -264
  st r31, 0, sp
  addi t0, sp, 256
  li t1, 0
  li t2, 1
  mul t3, t1, t2
  add t4, t0, t3
  li t5, 97
  stb t5, 0, t4
  addi t6, sp, 256
  li t7, 1
  li t8, 1
  mul t9, t7, t8
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t6, t10
  st t9, 16, sp
  li t9, 98
  st t9, 24, sp
  ld t9, 16, sp
  ld t10, 24, sp
  stb t10, 0, t9
  addi t9, sp, 256
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
  li t9, 99
  st t9, 72, sp
  ld t9, 64, sp
  ld t10, 72, sp
  stb t10, 0, t9
  addi t9, sp, 256
  st t9, 80, sp
  li t9, 3
  st t9, 88, sp
  li t9, 1
  st t9, 96, sp
  ld t9, 88, sp
  ld t10, 96, sp
  mul t9, t9, t10
  st t9, 104, sp
  ld t9, 80, sp
  ld t10, 104, sp
  add t9, t9, t10
  st t9, 112, sp
  li t9, 0
  st t9, 120, sp
  ld t9, 112, sp
  ld t10, 120, sp
  stb t10, 0, t9
  addi t9, sp, 256
  st t9, 128, sp
  ld t9, 128, sp
  mov a0, t9
  jal ra, strlen
  mov t9, a0
  st t9, 136, sp
  li t9, 3
  st t9, 144, sp
  ld t9, 136, sp
  ld t10, 144, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 152, sp
  ld t9, 152, sp
  beq t9, zero, .L0
  li t9, 1
  st t9, 160, sp
  ld t9, 160, sp
  mov r10, t9
  li r17, 3
  ecall
  li t9, 0
  st t9, 168, sp
.L0:
  li t9, 79
  st t9, 176, sp
  li t9, .Lputc0
  st t9, 184, sp
  ld t9, 184, sp
  ld t10, 176, sp
  stb t10, 0, t9
  ld t9, 184, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 192, sp
  li t9, 75
  st t9, 200, sp
  li t9, .Lputc1
  st t9, 208, sp
  ld t9, 208, sp
  ld t10, 200, sp
  stb t10, 0, t9
  ld t9, 208, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 216, sp
  li t9, 10
  st t9, 224, sp
  li t9, .Lputc2
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
  li t9, 0
  st t9, 248, sp
  ld t9, 248, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 264
  ret
