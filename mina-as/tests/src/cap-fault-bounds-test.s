.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    addi r3, r0, 8
    csetbounds c0, c0, r3

    li   r4, data_word
    addi r4, r4, 16
    ldbu r5, 0, r4

    # should not reach
    j fail

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 11
    bne  r1, r2, fail
    csrrw r3, mtval, r0
    addi r4, r0, 1
    bne  r3, r4, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 20
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 22
    li   r17, 1
    ecall
    ret

.align 2
data_word:
    .word 0

msg_ok:
    .byte 99, 97, 112, 45, 102, 97, 117, 108, 116, 45, 98, 111, 117, 110, 100, 115, 58, 79, 75, 10

msg_fail:
    .byte 99, 97, 112, 45, 102, 97, 117, 108, 116, 45, 98, 111, 117, 110, 100, 115, 58, 70, 65, 73, 76, 10
