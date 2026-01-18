.org 0x0000

start:
    # cycle monotonic
    csrrs r1, 0xC00, r0
    addi  r2, r0, 5
    addi  r2, r2, 1
    csrrs r3, 0xC00, r0
    blt   r1, r3, cycle_ok
    j     fail

cycle_ok:
    # time monotonic
    csrrs r4, 0xC01, r0
    csrrs r5, 0xC01, r0
    blt   r4, r5, time_ok
    j     fail

time_ok:
    # instret monotonic
    csrrs r6, 0xC02, r0
    csrrs r7, 0xC02, r0
    blt   r6, r7, ro_test
    j     fail

ro_test:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    addi r3, r0, 1
    csrrw r0, 0xC00, r3

    # should not reach
    j fail

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 2
    bne  r1, r2, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 15
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 17
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 99, 115, 114, 45, 99, 111, 117, 110, 116, 101, 114, 58, 79, 75, 10

msg_fail:
    .byte 99, 115, 114, 45, 99, 111, 117, 110, 116, 101, 114, 58, 70, 65, 73, 76, 10
