/* Exercise actual centered hit testing at multiple aspect ratios. */
#define PORT_TOUCH_CENTERED_LAYOUT
#include <assert.h>
#include <stdio.h>
#include "../android/native/touch_android.c"
PortConfig gPortConfig;
int PortUiActive(void){return 0;}
void PortUiOpenOptions(void){}
int PortUiHandleEvent(const void* event){(void)event;return 0;}
static void point(int id,float x,float y) {
    SDL_Event event={0};event.type=SDL_EVENT_FINGER_DOWN;event.tfinger.fingerID=id;
    event.tfinger.x=x;event.tfinger.y=y;PortAndroidTouchEvent(&event);
}
static void release(int id,SDL_EventType type) {
    SDL_Event event={0};event.type=type;event.tfinger.fingerID=id;
    PortAndroidTouchEvent(&event);
}
int main(void) {
    int sizes[][2]={{1920,1080},{5120,1440},{900,1600},{360,640},{1052,592}},i;
    for(i=0;i<5;i++) {
        float w=sizes[i][0],h=sizes[i][1];CenterLayout((int)w,(int)h);
        assert(SDL_fabsf((layoutX+layoutW*.5f)-w*.5f)<.1f);
        assert(SDL_fabsf((layoutY+layoutH*.5f)-h*.5f)<.1f);
        assert(layoutX>=0 && layoutY>=0);
        point(1,(layoutX+layoutW*.91f)/w,(layoutY+layoutH*.62f)/h);
        assert(PortAndroidTouchKeys()==1);PortAndroidTouchClear();
        point(2,(layoutX+layoutW*.07f)/w,(layoutY+layoutH*.63f)/h);
        assert(PortAndroidTouchKeys()==(0x20|0x40));PortAndroidTouchClear();
        /* Every labelled and shortcut button uses the same geometry as drawing. */
        for(int b=0;b<(int)SDL_arraysize(buttons)-1;b++) {
            point(10,(layoutX+layoutW*buttons[b].x)/w,(layoutY+layoutH*buttons[b].y)/h);
            assert(PortAndroidTouchKeys()==buttons[b].bits);
            release(10,SDL_EVENT_FINGER_UP);assert(!PortAndroidTouchKeys());
        }
        point(20,(layoutX+layoutW*.91f)/w,(layoutY+layoutH*.62f)/h);
        point(21,(layoutX+layoutW*.87f)/w,(layoutY+layoutH*.34f)/h);
        assert(PortAndroidTouchKeys()==(1|0x100));
        release(20,SDL_EVENT_FINGER_CANCELED);assert(PortAndroidTouchKeys()==0x100);
        SDL_Event lost={0};lost.type=SDL_EVENT_WINDOW_FOCUS_LOST;
        PortAndroidTouchEvent(&lost);assert(!PortAndroidTouchKeys());
    }
    puts("Web touch: 5 aspect ratios, all 8 buttons, diagonals, multi-touch, cancel and focus release PASS");return 0;
}
