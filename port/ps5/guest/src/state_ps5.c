/*
 * Save states.
 *
 * A state is taken and restored at one fixed point: the main loop's wait for
 * the vertical blank, where the native call stack is always the same
 * (main -> AgbMain -> VBlankIntrWait). It holds the GBA memory regions, the
 * game's globals (the .data/.bss group from port/game_prefix.h) and the
 * platform layer's own hardware state. The game image has a fixed base,
 * so every pointer inside the state stays valid for the same build.
 *
 * SRAM is deliberately not part of a state: loading one never touches the
 * player's real save file.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/port_state.h"
#include "port/port_ui.h"
#include "bridge_calls.h"

#define MAX_PATH 1024

#ifdef __MACH__
extern char __text_start[] __asm("section$start$__TEXT$__text");
extern char __text_end[] __asm("section$end$__TEXT$__text");
#else
extern char __text_start[], __text_end[];
#endif

extern void AgbMain(void);

#define STATE_MAGIC 0x5453484Bu /* "KHST" */
#define STATE_VERSION 1

typedef struct Region {
    uint32_t base;
    uint32_t size;
} Region;

static const Region sRegions[] = {
    { GBA_EWRAM, GBA_EWRAM_SIZE }, { GBA_IWRAM, GBA_IWRAM_SIZE }, { GBA_IO, GBA_IO_SIZE },
    { GBA_PLTT, GBA_PLTT_SIZE },   { GBA_VRAM, GBA_VRAM_SIZE },   { GBA_OAM, GBA_OAM_SIZE },
};

int gPortInMainLoop;
int gPortStateSlot = 1;
static int sPendingSave, sPendingLoad;
static char sPendingPath[MAX_PATH];
static FILE* sFile;
static int sOk;

/* States hold raw pointers into the game's code and data, so they are only
   valid for an identical game image: hash the game's code and its globals' layout. */
static uint32_t BuildId(void) {
    static uint32_t sId;
    char *data, *bss;
    size_t dataSize, bssSize;
    const uint8_t* p;
    uint32_t h = 2166136261u;

    if (sId)
        return sId;
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    for (p = (const uint8_t*)__text_start; p < (const uint8_t*)__text_end; p++)
        h = (h ^ *p) * 16777619u;
    h ^= (uint32_t)(uintptr_t)data ^ (uint32_t)dataSize * 31u ^ (uint32_t)(uintptr_t)bss ^ (uint32_t)bssSize * 7u;
    sId = h ^ STATE_VERSION;
    return sId;
}

static void VisitWrite(void* p, size_t n) {
    if (sOk && fwrite(p, 1, n, sFile) != n)
        sOk = 0;
}

static void VisitRead(void* p, size_t n) {
    if (sOk && fread(p, 1, n, sFile) != n)
        sOk = 0;
}

static void VisitAll(PortStateVisitor visit) {
    size_t i;

    for (i = 0; i < sizeof(sRegions) / sizeof(sRegions[0]); i++)
        visit((void*)(uintptr_t)sRegions[i].base, sRegions[i].size);
    {
        char *data, *bss;
        size_t dataSize, bssSize;
        PortGameSections(&data, &dataSize, &bss, &bssSize);
        visit(data, dataSize);
        visit(bss, bssSize);
    }
    PortDmaVisitState(visit);
    PortPpuVisitState(visit);
    PortAudioVisitState(visit);
}

const char* PortStatePath(int slot) {
    static char path[MAX_PATH];

    h_mkdir(PortConfigPath("states"));
    SDL_snprintf(path, sizeof(path), "%s", PortConfigPath("states/slot0.khst"));
    path[strlen(path) - 6] = (char)('0' + slot % 10);
    return path;
}

static void MakeParentDir(const char* path) {
    char dir[MAX_PATH];
    char* slash;

    SDL_strlcpy(dir, path, sizeof(dir));
    slash = SDL_strrchr(dir, '/');
    if (SDL_strrchr(dir, '\\') > slash)
        slash = SDL_strrchr(dir, '\\');
    if (slash != NULL) {
        *slash = 0;
        h_mkdir(dir);
    }
}

static int Save(const char* path) {
    uint32_t header[3] = { STATE_MAGIC, STATE_VERSION, BuildId() };

    MakeParentDir(path);
    sFile = fopen(path, "wb");
    if (sFile == NULL)
        return 0;
    sOk = 1;
    VisitWrite(header, sizeof(header));
    VisitAll(VisitWrite);
    if (fclose(sFile) != 0)
        sOk = 0;
    return sOk;
}

static int Load(const char* path) {
    uint32_t header[3];

    sFile = fopen(path, "rb");
    if (sFile == NULL)
        return 0;
    sOk = 1;
    VisitRead(header, sizeof(header));
    if (!sOk || header[0] != STATE_MAGIC || header[1] != STATE_VERSION || header[2] != BuildId()) {
        fclose(sFile);
        PortLog("state: %s is from another build", path);
        return -1;
    }
    VisitAll(VisitRead);
    fclose(sFile);
    PortAudioReset();
    return sOk;
}

void PortStateRequestSave(const char* path) {
    SDL_strlcpy(sPendingPath, path ? path : PortStatePath(gPortStateSlot), sizeof(sPendingPath));
    sPendingSave = 1;
}

void PortStateRequestLoad(const char* path) {
    SDL_strlcpy(sPendingPath, path ? path : PortStatePath(gPortStateSlot), sizeof(sPendingPath));
    sPendingLoad = 1;
}

/* Boss rematches (port_rematch.c): a request without notices, its result kept,
   as port_state.c does on PC. */
static int sQuiet, sLastResult;

void PortStateRequestQuiet(int load, const char* path) {
    SDL_strlcpy(sPendingPath, path, sizeof(sPendingPath));
    if (load) sPendingLoad = 1; else sPendingSave = 1;
    sQuiet = 1;
}

int PortStateBusy(void) {
    return sPendingSave || sPendingLoad;
}

int PortStateLastResult(void) {
    return sLastResult;
}

/* Called at the top of every frame; acts only at the main loop's wait. */
void PortStateService(void) {
    char msg[96];
    int r;

    if (!gPortInMainLoop || (!sPendingSave && !sPendingLoad))
        return;
    if (sPendingSave) {
        sPendingSave = 0;
        r = Save(sPendingPath);
        PortLog("state: save %s -> %s", sPendingPath, r ? "ok" : "failed");
        sLastResult = r;
        SDL_snprintf(msg, sizeof(msg), r ? "State %d saved" : "State %d: save failed", gPortStateSlot);
        if (!sQuiet)
            PortUiNotice(msg);
    }
    if (sPendingLoad) {
        sPendingLoad = 0;
        r = Load(sPendingPath);
        PortLog("state: load %s -> %d", sPendingPath, r);
        sLastResult = r;
        SDL_snprintf(msg, sizeof(msg),
                     r > 0 ? "State %d loaded" : (r < 0 ? "State %d: other version" : "State %d: empty"),
                     gPortStateSlot);
        if (!sQuiet)
            PortUiNotice(msg);
    }
    sQuiet = 0;
}
