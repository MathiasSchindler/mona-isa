.section .text
entry_pad:
    j    fail

start:
    li   r1, data_val
    li   r2, 0x3000
    bne  r1, r2, fail

    li   r1, bss_val
    li   r2, 0x4000
    bne  r1, r2, fail

    li   r3, 0x12345678
    stw  r3, 0, r1
    ldw  r4, 0, r1
    bne  r4, r3, fail

    jal  r31, print_ok
    ebreak

fail:
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

.section .data
.align 3
data_val:
    .dword 0xdeadbeef

msg_ok:
    .byte 101, 108, 102, 45, 108, 97, 121, 111, 117, 116, 58, 79, 75, 10

msg_fail:
    .byte 101, 108, 102, 45, 108, 97, 121, 111, 117, 116, 58, 70, 65, 73, 76, 10

.section .bss
.align 3
bss_val:
    .dword 0
