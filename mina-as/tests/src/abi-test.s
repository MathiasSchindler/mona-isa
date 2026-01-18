.org 0x0000

start:
    # caller sets callee-saved s0 (r1)
    addi r1, r0, 0x55

    # ensure stack alignment (16-byte)
    andi r4, r30, 0xF
    bne  r4, r0, fail

    # make a call
    jal  r31, callee

    # verify s0 preserved and return value
    addi r2, r0, 0x55
    bne  r1, r2, fail
    addi r2, r0, 0x33
    bne  r16, r2, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

callee:
    addi r30, r30, -16
    st   r31, 0, r30
    st   r1, 8, r30

    addi r1, r0, 0x99      # clobber s0
    addi r16, r0, 0x33     # return value in a0

    ld   r1, 8, r30
    ld   r31, 0, r30
    addi r30, r30, 16
    ret

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 7
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 9
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 97, 98, 105, 58, 79, 75, 10

msg_fail:
    .byte 97, 98, 105, 58, 70, 65, 73, 76, 10
