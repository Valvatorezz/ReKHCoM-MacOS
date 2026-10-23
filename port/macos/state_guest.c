/* Experimental same-build snapshots. Requests cross the bridge; restoration
 * occurs on the host only after abandoning the old native call stack. */
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/port_state.h"
#include "port/port_ui.h"
#include "bridge_calls.h"

extern void MacStateKeepSram(void);
extern void MacStateTimingReset(void);
extern void MacStateInputReset(void);
extern void PortUiForceClose(void);

const char* PortStatePath(int slot) {
    static char path[1024];
    h_mkdir(PortConfigPath("states"));
    SDL_snprintf(path, sizeof(path), "%sstates/slot%d.macstate", PortConfigPath(""), slot % 10);
    return path;
}
void PortStateRequestSave(const char* path) {
    h_state_request(1, path ? path : PortStatePath(gPortStateSlot));
}
void PortStateRequestLoad(const char* path) {
    h_state_request(2, path ? path : PortStatePath(gPortStateSlot));
}
void PortStateRequestQuiet(int load, const char* path) {
    /* Boss rematches remain disabled until separately validated. */
    (void)load; (void)path;
}
int PortStateBusy(void) { return 0; }
int PortStateLastResult(void) { return 0; }
void PortStateService(void) {}

/* Only called at the beginning of the extracted AgbMain loop. Never at a
 * nested VBlank wait inside a scene, movie or options-menu stack. */
void MacStateCheckpoint(void) {
    MacStateKeepSram();
    int result=h_state_checkpoint();
    if(result>0)PortUiNotice("State saved (experimental)");
    else if(result<0)PortUiNotice("State failed: check kh_com_host.log");
}

void macos_state_loaded(void) {
    PortAudioReset();
    MacStateInputReset();
    SDL_Event clear={0};clear.type=SDL_EVENT_WINDOW_FOCUS_LOST;
    PortInputHandleEvent(&clear);
    MacStateTimingReset();
    PortUiForceClose();
    PortAchievementsInvalidate("Save state loaded");
    PortUiNotice("State loaded (experimental)");
}
