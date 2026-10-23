#ifndef REKHCOM_MACOS_KEYBOARD_H
#define REKHCOM_MACOS_KEYBOARD_H
#include <stdint.h>
int h_keyboard(uint8_t*, int);
int h_key_event(uint32_t*); /* key, scancode, down, modifiers, repeat */
int h_scancode(uint32_t);
#endif
int h_audio_gain(float);
int h_fullscreen(int);
int h_vsync(int);
int h_window_size(int,int);
