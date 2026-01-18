.org 0x0000

start:
    addi r1, r0, 0
    csrrw r0, mstatus, r1

    li   r2, -1
    csrrw r0, sstatus, r2

    csrrs r3, sstatus, r0
    li   r4, 0x2722
    bne  r3, r4, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 20
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 22
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 99, 115, 114, 45, 115, 115, 116, 97, 116, 117, 115, 45, 109, 97, 115, 107, 58, 79, 75, 10

msg_fail:
    .byte 99, 115, 114, 45, 115, 115, 116, 97, 116, 117, 115, 45, 109, 97, 115, 107, 58, 70, 65, 73, 76, 10
