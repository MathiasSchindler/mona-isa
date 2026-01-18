.org 0x0000

start:
    li   r20, src
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
    addi r17, r12, -1  # j

pal_loop:
    bge  r16, r17, pal_ok
    add  r13, r20, r16
    ldbu r1, 0, r13
    add  r14, r20, r17
    ldbu r2, 0, r14
    bne  r1, r2, pal_fail
    addi r16, r16, 1
    addi r17, r17, -1
    j    pal_loop

pal_ok:
    jal  r31, print_ok
    ebreak

pal_fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 14
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 16
    li   r17, 1
    ecall
    ret

src:
    .byte 114, 97, 99, 101, 99, 97, 114, 0

msg_ok:
    .byte 112, 97, 108, 105, 110, 100, 114, 111, 109, 101, 58, 79, 75, 10

msg_fail:
    .byte 112, 97, 108, 105, 110, 100, 114, 111, 109, 101, 58, 70, 65, 73, 76, 10
