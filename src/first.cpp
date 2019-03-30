/*
    Copyright (C) 2019 Justas Dabrila (justasdabrila@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "stdio.h"
#include "stdlib.h"
#include <dlfcn.h>

#define intern static
#define extern_c extern "C"
#define force_export __attribute__ ((visibility ("default")))
#define force_inline inline __attribute__((always_inline)) 
#define baked static const 

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef bool b32;

#define assert_message(__EXPRESSION, __MESSAGE) assert_function(__EXPRESSION, __MESSAGE, __FILE__, __LINE__)
#define assert_no_message(__EXPRESSION) assert_function(__EXPRESSION, 0, __FILE__, __LINE__)

#define assert_macro_pick_overload(__ARG1, __ARG2, __ARG3, ...) __ARG3
#define assert(...) assert_macro_pick_overload(__VA_ARGS__, assert_message, assert_no_message)(__VA_ARGS__)

intern
void assert_function(b32 expression, const char* message, const char *file, s64 line) {
    if(expression) {
        return;
    }

    printf("Assertion failed!\n");
    printf("  At %s:%d\n", file, line);

    if(message) {
        printf("  Message:\n");
        printf("    %s\n", message); 
    }

    asm("int3");
    exit(1);
}


typedef xcb_generic_event_t * (*xcb_event_query_function)(xcb_connection_t*);

xcb_event_query_function xcb_original_wait_for_event;
xcb_event_query_function xcb_original_poll_for_event;
xcb_event_query_function xcb_original_poll_for_queued_event;

intern s32 xinput_opcode = 131; 
intern s32 keycode_min;
intern s32 keycode_max;
intern s32 num_keysyms_per_keycode;
intern KeySym *keysyms;

intern
void consume_xcb_event(xcb_generic_event_t * in_ev) {
    auto * ev = (xcb_client_message_event_t*)in_ev;
    ev->response_type = XCB_CLIENT_MESSAGE;
    ev->type = 0;
}

intern
KeySym keycode_to_keysym(s32 keycode) {
     return keysyms[(keycode - keycode_min) * num_keysyms_per_keycode + 0];
}

intern 
xcb_generic_event_t * event_middleman(xcb_connection_t * c, xcb_generic_event_t * ev) {
    if(ev->response_type == XCB_GE_GENERIC) {
        auto * e = (xcb_input_key_press_event_t*)ev;

        if(e->event_type == XCB_INPUT_KEY_RELEASE || e->event_type == XCB_INPUT_KEY_PRESS) {
            KeySym keysym = keycode_to_keysym(e->detail);

#define BLOCK_KEY(__key) if(keysym == __key) consume_xcb_event(ev)
            BLOCK_KEY(XK_Escape);
            BLOCK_KEY(XK_KP_Enter);
            BLOCK_KEY(XK_KP_Divide);
            BLOCK_KEY(XK_KP_Add);
            BLOCK_KEY(XK_KP_Multiply);
            BLOCK_KEY(XK_KP_Subtract);

            // NOTE(justas): keypad keys (XK_KP_0-9 aren't handled by gvim shrug)
            BLOCK_KEY(65436); 
            BLOCK_KEY(65433);
            BLOCK_KEY(65435);
            BLOCK_KEY(65430);
            BLOCK_KEY(65437);
            BLOCK_KEY(65432);
            BLOCK_KEY(65429);
            BLOCK_KEY(65431);
            BLOCK_KEY(65434);
            BLOCK_KEY(65439);
            BLOCK_KEY(65438);
#undef BLOCK_KEY
        }
    }
    return ev;
}

intern force_inline
xcb_generic_event_t * insert_event_middleman(xcb_connection_t * c, xcb_event_query_function original_fx) {
    auto *ev = (original_fx)(c);
    if(ev) {
        event_middleman(c, ev);
    }
    return ev;
}

extern_c force_export 
xcb_generic_event_t * xcb_wait_for_event (xcb_connection_t *c) {
    return insert_event_middleman(c, xcb_original_wait_for_event);
}

extern_c force_export 
xcb_generic_event_t * xcb_poll_for_event (xcb_connection_t *c) {
    return insert_event_middleman(c, xcb_original_poll_for_event);
}

extern_c force_export 
xcb_generic_event_t * 	xcb_poll_for_queued_event (xcb_connection_t *c) {
    return insert_event_middleman(c, xcb_original_poll_for_queued_event);
}

intern __attribute__((constructor))
void library_init_main_etc_theres_no_standard_for_this_so_whatever() { 
    {
        xcb_original_wait_for_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_wait_for_event");
        xcb_original_poll_for_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_poll_for_event");
        xcb_original_poll_for_queued_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_poll_for_queued_event");

        assert(xcb_original_wait_for_event);
        assert(xcb_original_poll_for_event);
        assert(xcb_poll_for_queued_event);
    }

    {
        auto * display = XOpenDisplay(0);
        XDisplayKeycodes(display, &keycode_min, &keycode_max);
        keysyms = XGetKeyboardMapping(display, keycode_min, keycode_max - keycode_min, &num_keysyms_per_keycode);
    }

}

intern __attribute__((destructor))
void library_destruct_exit_etc_theres_no_standard_for_this_so_whatever() {
    XFree(keysyms);
}


