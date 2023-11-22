
    .global _start
_start:
    la a0, _begin_str
    la a1, _end_str
    sub a1, a1, a0
    li a7, 0xff00
    ecall
halt:
    j halt
    
_begin_str:
    .ascii "Hello, World!\r\n"
_end_str:
