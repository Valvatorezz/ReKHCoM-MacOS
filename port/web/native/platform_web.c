/* Browser backend: original pixel fonts, normal SRAM, no raw-pointer states. */
#include <SDL3/SDL.h>
#include <emscripten.h>
#include <stdint.h>
#include "port/port.h"
#include "port/port_audio.h"
#include "port/port_state.h"
#include "port/port_ui.h"
#include "web_sjis.h"
static char noSnapshot;
static int hidden;
void PortGameSections(char** data,size_t* size,char** bss,size_t* bsize) {
    *data=*bss=&noSnapshot;*size=*bsize=0; /* Soft reset reloads the page. */
}
int gPortInMainLoop,gPortStateSlot=1;
const char* PortStatePath(int slot) { (void)slot;return ""; }
void PortStateRequestSave(const char* p) { (void)p;PortUiNotice("Use normal saves in the browser"); }
void PortStateRequestLoad(const char* p) { (void)p;PortUiNotice("Use normal saves in the browser"); }
void PortStateService(void) {}
/* no states here: boss rematches stay hidden (port_ui.c) */
void PortStateRequestQuiet(int load, const char* path) { (void)load; (void)path; }
int PortStateBusy(void) { return 0; }
int PortStateLastResult(void) { return 0; }
void PortWatchdogStart(void) {}
void PortCrashHandlerInstall(void) {}
void PortBiosFaultInstall(void) {}
int PortCodepointToSjis(unsigned cp) {
    int lo=0,hi=(int)(sizeof(webSjis)/sizeof(webSjis[0]));
    while(lo<hi) { int m=(lo+hi)/2;if(webSjis[m][0]<cp)lo=m+1;else hi=m; }
    return lo<(int)(sizeof(webSjis)/sizeof(webSjis[0])) && webSjis[lo][0]==cp ? webSjis[lo][1] : 0;
}
static int pendingW, pendingH;
extern void PortWebApplyResize(int w,int h);
EMSCRIPTEN_KEEPALIVE void PortWebResize(int w,int h) {
    if(w>=240 && h>=160 && w<=2560 && h<=1440){pendingW=w;pendingH=h;}
}
EMSCRIPTEN_KEEPALIVE void PortWebFlushSave(void) {PortSramSyncIfDirty();PortConfigSave();}
EMSCRIPTEN_KEEPALIVE void PortWebClearInput(void) {
    SDL_Event event={0};event.type=SDL_EVENT_WINDOW_FOCUS_LOST;
    PortInputHandleEvent(&event);
}
EMSCRIPTEN_KEEPALIVE void PortWebSetVisible(int on) {
    hidden=!on;
    PortWebClearInput();
    PortAudioReset();
    if(hidden)PortWebFlushSave();
}
void PortWebWaitVisible(void) {
    while(hidden)emscripten_sleep(100);
    if(pendingW){int w=pendingW,h=pendingH;pendingW=0;PortWebApplyResize(w,h);}
}
