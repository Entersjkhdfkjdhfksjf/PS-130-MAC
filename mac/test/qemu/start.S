    .section .text.entry, "ax"
    .global _start
_start:
    ldr sp, =0x06000000
    bl  qmain
1:  b   1b
