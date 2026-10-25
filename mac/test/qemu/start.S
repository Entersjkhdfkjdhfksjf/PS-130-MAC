    .section .text.entry, "ax"
    .global _start
_start:
    ldr sp, =0x06000000
    bl  qmain
1:  b   1b

    .section .rodata
    .global rom_start, rom_end
    .balign 4
rom_start:
    .incbin "test/MacII.ROM"
rom_end:

    .section .text.handlers, "ax"
    .global und_h, pabt_h, dabt_h
und_h:
    ldr sp, =0x06100000
    sub r0, lr, #4
    mov r2, #1
    bl  report_exc
    b   .
pabt_h:
    ldr sp, =0x06100000
    sub r0, lr, #4
    mov r2, #2
    bl  report_exc
    b   .
dabt_h:
    ldr sp, =0x06100000
    sub r0, lr, #8
    mov r2, #3
    bl  report_exc
    b   .
