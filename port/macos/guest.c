/* Small platform additions to the shared console shim. Normal SRAM remains upstream. */
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/port_state.h"
#include "port/port_ui.h"
#include "bridge_calls.h"
int gPortMmpx;
int gPortInMainLoop, gPortStateSlot=1;
void PortPs5LockSettings(void) {
#ifdef VERSION_JP
    gPortConfig.hdText=0; /* bundled Latin fonts do not cover Japanese */
#endif
}
void PortGameSections(char** data,size_t* size,char** bss,size_t* bsize) {
    static char empty; *data=*bss=&empty; *size=*bsize=0;
}
const char* PortStatePath(int slot) { (void)slot;return ""; }
void PortStateRequestSave(const char* p) { (void)p;PortUiNotice("Use normal cartridge saves on macOS"); }
void PortStateRequestLoad(const char* p) { (void)p;PortUiNotice("Use normal cartridge saves on macOS"); }
void PortStateService(void) {}
void PortStateRequestQuiet(int load,const char* path) { (void)load;(void)path; }
int PortStateBusy(void) { return 0; }
int PortStateLastResult(void) { return 0; }
void PortWatchdogStart(void) {}
void PortCrashHandlerInstall(void) {}
void PortBiosFaultInstall(void) { PortLog("macOS: checked linear memory; no signal-based fault recovery"); }
FILE* popen(const char* cmd,const char* mode) { (void)cmd;(void)mode;return NULL; }
int pclose(FILE* f) { (void)f;return -1; }
void* memcpy(void* dst,const void* src,size_t n) { unsigned char* d=dst;const unsigned char* s=src;while(n--)*d++=*s++;return dst; }
void* memset(void* dst,int c,size_t n) { unsigned char* d=dst;while(n--)*d++=(unsigned char)c;return dst; }
void* memmove(void* dst,const void* src,size_t n) {
    unsigned char* d=dst;const unsigned char* s=src;
    if(d<=s) return memcpy(dst,src,n);
    while(n) { n--;d[n]=s[n]; }return dst;
}
int main(int,char**);
int guest_main(void) {
    static char rom[4096]; extern const char* gPortRomPath;
    if(h_getenv("KHCOM_ROM",rom,sizeof(rom))>0) gPortRomPath=rom;
    const char* path=PortConfigPath("kh_com.ini");
    if(!h_stat(path)) {
        FILE* f=fopen(path,"w");
        if(f) {
            fputs("config_version=10\nview_mode=0\naspect=6\nfilter=0\nscale_mode=0\nhd_text=0\n"
                  "smooth_audio=0\ndodge_button=0\nstock_button=0\ndiscord_presence=0\n"
                  "hd_movies=0\nhd_music=0\nhd_subtitles=0\nhd_scenes=0\nhd_cards=0\nhd_title=0\n",f);
            fclose(f);
        }
    }
    static char name[]="ReKHCoM"; static char* argv[]={name,NULL}; return main(1,argv);
}
extern int h_audio_gain(float);
bool SDL_SetAudioStreamGain(SDL_AudioStream* stream,float gain) { (void)stream;return h_audio_gain(gain)!=0; }
/* HD replacement assets are outside this port's original-cartridge scope. */
void PortHdMusicHold(void) {}
void PortHdMusicGain(float gain) { (void)gain; }
void PortHdMusicFrame(void) {}
void PortHdMusicShutdown(void) {}
const char* PortHdMusicTrack(int song) { (void)song;return NULL; }
/* Invoked only by the isolated native integration test, never by the app. */
int macos_sram_test(void) {
    volatile unsigned char* sram=(volatile unsigned char*)(uintptr_t)GBA_SRAM;
    if(!PortMemoryInit()) return 1;
    PortSramLoad(); sram[0]=0x42; sram[GBA_SRAM_SIZE-1]=0x99;
    PortSramSyncIfDirty(); sram[0]=sram[GBA_SRAM_SIZE-1]=0;
    PortSramLoad(); return sram[0]!=0x42 || sram[GBA_SRAM_SIZE-1]!=0x99;
}
int macos_input_test(void) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) PortInputHandleEvent(&event);
    return PortInputKeys();
}
