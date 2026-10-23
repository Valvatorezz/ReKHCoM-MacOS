/* ROM-free GPU/font smoke. Synthetic PPU glyphs, real shared HD layout. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdarg.h>
#include "port/port.h"
PortConfig gPortConfig;
void PortHdTextDraw(SDL_Renderer* renderer,int layer,float x,float y,float scale);
static SDL_Window* window;
static SDL_Renderer* renderer;
static unsigned frames;
void PortLog(const char* fmt,...){(void)fmt;}
uint32_t PortFrameCount(void){return frames;}
const uint16_t* PortLatinGlyphWidths(void){static uint16_t widths[256];int i;for(i=0;i<256;i++)widths[i]=7;return widths;}
int PortPpuHudDigits(PortHudDigit* out,int max){(void)out;(void)max;return 0;}
int PortPpuHudDigitVisible(int digit,int x,int y){(void)digit;(void)x;(void)y;return 1;}
int PortPpuTextGlyphs(PortTextGlyph* out,int max,int hud){
    static const unsigned char* const lines[]={
        (const unsigned char*)"Hello, Sora! Kingdom Hearts.",
        (const unsigned char*)"\xA1Hola! \xBFQu\xE9 tal, Sora?",
        (const unsigned char*)"Comic Hearts: movie subtitles"
    };
    int row,i,n=0;(void)max;
    if(hud)return 0;
    for(row=0;row<3;row++)for(i=0;lines[row][i];i++) {
        PortTextGlyph g={0};g.x=8+i*7;g.y=20+row*36;g.code=lines[row][i];
        g.face=0xFFFFFF;g.shadow=0x301500;g.style=row==2?1:0;out[n++]=g;
    }
    return n;
}
EMSCRIPTEN_KEEPALIVE void SmokeResize(int w,int h){
    emscripten_set_canvas_element_size("#canvas",w,h);SDL_SetWindowSize(window,w,h);
}
static void frame(void){
    SDL_Event event;int w,h;
    while(SDL_PollEvent(&event)){}
    SDL_GetRenderOutputSize(renderer,&w,&h);
    SDL_SetRenderDrawColor(renderer,32,47,72,255);SDL_RenderClear(renderer);
    PortHdTextDraw(renderer,0,0,0,h/160.0f);SDL_RenderPresent(renderer);frames++;
    EM_ASM({document.getElementById('stats').textContent='Native SDL: '+$0+' × '+$1+' · frames '+$2;},w,h,frames);
}
int main(int argc,char** argv){
    (void)argc;(void)argv;
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY,"0");SDL_Init(SDL_INIT_VIDEO);
    window=SDL_CreateWindow("HD browser smoke",960,540,SDL_WINDOW_RESIZABLE);
    renderer=SDL_CreateRenderer(window,NULL);gPortConfig.hdText=1;
    emscripten_set_main_loop(frame,0,1);return 0;
}
