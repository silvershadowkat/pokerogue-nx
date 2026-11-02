// SPDX-License-Identifier: AGPL-3.0-only
// Gen1Recomp-derived SilverShadow rules; see THIRD_PARTY_TOUCH_NOTICES.md.
#pragma once
#include <stdbool.h>
enum { TC_DOWN, TC_MOVE, TC_UP, TC_CANCEL };
typedef struct {
    float x, y, w, h;
    int button;
    const char *label;
} tc_button;
typedef struct {
    int id, button;
    bool active;
} tc_finger;
typedef struct {
    double nx, ny, distance, tilt_x, tilt_y, shadow_x, shadow_y, depth,
        light[4];
} tc_pose;
typedef struct {
    int button;
    bool down;
} tc_transition;
typedef struct {
    int width, height, count;
    bool direct, pad;
    tc_button layout[16], dpad;
    tc_finger fingers[16];
    int dpad_button;
    tc_pose pose;
    float buttons[18];
    tc_transition transitions[256];
    int transition_count;
    unsigned revision, reset_epoch, action_revision;
    unsigned visible_mask;
    int held_order[18], held_count;
} touch_controls;
int tc_resolve(double dx, double dy, double width);
tc_pose tc_visual_pose(double dx, double dy, double width, int direction);
void tc_configure(touch_controls *s, int width, int height, bool direct,
                  bool pad);
bool tc_pointer(touch_controls *s, int kind, int id, float x, float y);
void tc_release(touch_controls *s);
void tc_set_visible(touch_controls *s, unsigned mask);
