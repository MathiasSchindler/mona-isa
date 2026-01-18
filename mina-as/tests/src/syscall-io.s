.org 0x0000

start:
    # write("OK\n") via syscall
    li   r10, 1        # fd = stdout
    li   r11, msg      # buf
    li   r12, 3        # len
    li   r17, 1        # SYS_write
    ecall

    ebreak

msg:
    .byte 79, 75, 10, 0
