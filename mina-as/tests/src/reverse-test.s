.org 0x0000

start:
    li   r20, src
    li   r21, dst
    addi r12, r0, 0    # len

len_loop:
    ldbu r1, 0, r20
    beq  r1, r0, len_done
    addi r20, r20, 1
    addi r12, r12, 1
    j    len_loop

len_done:
    li   r20, src
    addi r16, r0, 0    # i

rev_loop:
    bge  r16, r12, rev_done
    addi r15, r12, -1
    sub  r15, r15, r16
    add  r13, r20, r15
    ldbu r1, 0, r13
    add  r14, r21, r16
    stb  r1, 0, r14
    addi r16, r16, 1
    j    rev_loop

rev_done:
    add  r14, r21, r12
    stb  r0, 0, r14

    li   r22, exp
    addi r16, r0, 0

cmp_loop:
    bge  r16, r12, cmp_ok
    add  r13, r21, r16
    ldbu r1, 0, r13
    add  r14, r22, r16
    ldbu r2, 0, r14
    bne  r1, r2, cmp_fail
    addi r16, r16, 1
    j    cmp_loop

cmp_ok:
    jal  r31, print_ok
    ebreak

cmp_fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 11
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 13
    li   r17, 1
    ecall
    ret

src:
    .byte 97, 98, 99, 100, 101, 0

exp:
    .byte 101, 100, 99, 98, 97, 0

dst:
    .byte 0, 0, 0, 0, 0, 0

msg_ok:
    .byte 114, 101, 118, 101, 114, 115, 101, 58, 79, 75, 10

msg_fail:
    .byte 114, 101, 118, 101, 114, 115, 101, 58, 70, 65, 73, 76, 10
