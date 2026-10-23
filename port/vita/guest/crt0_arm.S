/* setjmp, longjmp and the heap of the Vita game image (ARMv7, port/vita/build_vita.py). The Switch image's
 * crt0_a64.S does the same in AArch64; here the game and the host share the process, the stack and the calling
 * convention, so there is no boot or trampoline: the host calls guest_main and the stubs jump straight to the host
 * (stubs_arm.c). memcpy, memset and memmove are the host's (newlib's, optimized for the Cortex-A9). */
    .syntax unified
    .arm
    .text

/* jmp_buf: r4-r11, sp, lr, d8-d15 (13 doublewords, include/setjmp.h) */
    .globl setjmp
    .type setjmp, %function
    .p2align 2
setjmp:
    mov ip, sp
    stmia r0!, {r4-r11, ip, lr}
    vstmia r0, {d8-d15}
    mov r0, #0
    bx lr

    .globl longjmp
    .type longjmp, %function
    .p2align 2
longjmp:
    ldmia r0!, {r4-r11, ip, lr}
    vldmia r0, {d8-d15}
    mov sp, ip
    movs r0, r1
    moveq r0, #1
    bx lr

/* The guest's heap (libc.c's allocator): zero-filled memory of the image. */
    .bss
    .p2align 4
    .globl __heap_start
    .globl __heap_end
__heap_start:
    .space 0x04000000
__heap_end:

    .section .note.GNU-stack,"",%progbits
