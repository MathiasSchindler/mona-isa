.org 0x0000

start:
    fence
    jal  r31, print_ok
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 9
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 102, 101, 110, 99, 101, 58, 79, 75, 10
