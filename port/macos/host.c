/* macOS SDL3 host, adapted from the upstream Android64 host's bridge backends.
 * The game is compiled ahead of time via wasm32 -> C -> arm64. No browser/JIT.
 * Only offsets into the linear memory cross the generated bridge adapters. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <CommonCrypto/CommonDigest.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "crash_diagnostics.h"
#include "bridge.h"
#include "core.h"
static FILE* sLog;
static int sQuit, sBackground;
static jmp_buf sExit;
static int sExitCode;
static char sData[4096];
static void Log(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); fputc('\n', stderr);
    if (sLog) { va_start(ap, fmt); vfprintf(sLog, fmt, ap); va_end(ap); fputc('\n', sLog); fflush(sLog); }
}
static void Fatal(const char* message) { Log("%s", message); SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ReKHCoM", message, NULL); exit(1); }
static void H_log(const char* s) { Log("%s", s); }
static void H_exit(int code) {
    Log("the game ended (%d)", code);
    if (sLog)
        fflush(sLog);
    sExitCode = code;
    longjmp(sExit, 1);
}
static uint64_t H_ticks_us(void) { return SDL_GetTicksNS() / 1000; }
static void H_sleep_us(uint32_t us) { SDL_DelayNS((Uint64)us * 1000); }
static int H_getenv(const char* name, char* out, int cap) {
    const char* v = SDL_getenv(name);
    if (!v || cap <= 0)
        return -1;
    SDL_strlcpy(out, v, (size_t)cap);
    return (int)strlen(out);
}

/* Files remain outside the read-only app bundle. */
static SDL_IOStream* sFiles[64];
static SDL_IOStream* OpenPath(const char* path, const char* mode) {
    return SDL_IOFromFile(path, mode);
}
static int H_fopen(const char* path, int mode) {
    static const char* const modes[] = { "rb", "wb", "ab", "r+b" };
    for (int i = 0; i < 64; ++i)
        if (!sFiles[i]) {
            sFiles[i] = OpenPath(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int H_fread(int h, void* buf, uint32_t n) { return (int)SDL_ReadIO(sFiles[h], buf, n); }
static int H_fwrite(int h, const void* buf, uint32_t n) { return (int)SDL_WriteIO(sFiles[h], buf, n); }
static int H_fseek(int h, int32_t off, int whence) {
    return SDL_SeekIO(sFiles[h], off, whence == 0 ? SDL_IO_SEEK_SET : whence == 1 ? SDL_IO_SEEK_CUR : SDL_IO_SEEK_END) < 0 ? -1 : 0;
}
static int H_ftell(int h) { return (int)SDL_TellIO(sFiles[h]); }
static int H_fclose(int h) {
    bool ok = SDL_CloseIO(sFiles[h]);
    sFiles[h] = NULL;
    return ok ? 0 : -1;
}
static int H_fflush(int h) { return SDL_FlushIO(sFiles[h]) ? 0 : -1; }
static int H_stat(const char* p) {
    struct stat s;
    return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1;
}
static int H_remove(const char* p) { return remove(p); }
static int H_rename(const char* a, const char* b) { return rename(a, b); }
static int H_mkdir(const char* p) { return mkdir(p, 0777); }
static DIR* sDirs[16];
static int H_dir_open(const char* p) {
    for (int i = 0; i < 16; ++i)
        if (!sDirs[i]) {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int H_dir_next(int d, char* name, int cap) {
    struct dirent* e;
    while ((e = readdir(sDirs[d]))) {
        if (e->d_name[0] == '.')
            continue;
        SDL_strlcpy(name, e->d_name, (size_t)cap);
        return 1;
    }
    return 0;
}
static void H_dir_close(int d) {
    closedir(sDirs[d]);
    sDirs[d] = NULL;
}

/* ---- video: the GPU ------------------------------------------------------------------------------------------- */

static SDL_Window* sWindow;
static SDL_Renderer* sRenderer;
static SDL_Texture* sTex[4096];
static int sTexW[4096], sTexH[4096], sTexSmooth[4096];
static int sBlend = 1;

static void H_gfx_size(int* w, int* h) { SDL_GetCurrentRenderOutputSize(sRenderer, w, h); }
static void H_gfx_begin(uint32_t clear) {
    SDL_SetRenderClipRect(sRenderer, NULL);
    SDL_SetRenderDrawColor(sRenderer, (clear >> 16) & 255, (clear >> 8) & 255, clear & 255, 255);
    if (clear >> 24)
        SDL_RenderClear(sRenderer);
}
static int H_tex_create(const void* rgba, int w, int h, int smooth) {
    for (int i = 0; i < 4096; i++)
        if (!sTex[i]) {
            sTex[i] = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!sTex[i])
                return -1;
            SDL_UpdateTexture(sTex[i], NULL, rgba, w * 4);
            SDL_SetTextureBlendMode(sTex[i], SDL_BLENDMODE_BLEND);
            sTexW[i] = w;
            sTexH[i] = h;
            sTexSmooth[i] = smooth;
            return i;
        }
    return -1;
}
static void H_tex_update(int id, const void* rgba) { SDL_UpdateTexture(sTex[id], NULL, rgba, sTexW[id] * 4); }
static void H_tex_free(int id) {
    SDL_DestroyTexture(sTex[id]);
    sTex[id] = NULL;
}
static void H_gfx_clip(int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_SetRenderClipRect(sRenderer, w > 0 ? &r : NULL);
}
static void H_gfx_blend(int on) { sBlend = on; }

/* HostVertex triangles; the shim adds 2, 4 or 6 to u for pixel art and the MMPX filters (port/macos/guest/src/sdl_shim.c):
   those are drawn crisp (nearest), as the software hosts read them */
static void H_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    SDL_Texture* t = id >= 0 && id < 4096 ? sTex[id] : NULL;
    static SDL_Vertex out[8192];
    int crisp = 0;
    float shift = 0;
    if (id >= 0 && !t)
        return;
    if (count < 0) return;
    if (count > 8192)
        count = 8192;
    if (count > 0 && t) {
        shift = floorf((v[0].u + 0.25f) / 2.0f) * 2.0f;
        crisp = shift > 0;
        SDL_SetTextureScaleMode(t, crisp || !sTexSmooth[id] ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        SDL_SetTextureBlendMode(t, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    }
    for (int i = 0; i < count; i++) {
        out[i].position.x = v[i].x;
        out[i].position.y = v[i].y;
        out[i].tex_coord.x = v[i].u - shift;
        out[i].tex_coord.y = v[i].v;
        out[i].color.r = ((v[i].argb >> 16) & 255) / 255.0f;
        out[i].color.g = ((v[i].argb >> 8) & 255) / 255.0f;
        out[i].color.b = (v[i].argb & 255) / 255.0f;
        out[i].color.a = (v[i].argb >> 24) / 255.0f;
    }
    SDL_SetRenderDrawBlendMode(sRenderer, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_RenderGeometry(sRenderer, t, out, count, NULL, 0);
}

static SDL_AudioStream* sAudio;
static int sGameRate = 48000; /* the rate the game opened the audio at */
static int H_audio_open(int rate) {
    sGameRate = rate;
    if (sAudio)
        return 0;
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, rate };
    sAudio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!sAudio) {
        Log("audio: %s", SDL_GetError());
        return -1;
    }
    SDL_ResumeAudioStreamDevice(sAudio);
    Log("audio: %d Hz", rate);
    return 0;
}
static void H_audio_push(const int16_t* frames, uint32_t count) {
    if (sAudio && count)
        SDL_PutAudioStreamData(sAudio, frames, (int)count * 4);
}

static uint32_t H_audio_queued(void) { return sAudio ? (uint32_t)SDL_GetAudioStreamQueued(sAudio) / 4 : 0; }
static int H_audio_gain(float gain) { return sAudio && SDL_SetAudioStreamGain(sAudio,gain); }
static void H_audio_clear(void) {
    if (sAudio)
        SDL_ClearAudioStream(sAudio);
}
static void H_audio_ratio(double ratio) {
    if (sAudio && ratio > 0.5 && ratio < 2.0)
        SDL_SetAudioStreamFrequencyRatio(sAudio, (float)ratio);
}

/* The real keyboard events feed the guest's existing remappable input handler. */
static SDL_Gamepad* sPad;
static uint32_t sKeyQueue[128][5];
static unsigned sKeyHead, sKeyTail;
static void Pump(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) sQuit = 1;
        if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            sBackground = 1;
            sKeyHead = 0; sKeyTail = 1;
            memset(sKeyQueue[0],0,sizeof(sKeyQueue[0]));
            sKeyQueue[0][2] = 2; /* clear held guest keys when leaving the window */
        }
        if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED) sBackground = 0;
        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !sPad) sPad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && sPad && SDL_GetGamepadID(sPad) == e.gdevice.which) {
            SDL_CloseGamepad(sPad); sPad = NULL;
        }
        if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat &&
                (e.key.key == SDLK_F11 || (e.key.key == SDLK_RETURN && (e.key.mod & SDL_KMOD_ALT))))
                SDL_SetWindowFullscreen(sWindow, !(SDL_GetWindowFlags(sWindow) & SDL_WINDOW_FULLSCREEN));
            unsigned next = (sKeyTail + 1) % 128;
            if (next != sKeyHead) {
                uint32_t* k = sKeyQueue[sKeyTail];
                k[0] = e.key.key; k[1] = e.key.scancode; k[2] = e.key.down; k[3] = e.key.mod; k[4] = e.key.repeat;
                sKeyTail = next;
            }
        }
    }
    if (!sPad) {
        int n = 0; SDL_JoystickID* ids = SDL_GetGamepads(&n);
        if (n) sPad = SDL_OpenGamepad(ids[0]);
        SDL_free(ids);
    }
}
static int H_fullscreen(int enabled) { return SDL_SetWindowFullscreen(sWindow,enabled!=0); }
static int H_vsync(int enabled) { return SDL_SetRenderVSync(sRenderer,enabled); }
static int H_window_size(int w,int h) { return SDL_SetWindowSize(sWindow,w,h); }
static int H_keyboard(uint8_t* keys, int cap) {
    int n; const bool* state = SDL_GetKeyboardState(&n);
    if (cap > n) cap = n;
    for (int j = 0; j < cap; j++) keys[j] = !sBackground && state[j];
    return cap;
}
static int H_key_event(uint32_t* out) {
    if (sKeyHead == sKeyTail) return 0;
    memcpy(out, sKeyQueue[sKeyHead], sizeof(sKeyQueue[0]));
    sKeyHead = (sKeyHead + 1) % 128; return 1;
}
static int H_scancode(uint32_t key) { return SDL_GetScancodeFromKey(key, NULL); }
static int H_pad_poll(HostPad* out, int count) {
    Pump();
    if (count <= 0 || count > HOSTPAD_MAX) return 0;
    memset(out, 0, sizeof(*out) * (size_t)count);
    if (!sPad || sBackground) return 0;
    static const struct { SDL_GamepadButton button; int bit; } map[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH,0}, {SDL_GAMEPAD_BUTTON_WEST,1}, {SDL_GAMEPAD_BUTTON_BACK,2},
        {SDL_GAMEPAD_BUTTON_START,3}, {SDL_GAMEPAD_BUTTON_DPAD_UP,4}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN,5},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT,6}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT,7}, {SDL_GAMEPAD_BUTTON_EAST,8},
        {SDL_GAMEPAD_BUTTON_NORTH,9}, {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,10}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,11},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK,14}, {SDL_GAMEPAD_BUTTON_RIGHT_STICK,15}, {SDL_GAMEPAD_BUTTON_TOUCHPAD,16}
    };
    out[0].connected = 1;
    for (size_t j=0;j<SDL_arraysize(map);j++)
        if (SDL_GetGamepadButton(sPad,map[j].button)) out[0].buttons |= 1u << map[j].bit;
    out[0].lx=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_LEFTX); out[0].ly=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_LEFTY);
    out[0].rx=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_RIGHTX); out[0].ry=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_RIGHTY);
    out[0].l2=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)>>7;
    out[0].r2=SDL_GetGamepadAxis(sPad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)>>7;
    if(out[0].l2>64) out[0].buttons|=1u<<12;
    if(out[0].r2>64) out[0].buttons|=1u<<13;
    return 1;
}
static int H_touch_poll(HostTouch* p, int n) { (void)p; (void)n; return 0; }
static int H_app_state(void) { return sBackground; }
static int H_quit_requested(void) { Pump(); return sQuit; }
static void H_set_title(const char* s) { SDL_SetWindowTitle(sWindow,s); }
static void H_notify(const char* s) { Log("notice: %s",s); }
static int H_hd_music(int op,const char* p,int gain) { (void)op;(void)p;(void)gain;return 0; }
static int H_hd_movie(const char* a,const char* b,const char* c,const char* d) { (void)a;(void)b;(void)c;(void)d;return 0; }
static unsigned sFrames;
static void H_gfx_end(void) {
    sFrames++;
    const char* shot=SDL_getenv("KHCOM_HOST_SHOT");
    const char* frame=SDL_getenv("KHCOM_HOST_SHOT_FRAME");
    if(shot && sFrames==(unsigned)(frame?atoi(frame):120)) {
        SDL_Surface* surface=SDL_RenderReadPixels(sRenderer,NULL);
        if(surface) { SDL_SaveBMP(surface,shot); SDL_DestroySurface(surface); }
    }
    SDL_RenderPresent(sRenderer);
}
/* All scalar types are converted explicitly; pointers become checked offsets. */
static m4_coreInstance sCore;
void trap(Trap code) { Log("game trap at display frame %u: %s",sFrames,trapDescription(code)); sExitCode=2; longjmp(sExit,1); }
static void* GuestSpan(wasmModuleInstance* instance,U32 offset,U64 size) {
    wasmMemory* m=m4_coreExport6_memory((m4_coreInstance*)instance);
    if(offset>m->size || size>m->size-offset) { Log("invalid guest span: offset=0x%08x size=%llu memory=%llu",offset,(unsigned long long)size,(unsigned long long)m->size); trap(trapMemoryOutOfBounds); }
    return m->data+offset;
}
static const char* GuestString(wasmModuleInstance* instance,U32 offset) {
    wasmMemory* m=m4_coreExport6_memory((m4_coreInstance*)instance);
    const char* p=GuestSpan(instance,offset,1);
    if(!memchr(p,0,(size_t)(m->size-offset))) trap(trapMemoryOutOfBounds);
    return p;
}
F32 i3_env6_roundf(wasmModuleInstance* instance,F32 x) { (void)instance;return roundf(x); }
#include "adapters.inc"

static const char* sSelected;
static int sDialogDone;
static void Selected(void* unused,const char* const* files,int filter) {
    (void)unused;(void)filter;
    if(files && files[0]) sSelected=SDL_strdup(files[0]);
    sDialogDone=1;
}
static void CleanupGuest(void) {
    for(int j=0;j<4096;j++) if(sTex[j]) H_tex_free(j);
    for(int j=0;j<64;j++) if(sFiles[j]) { SDL_CloseIO(sFiles[j]); sFiles[j]=NULL; }
    for(int j=0;j<16;j++) if(sDirs[j]) { closedir(sDirs[j]); sDirs[j]=NULL; }
    if(sAudio) { SDL_DestroyAudioStream(sAudio); sAudio=NULL; }
}
int main(int argc,char** argv) {
    const char* rom=SDL_getenv("KHCOM_ROM"); const char* data=SDL_getenv("KHCOM_DATA");
    for(int j=1;j<argc;j++) {
        if(!strcmp(argv[j],"--rom") && j+1<argc) rom=argv[++j];
        else if(!strcmp(argv[j],"--data-dir") && j+1<argc) data=argv[++j];
        else { fprintf(stderr,"Usage: %s [--rom path.gba] [--data-dir directory]\n",argv[0]); return 1; }
    }
    char* pref=data?NULL:SDL_GetPrefPath("ReKHCoM","ReKHCoM");
    if(!data && !pref) { fprintf(stderr,"Cannot determine save folder: %s\n",SDL_GetError()); return 1; }
    SDL_strlcpy(sData,data?data:pref,sizeof(sData)); SDL_free(pref);
    if(!SDL_CreateDirectory(sData)) { fprintf(stderr,"Cannot create save folder: %s\n",SDL_GetError()); return 1; }
    char logPath[4096]; SDL_snprintf(logPath,sizeof(logPath),"%s/kh_com_host.log",sData); sLog=fopen(logPath,"w");
    MacCrashInstall(sData);
    Log("ReKHCoM macOS arm64 (%s), SDL %d, data: %s",MACOS_REGION,SDL_GetVersion(),sData);
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD)) Fatal(SDL_GetError());
    sWindow=SDL_CreateWindow("ReKHCoM",960,640,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);
    sRenderer=sWindow?SDL_CreateRenderer(sWindow,NULL):NULL;
    if(!sRenderer) Fatal(SDL_GetError());
    SDL_SetRenderVSync(sRenderer,1);
    if(!rom) {
        static const SDL_DialogFileFilter filters[]={{"Game Boy Advance cartridge dump","gba"}};
        SDL_ShowOpenFileDialog(Selected,NULL,sWindow,filters,1,NULL,false);
        while(!sDialogDone && !sQuit) { Pump(); SDL_Delay(10); }
        rom=sSelected;
        if(!rom) { SDL_Quit(); return 0; }
    }
    char resolved[4096]; if(!realpath(rom,resolved)) Fatal("The cartridge dump could not be opened.");
    size_t size; void* bytes=SDL_LoadFile(resolved,&size);
    unsigned char digest[CC_SHA1_DIGEST_LENGTH]; char hex[41];
    if(!bytes || size!=32u*1024u*1024u) Fatal("Expected a 32 MiB cartridge dump.");
    CC_SHA1(bytes,(CC_LONG)size,digest); SDL_free(bytes);
    for(int j=0;j<20;j++) SDL_snprintf(hex+j*2,3,"%02x",digest[j]);
    if(strcmp(hex,MACOS_ROM_SHA1)) Fatal("This dump does not match this build's cartridge region/hash. Build the matching region first.");
    SDL_setenv_unsafe("KHCOM_ROM",resolved,1); SDL_setenv_unsafe("KHCOM_DATA",sData,1);
    char assetsPath[4096];SDL_snprintf(assetsPath,sizeof(assetsPath),"%s/assets",SDL_GetBasePath());
    SDL_setenv_unsafe("KHCOM_ASSETS",assetsPath,1);
    Log("ROM verified: %s; renderer: %s",MACOS_REGION,SDL_GetRendererName(sRenderer));
    do {
        sExitCode=0; sFrames=0;
        m4_coreInstantiate(&sCore,NULL);
        if(!setjmp(sExit)) sExitCode=(int)m4_coreExport10_guestX5Fmain(&sCore);
        CleanupGuest(); m4_coreFreeInstance(&sCore);
    } while(sExitCode==75 && !sQuit);
    if(sPad) SDL_CloseGamepad(sPad);
    SDL_DestroyRenderer(sRenderer); SDL_DestroyWindow(sWindow); SDL_Quit();
    if(sLog) fclose(sLog);
    SDL_free((void*)sSelected);
    return sExitCode;
}
