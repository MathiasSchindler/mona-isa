.org 0x0000

    # Demonstrates a minimal tensor path (FP8 load + tmma + store)
    # r8 = 0x1000 (A), r9 = 0x1100 (B), r10 = 0x2000 (C)
    movhi r8, 0x1
    addi  r8, r8, 0x000
    movhi r9, 0x1
    addi  r9, r9, 0x100
    movhi r10, 0x2
    addi  r10, r10, 0x000

    tzero tr0
    tcvt  tr0, tr0, fp8e4m3
    tld   tr0, r8, 16

    tzero tr1
    tcvt  tr1, tr1, fp8e4m3
    tld   tr1, r9, 16

    tzero tr2
    tmma  tr2, tr0, tr1
    tst   tr2, r10, 64

    ebreak
