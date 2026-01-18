.org 0x0000

start:
    addi r1, r0, handler
    csrrw r2, mtvec, r1

    li   r3, cap_mem
    cld  c0, 0, r3

    li   r4, data_byte
    ldbu r5, 0, r4

    j fail

handler:
    csrrw r1, mcause, r0
    addi r2, r0, 11
    bne  r1, r2, fail
    csrrw r3, mtval, r0
    addi r4, r0, 3
    bne  r3, r4, fail
    jal  r31, print_ok
    ebreak

fail:
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

.align 4
cap_mem:
    .dword 0
    .dword 0

data_byte:
    .byte 0

msg_ok:
    .byte 99, 97, 112, 45, 102, 97, 117, 108, 116, 45, 116, 97, 103, 58, 79, 75, 10

msg_fail:
    .byte 99, 97, 112, 45, 102, 97, 117, 108, 116, 45, 116, 97, 103, 58, 70, 65, 73, 76, 10
