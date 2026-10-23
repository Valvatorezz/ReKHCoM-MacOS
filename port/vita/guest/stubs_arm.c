/* One stub per host function: jumps to entry N of the host's table (bridge_list.h order, the host's HF_*). Game and
 * host are the same ARMv7 process with the same calling convention, so the arguments pass through untouched. */
#include "bridge.h"

#define STR2(x) #x
#define STR(x) STR2(x)
#define X(name) \
    __asm__(".text\n.arm\n.p2align 2\n.globl h_" #name "\n.type h_" #name ", %function\nh_" #name ":\n" \
            "\tldr ip, =gHostTable+" STR(__COUNTER__) "*8\n\tldr pc, [ip]\n.ltorg\n");
#include "bridge_list.h"
#undef X

/* filled by the host (port/switch/host/switch_host.c, __vita__) before it calls guest_main: the low word of each
   entry is the function */
uint64_t gHostTable[HF_COUNT];
