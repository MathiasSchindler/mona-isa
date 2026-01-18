.org 0x0000

start:
    addi r1, r0, 29    # n
    addi r2, r0, 2     # d
    addi r5, r0, 1     # prime flag

prime_loop:
    addi r6, r0, 6
    beq  r2, r6, prime_done
    add  r4, r1, r0    # temp = n

mod_loop:
    blt  r4, r2, mod_done
    sub  r4, r4, r2
    j    mod_loop

mod_done:
    beq  r4, r0, not_prime
    addi r2, r2, 1
    j    prime_loop

not_prime:
    addi r5, r0, 0

prime_done:
    beq  r5, r0, prime_fail

    # check 30 is not prime (divisible by 2)
    addi r1, r0, 30
    addi r2, r0, 2
    add  r4, r1, r0

mod2_loop:
    blt  r4, r2, mod2_done
    sub  r4, r4, r2
    j    mod2_loop

mod2_done:
    bne  r4, r0, prime_fail

    jal  r31, print_ok
    ebreak

prime_fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 9
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 11
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 112, 114, 105, 109, 101, 58, 79, 75, 10

msg_fail:
    .byte 112, 114, 105, 109, 101, 58, 70, 65, 73, 76, 10
