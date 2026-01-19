.include "include-macro.inc"
.globl start

.text
start:
    li r10, 1
    emit_msg msg 3
    li r17, 1
    ecall
    ebreak

.section .rodata
msg:
    .byte 72, 73, 10
