.data
:
  .byte 0
:
  .byte 0
:
  .byte 0
:
  .byte 0
:
  .byte 0
.text
start:
  jal ra, main
  mov r10, a0
  li r17, 3
  ecall
main:
  addi sp, sp, -288
  addi t1, sp, 280
  mov t0, t1
  addi t2, sp, 280
  li t3, 11
  st t3, 0, t2
  addi t4, sp, 280
  li t5, 8
  add t6, t4, t5
  li t7, 65
  stb t7, 0, t6
  addi t8, sp, 280
  li t9, 16
  st t9, 0, sp
  ld t10, 0, sp
  add t9, t8, t10
  st t9, 8, sp
  li t9, 22
  st t9, 16, sp
  ld t9, 8, sp
  ld t10, 16, sp
  st t10, 0, t9
  ld t10, 0, t0
  st t10, 24, sp
  li t9, 11
  st t9, 32, sp
  ld t9, 24, sp
  ld t10, 32, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 40, sp
  ld t9, 40, sp
  beq t9, zero, .L0
  li t9, 70
  st t9, 48, sp
  li t9, 
  st t9, 56, sp
  ld t9, 56, sp
  ld t10, 48, sp
  stb t10, 0, t9
  ld t9, 56, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 64, sp
  li t9, 1
  st t9, 72, sp
  ld t9, 72, sp
  mov a0, t9
  addi sp, sp, 288
  ret
.L0:
  li t9, 8
  st t9, 80, sp
  ld t10, 80, sp
  add t9, t0, t10
  st t9, 88, sp
  ld t9, 88, sp
  ldb t10, 0, t9
  st t10, 96, sp
  li t9, 65
  st t9, 104, sp
  ld t9, 96, sp
  ld t10, 104, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 112, sp
  ld t9, 112, sp
  beq t9, zero, .L2
  li t9, 70
  st t9, 120, sp
  li t9, 
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
  li t9, 1
  st t9, 144, sp
  ld t9, 144, sp
  mov a0, t9
  addi sp, sp, 288
  ret
.L2:
  li t9, 16
  st t9, 152, sp
  ld t10, 152, sp
  add t9, t0, t10
  st t9, 160, sp
  ld t9, 160, sp
  ld t10, 0, t9
  st t10, 168, sp
  li t9, 22
  st t9, 176, sp
  ld t9, 168, sp
  ld t10, 176, sp
  xor t9, t9, t10
  sltu t9, zero, t9
  st t9, 184, sp
  ld t9, 184, sp
  beq t9, zero, .L4
  li t9, 70
  st t9, 192, sp
  li t9, 
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
  li t9, 1
  st t9, 216, sp
  ld t9, 216, sp
  mov a0, t9
  addi sp, sp, 288
  ret
.L4:
  li t9, 79
  st t9, 224, sp
  li t9, 
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
  li t9, 75
  st t9, 248, sp
  li t9, 
  st t9, 256, sp
  ld t9, 256, sp
  ld t10, 248, sp
  stb t10, 0, t9
  ld t9, 256, sp
  li r10, 1
  mov r11, t9
  li r12, 1
  li r17, 1
  ecall
  mov t9, a0
  st t9, 264, sp
  li t9, 0
  st t9, 272, sp
  ld t9, 272, sp
  mov a0, t9
  addi sp, sp, 288
  ret
