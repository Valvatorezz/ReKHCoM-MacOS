/* ROM-free browser probe: the real SDL/Emscripten keyboard and touch paths. */
#include <emscripten.h>
#define main NativeControlsConformanceMain
#include "input_controls_conformance.c"
#undef main
#include "../android/native/touch_android.c"
void PortConfigSave(void){}
static SDL_Renderer* renderer;
static unsigned observed, presses, frames;
int PortUiActive(void){return 0;}
void PortUiOpenOptions(void){}
int PortUiHandleEvent(const void* p){(void)p;return 0;}
EMSCRIPTEN_KEEPALIVE void SmokeClear(void){
    SDL_Event e={0};e.type=SDL_EVENT_WINDOW_FOCUS_LOST;PortInputHandleEvent(&e);
}
static void frame(void){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        PortInputHandleEvent(&e);
        if(e.type==SDL_EVENT_KEY_DOWN)presses++;
    }
    unsigned keys=PortInputKeys();observed|=keys;frames++;
    SDL_SetRenderDrawColor(renderer,11,16,26,255);SDL_RenderClear(renderer);
    PortAndroidTouchDraw(renderer);SDL_RenderPresent(renderer);
    EM_ASM({document.getElementById('stats').textContent=JSON.stringify({held:$0,observed:$1,presses:$2,frames:$3});},keys,observed,presses,frames);
}
int main(void){
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT,"#canvas");
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY,"0");
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD|SDL_INIT_EVENTS));
    SDL_Window* window=SDL_CreateWindow("Controls probe",960,540,0);assert(window);
    renderer=SDL_CreateRenderer(window,NULL);assert(renderer);
    PortWebTouchEnable(1);emscripten_set_main_loop(frame,0,1);return 0;
}
