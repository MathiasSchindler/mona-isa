.org 0x0000

start:
    # UART TX at 0x10000000
    movhi r10, 0x10000

    cgettag r1, c0
    addi r2, r0, 1
    bne  r1, r2, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    addi r1, r0, 79   # O
    stb  r1, 0, r10
    addi r1, r0, 75   # K
    stb  r1, 0, r10
    addi r1, r0, 10   # \n
    stb  r1, 0, r10
    ret

print_fail:
    addi r1, r0, 70   # F
    stb  r1, 0, r10
    addi r1, r0, 65   # A
    stb  r1, 0, r10
    addi r1, r0, 73   # I
    stb  r1, 0, r10
    addi r1, r0, 76   # L
    stb  r1, 0, r10
    addi r1, r0, 10   # \n
    stb  r1, 0, r10
    ret
