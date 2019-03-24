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
#include <X11/Xlib.h>
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

intern s32 xinput_opcode = 131; // @HACK @TEMPORARY

intern 
xcb_generic_event_t * event_middleman(xcb_connection_t * c, xcb_generic_event_t * ev) {
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
    xcb_original_wait_for_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_wait_for_event");
    xcb_original_poll_for_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_poll_for_event");
    xcb_original_poll_for_queued_event = (xcb_event_query_function)dlsym(RTLD_NEXT, "xcb_poll_for_queued_event");

    assert(xcb_original_wait_for_event);
    assert(xcb_original_poll_for_event);
    assert(xcb_poll_for_queued_event);
}

intern __attribute__((destructor))
void library_destruct_exit_etc_theres_no_standard_for_this_so_whatever() {
}


