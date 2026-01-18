.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    tzero tr0
    tcvt tr0, tr0, int8
    tzero tr1
    tadd tr2, tr0, tr1

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
    li   r12, 22
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 24
    li   r17, 1
    ecall
    ret

.align 3
expected_insn:
    .dword 0x000000000010015b

msg_ok:
    .byte 116, 101, 110, 115, 111, 114, 45, 105, 108, 108, 101, 103, 97, 108, 45, 102, 109, 116, 58, 79, 75, 10

msg_fail:
    .byte 116, 101, 110, 115, 111, 114, 45, 105, 108, 108, 101, 103, 97, 108, 45, 102, 109, 116, 58, 70, 65, 73, 76, 10
