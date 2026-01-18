.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    addi r1, r0, 1
    slli r2, r1, 64

    # should not reach
    jal  r31, print_fail
    ebreak

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 2   # illegal instruction
    bne  r1, r2, trap_fail
    jal  r31, print_ok
    ebreak

trap_fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 17
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 19
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 105, 108, 108, 101, 103, 97, 108, 45, 115, 104, 105, 102, 116, 58, 79, 75, 10

msg_fail:
    .byte 105, 108, 108, 101, 103, 97, 108, 45, 115, 104, 105, 102, 116, 58, 70, 65, 73, 76, 10
