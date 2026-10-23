/*
 * The desktop build logs a crash from an unhandled-exception filter and, with KHCOM_WATCHDOG set, samples the game
 * thread to locate hangs. Here the host (app/ps5_host.c) writes the crash report, with the registers and the game's
 * call chain, in a file of its own under logs/, so there is nothing to install in the game.
 */
#include "port/port.h"

void PortCrashHandlerInstall(void) {}
void PortWatchdogStart(void) {}
