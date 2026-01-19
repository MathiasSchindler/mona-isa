.file "directives-test.c"
.loc 1 1 0
.globl start

.text
start:
    li   r10, 1
    li   r11, msg
    li   r12, 3
    li   r17, 1
    ecall
    ebreak

.section .rodata
.align 12
msg:
    .byte 79, 75, 10
