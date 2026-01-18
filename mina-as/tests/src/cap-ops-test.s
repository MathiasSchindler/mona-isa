.org 0x0000

start:
    addi r1, r0, 0x100
    csetbounds c1, c0, r1
    cgetlen r2, c1
    bne  r2, r1, fail

    addi r3, r0, 0x42
    cseal   c2, c1, r3
    cunseal c3, c2, r3
    cgetlen r4, c3
    bne  r4, r1, fail

    addi r5, r0, 0x1
    csetperm c4, c3, r5

    li   r6, cap_mem
    cst  c3, 0, r6
    cld  c5, 0, r6
    cgetlen r7, c5
    bne  r7, r1, fail

    cgettag r8, c5
    addi r9, r0, 1
    bne  r8, r9, fail

    jal  r31, print_ok
    ebreak

fail:
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

.align 4
cap_mem:
    .dword 0
    .dword 0

msg_ok:
    .byte 99, 97, 112, 45, 111, 112, 115, 58, 79, 75, 10

msg_fail:
    .byte 99, 97, 112, 45, 111, 112, 115, 58, 70, 65, 73, 76, 10
