#ifndef G_SETJMP_H
#define G_SETJMP_H
#ifdef __aarch64__
typedef unsigned long long jmp_buf[22]; /* x19-x30, sp, d8-d15 */
#elif defined(__arm__)
typedef unsigned long long jmp_buf[13]; /* r4-r11, sp, lr, d8-d15 (the Vita image, port/vita/guest/crt0_arm.S) */
#else
typedef unsigned long long jmp_buf[8];
#endif
int setjmp(jmp_buf);
void longjmp(jmp_buf, int) __attribute__((noreturn));
#endif
