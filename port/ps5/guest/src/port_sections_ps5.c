/*
 * Locates the game's globals (sections khdata / khbss, see port/game_prefix.h)
 * from the linker script of the game image (guest.ld).
 */
#include <string.h>

#include "port/port.h"

#ifdef __MACH__
extern char __start_khdata[] __asm("section$start$__DATA$khdata");
extern char __stop_khdata[] __asm("section$end$__DATA$khdata");
extern char __start_khbss[] __asm("section$start$__DATA$khbss");
extern char __stop_khbss[] __asm("section$end$__DATA$khbss");
#else
extern char __start_khdata[], __stop_khdata[], __start_khbss[], __stop_khbss[];
#endif

void PortGameSections(char** data, size_t* dataSize, char** bss, size_t* bssSize) {
    *data = __start_khdata;
    *dataSize = (size_t)(__stop_khdata - __start_khdata);
    *bss = __start_khbss;
    *bssSize = (size_t)(__stop_khbss - __start_khbss);
}
