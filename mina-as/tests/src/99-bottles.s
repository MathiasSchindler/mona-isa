.org 0x0000

start:
    # UART TX at 0x10000000
    movhi r10, 0x10000

    addi r22, r0, 99

verse_loop:
    add  r1, r22, r0
    jal  r31, print_num
    jal  r31, print_bottles_wall
    jal  r31, print_newline

    add  r1, r22, r0
    jal  r31, print_num
    jal  r31, print_bottles
    jal  r31, print_newline

    jal  r31, print_take_one

    addi r22, r22, -1
    beq  r22, r0, next_no_more
    add  r1, r22, r0
    jal  r31, print_num
    jal  r31, print_bottles_wall
    j    verse_end

next_no_more:
    jal  r31, print_no_more_wall

verse_end:
    jal  r31, print_newline
    jal  r31, print_newline
    bne  r22, r0, verse_loop

final_verse:
    jal  r31, print_no_more_wall_full
    jal  r31, print_newline
    jal  r31, print_go_store
    jal  r31, print_99_wall
    jal  r31, print_newline
    ebreak

print_str:
    ldbu r21, 0, r20
    beq  r21, r0, print_str_ret
    stb  r21, 0, r10
    addi r20, r20, 1
    j    print_str

print_str_ret:
    ret

print_newline:
    li   r20, str_newline
    j    print_str

print_num:
    add  r23, r1, r0
    addi r24, r0, 0
    addi r12, r0, 10
    blt  r23, r12, num_ones

num_tens_loop:
    addi r23, r23, -10
    addi r24, r24, 1
    bge  r23, r12, num_tens_loop
    addi r24, r24, 48
    stb  r24, 0, r10

num_ones:
    addi r23, r23, 48
    stb  r23, 0, r10
    ret

print_bottles_wall:
    addi r2, r0, 1
    beq  r1, r2, pbw_sing
    li   r20, str_bottles_wall
    j    print_str

pbw_sing:
    li   r20, str_bottle_wall
    j    print_str

print_bottles:
    addi r2, r0, 1
    beq  r1, r2, pb_sing
    li   r20, str_bottles
    j    print_str

pb_sing:
    li   r20, str_bottle
    j    print_str

print_take_one:
    li   r20, str_take_one
    j    print_str

print_no_more_wall:
    li   r20, str_no_more_wall
    j    print_str

print_no_more_wall_full:
    li   r20, str_no_more_wall_full
    j    print_str

print_go_store:
    li   r20, str_go_store
    j    print_str

print_99_wall:
    li   r20, str_99_wall
    j    print_str

str_newline:
    .byte 10, 0

str_bottles_wall:
    .byte 32, 98, 111, 116, 116, 108, 101, 115, 32, 111, 102, 32, 98, 101, 101, 114
    .byte 32, 111, 110, 32, 116, 104, 101, 32, 119, 97, 108, 108, 46, 0

str_bottle_wall:
    .byte 32, 98, 111, 116, 116, 108, 101, 32, 111, 102, 32, 98, 101, 101, 114, 32
    .byte 111, 110, 32, 116, 104, 101, 32, 119, 97, 108, 108, 46, 0

str_bottles:
    .byte 32, 98, 111, 116, 116, 108, 101, 115, 32, 111, 102, 32, 98, 101, 101, 114
    .byte 46, 0

str_bottle:
    .byte 32, 98, 111, 116, 116, 108, 101, 32, 111, 102, 32, 98, 101, 101, 114, 46
    .byte 0

str_take_one:
    .byte 84, 97, 107, 101, 32, 111, 110, 101, 32, 100, 111, 119, 110, 32, 97, 110
    .byte 100, 32, 112, 97, 115, 115, 32, 105, 116, 32, 97, 114, 111, 117, 110, 100
    .byte 44, 32, 0

str_no_more_wall:
    .byte 110, 111, 32, 109, 111, 114, 101, 32, 98, 111, 116, 116, 108, 101, 115, 32
    .byte 111, 102, 32, 98, 101, 101, 114, 32, 111, 110, 32, 116, 104, 101, 32, 119
    .byte 97, 108, 108, 46, 0

str_no_more_wall_full:
    .byte 78, 111, 32, 109, 111, 114, 101, 32, 98, 111, 116, 116, 108, 101, 115, 32
    .byte 111, 102, 32, 98, 101, 101, 114, 32, 111, 110, 32, 116, 104, 101, 32, 119
    .byte 97, 108, 108, 44, 32, 110, 111, 32, 109, 111, 114, 101, 32, 98, 111, 116
    .byte 116, 108, 101, 115, 32, 111, 102, 32, 98, 101, 101, 114, 46, 0

str_go_store:
    .byte 71, 111, 32, 116, 111, 32, 116, 104, 101, 32, 115, 116, 111, 114, 101, 32
    .byte 97, 110, 100, 32, 98, 117, 121, 32, 115, 111, 109, 101, 32, 109, 111, 114
    .byte 101, 44, 32, 0

str_99_wall:
    .byte 57, 57, 32, 98, 111, 116, 116, 108, 101, 115, 32, 111, 102, 32, 98, 101
    .byte 101, 114, 32, 111, 110, 32, 116, 104, 101, 32, 119, 97, 108, 108, 46, 0
