.org 0x0000

start:
    addi r1, r0, m_handler
    csrrw r2, mtvec, r1
    addi r1, r0, s_handler
    csrrw r2, stvec, r1

    # enable MIE
    addi r1, r0, 1
    csrrs r0, mstatus, r1

    ecall

after_mret:
    csrrs r1, mstatus, r0
    addi r2, r0, 1
    and  r3, r1, r2
    beq  r3, r0, fail
    addi r2, r0, 0x10
    and  r3, r1, r2
    beq  r3, r0, fail

    # delegate ECALL from S (cause 9)
    addi r1, r0, 0x200
    csrrw r2, medeleg, r1

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
    # enable SIE
    addi r1, r0, 0x2
    csrrs r0, sstatus, r1

    ecall

after_sret:
    csrrs r1, sstatus, r0
    addi r2, r0, 0x2
    and  r3, r1, r2
    beq  r3, r0, fail
    addi r2, r0, 0x20
    and  r3, r1, r2
    beq  r3, r0, fail
    movhi r2, 0x2
    and   r3, r1, r2
    bne   r3, r0, fail

    jal  r31, print_ok
    ebreak

m_handler:
    csrrs r1, mstatus, r0
    addi r2, r0, 1
    and  r3, r1, r2
    bne  r3, r0, fail
    addi r2, r0, 0x10
    and  r3, r1, r2
    beq  r3, r0, fail

    csrrw r1, mepc, r0
    addi r1, r1, 4
    csrrw r0, mepc, r1
    mret

s_handler:
    csrrs r1, sstatus, r0
    addi r2, r0, 0x2
    and  r3, r1, r2
    bne  r3, r0, fail
    addi r2, r0, 0x20
    and  r3, r1, r2
    beq  r3, r0, fail
    movhi r2, 0x2
    and   r3, r1, r2
    beq   r3, r0, fail

    csrrw r1, sepc, r0
    addi r1, r1, 4
    csrrw r0, sepc, r1
    sret

fail:
    jal  r31, print_fail
    ebreak

print_ok:
    li   r10, 1
    li   r11, msg_ok
    li   r12, 12
    li   r17, 1
    ecall
    ret

print_fail:
    li   r10, 1
    li   r11, msg_fail
    li   r12, 14
    li   r17, 1
    ecall
    ret

msg_ok:
    .byte 114, 101, 116, 45, 98, 105, 116, 115, 58, 79, 75, 10

msg_fail:
    .byte 114, 101, 116, 45, 98, 105, 116, 115, 58, 70, 65, 73, 76, 10
