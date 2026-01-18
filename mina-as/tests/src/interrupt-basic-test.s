.org 0x0000

start:
    addi r1, r0, m_handler
    csrrw r2, mtvec, r1
    addi r1, r0, s_handler
    csrrw r2, stvec, r1

    # enable MIE
    addi r1, r0, 1
    csrrs r0, mstatus, r1

    # enable MSIP in mie
    addi r1, r0, 0x8
    csrrw r2, mie, r1

    # set MSIP pending in mip
    addi r1, r0, 0x8
    csrrw r2, mip, r1

    addi r0, r0, 0
    j fail

m_handler:
    csrrw r1, mcause, r0
    slt  r4, r1, r0
    beq  r4, r0, fail
    addi r2, r0, 0x7
    and  r4, r1, r2
    addi r3, r0, 0x3
    bne  r4, r3, fail

    csrrs r1, mstatus, r0
    addi r2, r0, 1
    and  r3, r1, r2
    bne  r3, r0, fail
    addi r2, r0, 0x10
    and  r3, r1, r2
    beq  r3, r0, fail

    # delegate MSIP to S
    addi r1, r0, 0x8
    csrrw r2, mideleg, r1
    csrrw r2, mie, r1
    csrrw r2, mip, r1

    # enable SIE, disable MIE
    addi r1, r0, 0x2
    csrrs r0, mstatus, r1
    addi r1, r0, 1
    csrrc r0, mstatus, r1

    # mepc = s_main, MPP = S
    addi r1, r0, s_main
    csrrw r2, mepc, r1
    movhi r1, 0x2
    addi  r1, r1, -0x800
    csrrc r0, mstatus, r1
    movhi r1, 0x1
    addi  r1, r1, -0x800
    csrrs r0, mstatus, r1

    mret

s_main:
    j fail

s_handler:
    csrrw r1, scause, r0
    slt  r4, r1, r0
    beq  r4, r0, fail
    addi r2, r0, 0x7
    and  r4, r1, r2
    addi r3, r0, 0x3
    bne  r4, r3, fail

    csrrs r1, sstatus, r0
    addi r2, r0, 0x2
    and  r3, r1, r2
    bne  r3, r0, fail
    addi r2, r0, 0x20
    and  r3, r1, r2
    beq  r3, r0, fail

    jal  r31, print_ok
    ebreak

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 13
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 15
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 105, 110, 116, 101, 114, 114, 117, 112, 116, 58, 79, 75, 10

msg_fail:
    .byte 105, 110, 116, 101, 114, 114, 117, 112, 116, 58, 70, 65, 73, 76, 10
