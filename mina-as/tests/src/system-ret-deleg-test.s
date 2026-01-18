.org 0x0000

start:
    # UART TX at 0x10000000
    movhi r10, 0x10000

    # stvec = s_handler
    addi r1, r0, s_handler
    csrrw r2, stvec, r1

    # medeleg: delegate illegal instruction (cause 2)
    addi r1, r0, 4
    csrrw r2, medeleg, r1

    # mepc = s_main
    addi r1, r0, s_main
    csrrw r2, mepc, r1

    # set MPP = S (bit 11)
    addi r1, r0, 0x800
    csrrs r0, mstatus, r1

    mret

s_main:
    # sepc = u_main
    addi r1, r0, u_main
    csrrw r2, sepc, r1

    # clear SPP (bit 13) to return to U
    movhi r1, 0x2
    csrrc r0, sstatus, r1

    sret

u_main:
    csrrs r1, mstatus, r0
    j fail

s_handler:
    csrrw r1, scause, r0
    addi r2, r0, 2
    bne  r1, r2, fail
    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    addi r1, r0, 115  # s
    stb  r1, 0, r10
    addi r1, r0, 121  # y
    stb  r1, 0, r10
    addi r1, r0, 115  # s
    stb  r1, 0, r10
    addi r1, r0, 114  # r
    stb  r1, 0, r10
    addi r1, r0, 101  # e
    stb  r1, 0, r10
    addi r1, r0, 116  # t
    stb  r1, 0, r10
    addi r1, r0, 58   # :
    stb  r1, 0, r10
    addi r1, r0, 79   # O
    stb  r1, 0, r10
    addi r1, r0, 75   # K
    stb  r1, 0, r10
    addi r1, r0, 10   # \n
    stb  r1, 0, r10
    ret

print_fail:
    addi r1, r0, 115  # s
    stb  r1, 0, r10
    addi r1, r0, 121  # y
    stb  r1, 0, r10
    addi r1, r0, 115  # s
    stb  r1, 0, r10
    addi r1, r0, 114  # r
    stb  r1, 0, r10
    addi r1, r0, 101  # e
    stb  r1, 0, r10
    addi r1, r0, 116  # t
    stb  r1, 0, r10
    addi r1, r0, 58   # :
    stb  r1, 0, r10
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
