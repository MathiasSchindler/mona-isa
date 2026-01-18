.org 0x0000

start:
    # UART TX 0x10000000, RX 0x10000004, STATUS 0x10000008
    movhi r10, 0x10000
    addi r11, r10, 4
    addi r12, r10, 8

wait:
    ldbu r1, 0, r12
    beq  r1, r0, wait

    ldbu r1, 0, r11
    stb  r1, 0, r10
    ebreak
