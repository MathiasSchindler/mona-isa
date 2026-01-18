.org 0x0000

start:
    # check 29 is prime
    addi r1, r0, 29
    jal  r31, is_prime
    beq  r16, r0, prime_fail

    # check 30 is not prime
    addi r1, r0, 30
    jal  r31, is_prime
    bne  r16, r0, prime_fail

    # check 1 is not prime
    addi r1, r0, 1
    jal  r31, is_prime
    bne  r16, r0, prime_fail

    jal  r31, print_ok
    ebreak

prime_fail:
    jal  r31, print_fail
    ebreak

# is_prime(n in r1) -> r16 (a0) = 1 if prime else 0
is_prime:
    addi r2, r0, 2
    blt  r1, r2, not_prime
    beq  r1, r2, prime

    # even check
    andi r3, r1, 1
    beq  r3, r0, not_prime

    addi r4, r0, 3      # d

prime_loop:
    mul  r5, r4, r4     # d*d
    blt  r1, r5, prime
    add  r6, r1, r0      # temp = n

mod_loop:
    blt  r6, r4, mod_done
    sub  r6, r6, r4
    j    mod_loop

mod_done:
    beq  r6, r0, not_prime
    addi r4, r4, 2
    j    prime_loop

prime:
    addi r16, r0, 1
    ret

not_prime:
    addi r16, r0, 0
    ret

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 10
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 12
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 112, 114, 105, 109, 101, 50, 58, 79, 75, 10

msg_fail:
    .byte 112, 114, 105, 109, 101, 50, 58, 70, 65, 73, 76, 10
