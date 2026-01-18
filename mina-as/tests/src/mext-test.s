.org 0x0000

start:
    # mulh (signed high)
    addi r1, r0, -3
    addi r2, r0, 5
    mulh r3, r1, r2
    addi r4, r0, -1
    bne  r3, r4, fail

    # mulhsu (signed * unsigned high)
    addi r1, r0, -2
    addi r2, r0, 3
    mulhsu r3, r1, r2
    addi r4, r0, -1
    bne  r3, r4, fail

    # mulhu (unsigned high)
    addi r1, r0, -1
    addi r2, r0, 2
    mulhu r3, r1, r2
    addi r4, r0, 1
    bne  r3, r4, fail

    # div (signed)
    addi r1, r0, -7
    addi r2, r0, 2
    div  r3, r1, r2
    addi r4, r0, -3
    bne  r3, r4, fail

    # divu (unsigned)
    addi r1, r0, 7
    addi r2, r0, 3
    divu r3, r1, r2
    addi r4, r0, 2
    bne  r3, r4, fail

    # rem (signed)
    addi r1, r0, -7
    addi r2, r0, 2
    rem  r3, r1, r2
    addi r4, r0, -1
    bne  r3, r4, fail

    # remu (unsigned)
    addi r1, r0, 7
    addi r2, r0, 3
    remu r3, r1, r2
    addi r4, r0, 1
    bne  r3, r4, fail

    # div by zero -> -1
    addi r1, r0, 5
    addi r2, r0, 0
    div  r3, r1, r2
    addi r4, r0, -1
    bne  r3, r4, fail

    # rem by zero -> rs1
    addi r1, r0, 5
    addi r2, r0, 0
    rem  r3, r1, r2
    addi r4, r0, 5
    bne  r3, r4, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 8
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 10
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 109, 101, 120, 116, 58, 79, 75, 10

msg_fail:
    .byte 109, 101, 120, 116, 58, 70, 65, 73, 76, 10
