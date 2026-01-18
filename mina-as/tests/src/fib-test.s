.org 0x0000

start:
    addi r1, r0, 0     # a
    addi r2, r0, 1     # b
    addi r3, r0, 10    # count

fib_loop:
    beq  r3, r0, fib_done
    add  r4, r1, r2    # t
    add  r1, r2, r0    # a = b
    add  r2, r4, r0    # b = t
    addi r3, r3, -1
    j    fib_loop

fib_done:
    addi r5, r0, 55
    bne  r1, r5, fib_fail
    jal  r31, print_ok
    ebreak

fib_fail:
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

msg_ok:
    .byte 102, 105, 98, 58, 53, 53, 10

msg_fail:
    .byte 102, 105, 98, 58, 70, 65, 73, 76, 10
