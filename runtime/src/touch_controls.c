// SPDX-License-Identifier: AGPL-3.0-only
// Native translation of SilverShadow's proven input/pose helpers, without
// tuning changes. Gen1Recomp portions (c) 2026 BOIS CLUB GAMES, LLC, MIT; see
// notices.
#include "touch_controls.h"
#include <math.h>
#include <string.h>
static double clamp(double v, double lo, double hi) {
    return v < lo ? lo : v > hi ? hi : v;
}
int tc_resolve(double x, double y, double width) {
    if (!isfinite(x) || !isfinite(y) || !isfinite(width) || width <= 0)
        return -1;
    double dead = width * .16;
    if (fabs(x) < dead && fabs(y) < dead)
        return -1;
    if (fabs(x) >= fabs(y))
        return x > 0 ? 15 : 14;
    return y > 0 ? 13 : 12;
}
tc_pose tc_visual_pose(double x, double y, double width, int direction) {
    tc_pose p = {0};
    if (!isfinite(x) || !isfinite(y) || !isfinite(width) || width <= 0)
        return p;
    double distance = hypot(x, y);
    if (!isfinite(distance) || distance == 0)
        return p;
    double n = clamp(distance / (width / 2), 0, 1), ux = x / distance,
           uy = y / distance, response = n * n * (3 - 2 * n);
    double tilt = direction < 0 ? fmin(8.5 * response, 1) : 8.5 * response;
    p.nx = ux * n;
    p.ny = uy * n;
    p.distance = n;
    p.tilt_x = -uy * tilt;
    p.tilt_y = ux * tilt;
    p.shadow_x = -ux * 4 * response;
    p.shadow_y = -uy * 4 * response;
    p.depth = .014 * response;
    int horizontal = ux < 0 ? 3 : 1, vertical = uy < 0 ? 0 : 2;
    if (direction < 0) {
        p.light[vertical] = fmin(response * fabs(uy), .12);
        p.light[horizontal] = fmin(response * fabs(ux), .12);
        return p;
    }
    bool pv = direction == 12 || direction == 13;
    int primary = direction == 12   ? 0
                  : direction == 15 ? 1
                  : direction == 13 ? 2
                                    : 3,
        secondary = pv ? horizontal : vertical;
    p.light[primary] =
        clamp(response * (.72 + .28 * (pv ? fabs(uy) : fabs(ux))), 0, 1);
    p.light[secondary] =
        fmin(clamp(response * (pv ? fabs(ux) : fabs(uy)) * .55, 0, 1),
             p.light[primary] * .8);
    return p;
}
static void queue(touch_controls *s, int button, bool down) {
    if (s->transition_count >= 256) {
        // Recover to an explicit snapshot if an event flood exceeds the queue.
        s->transition_count = 0;
        for (int i = 0; i < 18; i++)
            s->transitions[s->transition_count++] = (tc_transition){i, false};
        for (int i = 0; i < 18; i++)
            if (s->buttons[i])
                s->transitions[s->transition_count++] =
                    (tc_transition){i, true};
    }
    s->transitions[s->transition_count++] = (tc_transition){button, down};
    if (down) {
        if (s->held_count < 18)
            s->held_order[s->held_count++] = button;
    } else
        for (int i = 0; i < s->held_count; i++)
            if (s->held_order[i] == button) {
                memmove(s->held_order + i, s->held_order + i + 1,
                        (size_t)(s->held_count - i - 1) * sizeof(int));
                s->held_count--;
                break;
            }
}
static void rebuild(touch_controls *s) {
    float next[18] = {0};
    for (int i = 0; i < 16; i++)
        if (s->fingers[i].active) {
            int button = s->fingers[i].button;
            if (button == -2)
                button = s->dpad_button;
            if (button >= 0)
                next[button] = 1;
        }
    // Release the previous direction before pressing the new one, exactly as
    // the source helper does. The renderer/gamepad loop never determines order.
    for (int i = 0; i < 18; i++)
        if (s->buttons[i] && !next[i])
            queue(s, i, false);
    for (int i = 0; i < 18; i++)
        if (!s->buttons[i] && next[i])
            queue(s, i, true);
    if (memcmp(next, s->buttons, 10 * sizeof(float)))
        s->action_revision++;
    if (memcmp(next, s->buttons, sizeof next)) {
        memcpy(s->buttons, next, sizeof next);
        s->revision++;
    }
}
static void arrange(touch_controls *s) {
    if (s->count != 10)
        return;
    float size = s->layout[0].w, small = size / 3, padding = 16;
    int ids[6], count = 0;
    for (int i = 2; i < 8; i++)
        if (s->visible_mask & (1u << i))
            ids[count++] = i;
    int rows = count > 3 ? 2 : 1;
    for (int n = 0; n < count; n++) {
        int row = n / 3, col = n % 3, columns = count - row * 3;
        if (columns > 3)
            columns = 3;
        s->layout[ids[n]].x = s->width - padding - 2.3f * size +
                              (col + .5f) * (1.2f * size / columns) - small / 2;
        s->layout[ids[n]].y = s->height - padding - 1.9f * size +
                              (row + .5f) * (.8f * size / rows) - small / 2;
    }
    bool c = (s->visible_mask & (1u << 8)) != 0;
    for (int i = 8; i < 10; i++) {
        s->layout[i].x = s->width - padding - .87f * size;
        s->layout[i].w = .74f * size;
        s->layout[i].y = s->height - padding - .9f * size +
                         ((c ? (i - 8 + .5f) / 2 : .5f) * .8f * size) -
                         small / 2;
    }
}
void tc_release(touch_controls *s) {
    while (s->held_count)
        queue(s, s->held_order[0], false);
    memset(s->fingers, 0, sizeof s->fingers);
    s->dpad_button = -1;
    memset(&s->pose, 0, sizeof s->pose);
    memset(s->buttons, 0, sizeof s->buttons);
    s->revision++;
    s->reset_epoch++;
    s->action_revision++;
}
void tc_set_visible(touch_controls *s, unsigned mask) {
    if (s->visible_mask == mask)
        return;
    s->visible_mask = mask;
    s->revision++;
    s->action_revision++;
    arrange(s);
    for (int i = 0; i < 16; i++)
        if (s->fingers[i].active && s->fingers[i].button >= 0 &&
            !(mask & (1u << s->fingers[i].button)))
            s->fingers[i].active = false;
    rebuild(s);
}
static void add(touch_controls *s, float x, float y, float w, float h,
                int button, const char *label) {
    s->layout[s->count++] = (tc_button){x, y, w, h, button, label};
}
void tc_configure(touch_controls *s, int width, int height, bool direct,
                  bool pad) {
    if (s->width == width && s->height == height && s->direct == direct &&
        s->pad == pad)
        return;
    tc_release(s);
    s->width = width;
    s->height = height;
    s->direct = direct;
    s->pad = pad;
    s->count = 0;
    s->revision++;
    if (!s->visible_mask)
        s->visible_mask = 0x3ff;
    // Landscape upstream default: controls-size 20vh, D-pad assembly 2.2x.
    // In portrait upstream uses 10vh; clamp only to keep the full region on
    // screen.
    float size = height * (width >= height ? .20f : .10f), padding = 16.f;
    if (2.2f * size > width - 2 * padding)
        size = (width - 2 * padding) / 2.2f;
    float d = 2.2f * size;
    s->dpad = (tc_button){padding, height - padding - d, d, d, -2, "D-pad"};
    add(s, width - padding - size, height - padding - 2 * size, size, size, 0,
        "A");
    add(s, width - padding - 2.2f * size, height - padding - size, size, size,
        1, "B");
    float small = size / 3, step = small * 1.18f,
          left = width - padding - 2.35f * size,
          top = height - padding - 1.9f * size;
    const char *labels[] = {"F", "G", "R", "E", "N", "V"};
    for (int i = 0; i < 6; i++)
        add(s, left + (i % 3) * step, top + (i / 3) * step, small, small, i + 2,
            labels[i]);
    add(s, width - padding - .8f * size, height - padding - .75f * size,
        .8f * size, small, 8, "C");
    add(s, width - padding - .8f * size, height - padding - small, .8f * size,
        small, 9, "Start");
    arrange(s);
}
static bool inside(const tc_button *b, float x, float y, float slop) {
    return x >= b->x - b->w * slop && x < b->x + b->w * (1 + slop) &&
           y >= b->y - b->h * slop && y < b->y + b->h * (1 + slop);
}
static void move_dpad(touch_controls *s, float x, float y) {
    double dx = x - (s->dpad.x + s->dpad.w / 2),
           dy = y - (s->dpad.y + s->dpad.h / 2);
    s->dpad_button = tc_resolve(dx, dy, s->dpad.w * .84);
    s->pose = tc_visual_pose(dx, dy, s->dpad.w * .96, s->dpad_button);
}
bool tc_pointer(touch_controls *s, int kind, int id, float x, float y) {
    tc_finger *finger = NULL;
    bool owned = false;
    for (int i = 0; i < 16; i++)
        if (s->fingers[i].active) {
            if (s->fingers[i].id == id)
                finger = &s->fingers[i];
            if (s->fingers[i].button == -2)
                owned = true;
        }
    if (finger) {
        if (kind == TC_UP || kind == TC_CANCEL) {
            finger->active = false;
            if (finger->button == -2) {
                s->dpad_button = -1;
                memset(&s->pose, 0, sizeof s->pose);
            }
        } else if (finger->button == -2)
            move_dpad(s, x, y);
        // Action ownership stays with its original button, even outside it.
        rebuild(s);
        return true;
    }
    if (kind != TC_DOWN)
        return !s->direct;
    int button = -1;
    if (s->pad) {
        if (inside(&s->dpad, x, y, 0))
            button = owned ? -3 : -2;
        else
            for (int i = s->count - 1; i >= 0; i--)
                if ((s->visible_mask & (1u << s->layout[i].button)) &&
                    inside(&s->layout[i], x, y, .15f)) {
                    button = s->layout[i].button;
                    break;
                }
    }
    if (button == -1)
        return !s->direct;
    for (int i = 0; i < 16; i++)
        if (!s->fingers[i].active) {
            s->fingers[i] = (tc_finger){id, button, true};
            if (button == -2)
                move_dpad(s, x, y);
            rebuild(s);
            return true;
        }
    return true;
}
