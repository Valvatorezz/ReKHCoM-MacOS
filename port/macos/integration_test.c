/* Exercises real native SDL -> offset bridge -> game input, and SRAM across
 * core reinstantiation. Always run in an empty isolated test directory. */
#define main ReKHCoMAppMain
#include "host.c"
#undef main
#include <assert.h>

static void TestBlendModes(void) {
    /* Half-transparent red over blue distinguishes alpha from additive blend. */
    uint32_t red = 0x800000ff;
    int tex = H_tex_create(&red, 1, 1, 0);
    assert(tex >= 0);
    const SDL_BlendMode modes[] = {
        SDL_BLENDMODE_NONE, SDL_BLENDMODE_BLEND, SDL_BLENDMODE_ADD, SDL_BLENDMODE_BLEND
    };
    const int expected[][3] = {{255,0,0}, {128,0,127}, {128,0,255}, {128,0,127}};
    for (int textured = 0; textured < 2; ++textured) {
        uint32_t tint = textured ? 0xffffffff : 0x80ff0000;
        HostVertex vertices[3] = {
            {0,0,0,0,tint}, {240,0,1,0,tint}, {0,160,0,1,tint}
        };
        for (unsigned i = 0; i < sizeof(modes)/sizeof(modes[0]); ++i) {
            H_gfx_begin(0xff0000ff);
            H_gfx_blend((int)modes[i]);
            H_gfx_draw(textured ? tex : -1, vertices, 3);
            SDL_Surface* surface = SDL_RenderReadPixels(sRenderer, NULL);
            assert(surface);
            Uint8 r,g,b,a;
            assert(SDL_ReadSurfacePixel(surface, 1, 1, &r, &g, &b, &a));
            assert(abs((int)r-expected[i][0]) <= 2);
            assert(abs((int)g-expected[i][1]) <= 2);
            assert(abs((int)b-expected[i][2]) <= 2);
            SDL_DestroySurface(surface);
        }
    }
    H_tex_free(tex);
    H_gfx_blend(SDL_BLENDMODE_BLEND);
    puts("PASS alpha/additive blend modes for textured and untextured triangles");
}

int main(void) {
    _Static_assert(sizeof(HostVertex)==20,"vertex ABI");
    _Static_assert(sizeof(HostPad)==20,"pad ABI");
    _Static_assert(sizeof(HostTouch)==12,"touch ABI");
    char cwd[4096];assert(getcwd(cwd,sizeof(cwd)));
    MacCrashInstall(cwd);struct sigaction action;assert(!sigaction(SIGSEGV,NULL,&action));
    assert(action.sa_sigaction==MacCrashSignal);MacCrashRecord(SIGSEGV,NULL,NULL);
    struct stat crash;assert(!stat("kh_com_crash.log",&crash)&&crash.st_size>0);
    puts("PASS macOS signal handler installation and crash record output");
    SDL_setenv_unsafe("KHCOM_DATA",cwd,1);
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD));
    sWindow=SDL_CreateWindow("Integration test",240,160,0);
    sRenderer=SDL_CreateRenderer(sWindow,NULL);assert(sRenderer);
    m4_coreInstantiate(&sCore,NULL);
    SDL_Event e={0};e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_X;e.key.scancode=SDL_SCANCODE_X;e.key.down=true;
    assert(SDL_PushEvent(&e));
    assert(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1);
    e.type=SDL_EVENT_KEY_UP;e.key.down=false;assert(SDL_PushEvent(&e));
    assert(!(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1));
    puts("PASS keyboard press/release through guest input handler");
    e.type=SDL_EVENT_KEY_DOWN;e.key.down=true;assert(SDL_PushEvent(&e));
    assert(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1);
    e.type=SDL_EVENT_WINDOW_FOCUS_LOST;assert(SDL_PushEvent(&e));
    assert(!(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1));
    e.type=SDL_EVENT_WINDOW_FOCUS_GAINED;assert(SDL_PushEvent(&e));
    m4_coreExport16_macosX5FinputX5Ftest(&sCore);
    puts("PASS held keyboard input clears on focus loss");
    SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="ReKHCoM test pad";
    desc.button_mask=(1u<<SDL_GAMEPAD_BUTTON_COUNT)-1;desc.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;
    SDL_JoystickID id=SDL_AttachVirtualJoystick(&desc);assert(id);
    SDL_Joystick* stick=SDL_OpenJoystick(id);assert(stick);
    if(sPad)SDL_CloseGamepad(sPad);sPad=SDL_OpenGamepad(id);assert(sPad);
    assert(SDL_SetJoystickVirtualButton(stick,SDL_GAMEPAD_BUTTON_SOUTH,true));SDL_UpdateJoysticks();
    assert(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1);
    assert(SDL_SetJoystickVirtualButton(stick,SDL_GAMEPAD_BUTTON_SOUTH,false));SDL_UpdateJoysticks();
    assert(!(m4_coreExport16_macosX5FinputX5Ftest(&sCore)&1));
    SDL_CloseJoystick(stick);assert(SDL_DetachVirtualJoystick(id));
    m4_coreExport16_macosX5FinputX5Ftest(&sCore);
    puts("PASS SDL virtual controller input and hot removal through guest");
    assert(H_audio_open(48000)==0);int16_t samples[960*2];
    for(int j=0;j<960;j++)samples[j*2]=samples[j*2+1]=(int16_t)(sin(j*2*3.141592653589793*440/48000)*1000);
    H_audio_push(samples,960);assert(H_audio_queued()>0);H_audio_ratio(1.01);H_audio_clear();assert(H_audio_queued()==0);
    puts("PASS audio stream open, queue, ratio and clear (dummy device)");
    uint32_t white=0xffffffff;int tex=H_tex_create(&white,1,1,0);assert(tex>=0);
    HostVertex vertices[3]={{0,0,0,0,0xffffffff},{240,0,1,0,0xffffffff},{0,160,0,1,0xffffffff}};
    H_gfx_begin(0xff000000);H_gfx_draw(tex,vertices,3);
    SDL_Surface* surface=SDL_RenderReadPixels(sRenderer,NULL);assert(surface);
    Uint8 r,g,b,a;assert(SDL_ReadSurfacePixel(surface,1,1,&r,&g,&b,&a));assert(r==255&&g==255&&b==255);
    assert(SDL_SaveBMP(surface,"bridge-render.bmp"));SDL_DestroySurface(surface);H_tex_free(tex);
    puts("PASS native triangle rendering and readback");
    TestBlendModes();
    assert(m4_coreExport15_macosX5FsramX5Ftest(&sCore)==0);
    m4_coreFreeInstance(&sCore);m4_coreInstantiate(&sCore,NULL);
    assert(m4_coreExport15_macosX5FsramX5Ftest(&sCore)==0);
    struct stat st;assert(!stat("kh_com_us.sav",&st)&&st.st_size==65536);
    puts("PASS normal SRAM atomic write/read across core reinstantiation");
    if(!setjmp(sExit)) { GuestSpan(&sCore.common,0xffffffff,8);assert(!"expected bounds trap"); }
    assert(sExitCode==2);puts("PASS offset bridge bounds check");
    CleanupGuest();m4_coreFreeInstance(&sCore);
    SDL_DestroyRenderer(sRenderer);SDL_DestroyWindow(sWindow);SDL_Quit();return 0;
}
