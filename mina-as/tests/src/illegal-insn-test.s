.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    .word 0xffffffff

    j fail

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 2
    bne  r1, r2, fail
    csrrw r3, mtval, r0
    li   r5, expected_insn
    ld   r4, 0, r5
    bne  r3, r4, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 16
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 18
    li   r17, 1
    ecall
    ret

.align 3
expected_insn:
    .dword 0x00000000ffffffff

msg_ok:
    .byte 105, 108, 108, 101, 103, 97, 108, 45, 105, 110, 115, 110, 58, 79, 75, 10

msg_fail:
    .byte 105, 108, 108, 101, 103, 97, 108, 45, 105, 110, 115, 110, 58, 70, 65, 73, 76, 10
