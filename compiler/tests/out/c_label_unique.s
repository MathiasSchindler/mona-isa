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
f:
  addi sp, sp, -40
  mov t0, a0
  li t2, 0
  mov t1, t2
  li t4, 0
  mov t3, t4
.L0:
  slt t5, t1, t0
  beq t5, zero, .L1
  li t6, 2
  xor t7, t1, t6
  sltiu t7, t7, 1
  beq t7, zero, .L2
  li t8, 5
  add t9, t3, t8
  st t9, 0, sp
  ld t9, 0, sp
  mov t3, t9
  j .L3
.L2:
  li t9, 1
  st t9, 8, sp
  ld t10, 8, sp
  add t9, t3, t10
  st t9, 16, sp
  ld t9, 16, sp
  mov t3, t9
.L3:
  li t9, 1
  st t9, 24, sp
  ld t10, 24, sp
  add t9, t1, t10
  st t9, 32, sp
  ld t9, 32, sp
  mov t1, t9
  j .L0
.L1:
  mov a0, t3
  addi sp, sp, 40
  ret
g:
  addi sp, sp, -48
  mov t0, a0
  li t2, 0
  mov t1, t2
  li t4, 0
  mov t3, t4
  li t5, 0
  mov t1, t5
.L4:
  slt t6, t1, t0
  beq t6, zero, .L6
  li t7, 3
  slt t8, t1, t7
  beq t8, zero, .L7
  li t9, 2
  st t9, 0, sp
  ld t10, 0, sp
  add t9, t3, t10
  st t9, 8, sp
  ld t9, 8, sp
  mov t3, t9
  j .L8
.L7:
  li t9, 3
  st t9, 16, sp
  ld t10, 16, sp
  add t9, t3, t10
  st t9, 24, sp
  ld t9, 24, sp
  mov t3, t9
.L8:
.L5:
  li t9, 1
  st t9, 32, sp
  ld t10, 32, sp
  add t9, t1, t10
  st t9, 40, sp
  ld t9, 40, sp
  mov t1, t9
  j .L4
.L6:
  mov a0, t3
  addi sp, sp, 48
  ret
main:
  addi sp, sp, -96
  st r31, 0, sp
  li t1, 5
  mov a0, t1
  jal ra, f
  mov t2, a0
  mov t0, t2
  li t4, 5
  mov a0, t4
  jal ra, g
  mov t5, a0
  mov t3, t5
  add t6, t0, t3
  li t7, 20
  xor t8, t6, t7
  sltu t8, zero, t8
  beq t8, zero, .L9
  li t9, 70
  st t9, 8, sp
  li t9, .Lputc0
  st t9, 16, sp
  ld t9, 16, sp
  ld t10, 8, sp
  stb t10, 0, t9
  ld t9, 16, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 24, sp
  li t9, 1
  st t9, 32, sp
  ld t9, 32, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 96
  ret
.L9:
  li t9, 79
  st t9, 40, sp
  li t9, .Lputc1
  st t9, 48, sp
  ld t9, 48, sp
  ld t10, 40, sp
  stb t10, 0, t9
  ld t9, 48, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 56, sp
  li t9, 75
  st t9, 64, sp
  li t9, .Lputc2
  st t9, 72, sp
  ld t9, 72, sp
  ld t10, 64, sp
  stb t10, 0, t9
  ld t9, 72, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 80, sp
  li t9, 0
  st t9, 88, sp
  ld t9, 88, sp
  mov a0, t9
  ld r31, 0, sp
  addi sp, sp, 96
  ret
