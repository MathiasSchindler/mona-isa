.org 0x0000

start:
    # amoswap.w test
    li   r1, data_w
    ldw  r4, 0, r1
    ldw  r2, val_w, r0
    amoswap.w r3, r1, r2
    bne  r3, r4, fail
    ldw  r5, 0, r1
    bne  r5, r2, fail

    # amoswap.d test
    li   r1, data_d
    ld   r4, 0, r1
    ld   r2, val_d, r0
    amoswap.d r3, r1, r2
    bne  r3, r4, fail
    ld   r5, 0, r1
    bne  r5, r2, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

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

.align 4
data_w:
    .word 0xdeadbeef

val_w:
    .word 0x11223344

.align 8
data_d:
    .dword 0x0102030405060708

val_d:
    .dword 0x1122334455667788

msg_ok:
    .byte 97, 109, 111, 58, 79, 75, 10

msg_fail:
    .byte 97, 109, 111, 58, 70, 65, 73, 76, 10
