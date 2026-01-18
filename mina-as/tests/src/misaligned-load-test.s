.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    li   r4, data_word
    addi r4, r4, 2
    mov  r6, r4
    ldw  r5, 0, r4

    j fail

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 4
    bne  r1, r2, fail
    csrrw r3, mtval, r0
    bne  r3, r6, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 19
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 21
    li   r17, 1
    ecall
    ret

.align 2
data_word:
    .word 0

msg_ok:
    .byte 109, 105, 115, 97, 108, 105, 103, 110, 101, 100, 45, 108, 111, 97, 100, 58, 79, 75, 10

msg_fail:
    .byte 109, 105, 115, 97, 108, 105, 103, 110, 101, 100, 45, 108, 111, 97, 100, 58, 70, 65, 73, 76, 10
