.org 0x0000

start:
    # ensure stack alignment (16-byte)
    andi r4, r30, 0xF
    bne  r4, r0, fail

    # args 1..8 in a0-a7
    addi r16, r0, 1
    addi r17, r0, 2
    addi r18, r0, 3
    addi r19, r0, 4
    addi r20, r0, 5
    addi r21, r0, 6
    addi r22, r0, 7
    addi r23, r0, 8

    # stack args (9th, 10th): arg8 at 0(sp), arg9 at 8(sp)
    addi r30, r30, -16
    addi r4, r0, 9
    st   r4, 0, r30
    addi r4, r0, 10
    st   r4, 8, r30

    jal  r31, sum10

    addi r30, r30, 16

    # expect sum 1..10 = 55
    addi r2, r0, 55
    bne  r16, r2, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

sum10:
    ld   r4, 0, r30
    ld   r5, 8, r30
    add  r16, r16, r17
    add  r16, r16, r18
    add  r16, r16, r19
    add  r16, r16, r20
    add  r16, r16, r21
    add  r16, r16, r22
    add  r16, r16, r23
    add  r16, r16, r4
    add  r16, r16, r5
    ret

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 13
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 15
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 97, 98, 105, 45, 115, 116, 97, 99, 107, 58, 79, 75, 10

msg_fail:
    .byte 97, 98, 105, 45, 115, 116, 97, 99, 107, 58, 70, 65, 73, 76, 10
