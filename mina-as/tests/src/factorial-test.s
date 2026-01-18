.org 0x0000

start:
    addi r1, r0, 5     # n
    addi r2, r0, 1     # acc

fact_loop:
    beq  r1, r0, fact_done
    mul  r2, r2, r1
    addi r1, r1, -1
    j    fact_loop

fact_done:
    addi r3, r0, 120
    bne  r2, r3, fact_fail
    jal  r31, print_ok
    ebreak

fact_fail:
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
    li   r12, 15
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 102, 97, 99, 116, 111, 114, 105, 97, 108, 58, 49, 50, 48, 10

msg_fail:
    .byte 102, 97, 99, 116, 111, 114, 105, 97, 108, 58, 70, 65, 73, 76, 10
