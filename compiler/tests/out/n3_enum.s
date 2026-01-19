.data
c:
  .dword 2
.Lputc0:
  .byte 0
.Lputc1:
  .byte 0
.Lputc2:
  .byte 0
.Lputc3:
  .byte 0
.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  addi sp, sp, -160
  li t1, 1
  li t2, 2
  add t3, t1, t2
  li t4, 10
  add t5, t3, t4
  li t6, 11
  add t7, t5, t6
  mov t0, t7
  li t8, c
  ld t10, 0, t8
  st t10, 0, sp
  li t9, 2
  st t9, 8, sp
  ld t9, 0, sp
  ld t10, 8, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 16, sp
  ld t9, 16, sp
  beq t9, zero, .L0
  li t9, 70
  st t9, 24, sp
  li t9, .Lputc0
  st t9, 32, sp
  ld t9, 32, sp
  ld t10, 24, sp
  stb t10, 0, t9
  ld t9, 32, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 40, sp
  li t9, 1
  st t9, 48, sp
  ld t9, 48, sp
  mov a0, t9
  addi sp, sp, 160
  ret
.L0:
  li t9, 23
  st t9, 56, sp
  ld t10, 56, sp
  xor t9, t0, t10
  sltiu t9, t9, 1
  st t9, 64, sp
  ld t9, 64, sp
  beq t9, zero, .L4
  j .L3
.L4:
  j .L5
.L3:
  li t9, 79
  st t9, 72, sp
  li t9, .Lputc1
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
  li t9, .Lputc2
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
  li t9, 0
  st t9, 120, sp
  ld t9, 120, sp
  mov a0, t9
  addi sp, sp, 160
  ret
.L5:
  li t9, 70
  st t9, 128, sp
  li t9, .Lputc3
  st t9, 136, sp
  ld t9, 136, sp
  ld t10, 128, sp
  stb t10, 0, t9
  ld t9, 136, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 144, sp
  li t9, 1
  st t9, 152, sp
  ld t9, 152, sp
  mov a0, t9
  addi sp, sp, 160
  ret
.L2:
