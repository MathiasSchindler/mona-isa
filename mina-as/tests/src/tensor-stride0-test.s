.org 0x0000

start:
    li   r8, tile_stride0
    tld  tr0, r8, 0

    tred tr0, r3, sum
    li   r5, expected_512
    ld   r4, 0, r5
    bne  r3, r4, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 18
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 20
    li   r17, 1
    ecall
    ret

.align 3
expected_512:
    .dword 0x4080000000000000

.align 4
tile_stride0:
    .word 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000
    .word 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0
    .word 0, 0, 0, 0, 0, 0, 0, 0

msg_ok:
    .byte 116, 101, 110, 115, 111, 114, 45, 115, 116, 114, 105, 100, 101, 48, 58, 79, 75, 10

msg_fail:
    .byte 116, 101, 110, 115, 111, 114, 45, 115, 116, 114, 105, 100, 101, 48, 58, 70, 65, 73, 76, 10
