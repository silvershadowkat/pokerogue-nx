// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <stdbool.h>
enum { TC_DOWN, TC_MOVE, TC_UP };
typedef struct { float x, y, w, h; int button; const char *label; } tc_button;
typedef struct { int id, button; bool active; } tc_finger;
typedef struct {
    int width, height, count;
    bool direct, pad;
    tc_button layout[16];
    tc_finger fingers[16];
    float buttons[18];
    unsigned revision;
} touch_controls;
void tc_configure(touch_controls *s, int width, int height, bool direct, bool pad);
bool tc_pointer(touch_controls *s, int kind, int id, float x, float y);
void tc_release(touch_controls *s);
