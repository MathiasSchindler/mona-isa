.globl start

.text
start:
    li r1, buf
    ld r2, 0, r1
    beq r2, zero, ok
    li r11, bad
    li r12, 4
    j out
ok:
    li r11, okmsg
    li r12, 3
out:
    li r10, 1
    li r17, 1
    ecall
    ebreak

.section .rodata
okmsg:
    .byte 79, 75, 10
bad:
    .byte 66, 65, 68, 10

.bss
.align 3
buf:
    .zero 8
