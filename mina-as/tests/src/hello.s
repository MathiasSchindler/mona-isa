.org 0x0000

start:
    # write("Hello, world!\n") via syscall
    li   r10, 1        # fd = stdout
    li   r11, msg      # buf
    li   r12, 14       # len
    li   r17, 1        # SYS_write
    ecall

    ebreak

msg:
    .byte 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33, 10
