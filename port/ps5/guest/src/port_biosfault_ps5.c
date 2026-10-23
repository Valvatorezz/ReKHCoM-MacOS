/*
 * Stray pointers, as the GBA bus handles them: the handler is the host's (app/ps5_host.c), a port of the desktop
 * build's port_biosfault.c to signals; it is installed before the game starts.
 */
#include "port/port.h"

void PortBiosFaultInstall(void) {
    PortLog("PS5: stray pointers and divisions by zero are handled by the host");
}
