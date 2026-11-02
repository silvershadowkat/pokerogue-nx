// SPDX-License-Identifier: AGPL-3.0-only
// Retained SilverShadow controller: cached vectors + GPU perspective/opacity.
#include "host.h"
#include "touch_art.h"
#include "touch_controls.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
touch_controls g_touch_controls;
static bool active;
static double last_release, fade_start;
static float fade = 1, fade_from = 1, fade_target = 1;
static tc_pose shown, from, target;
static double pose_start, pose_duration;
static unsigned last_epoch;
static struct {
    int id;
    bool active;
} contacts[16];
static bool contacts_down(void) {
    for (int i = 0; i < 16; i++)
        if (contacts[i].active)
            return true;
    return false;
}
static bool dpad_down(void) {
    for (int i = 0; i < 16; i++)
        if (g_touch_controls.fingers[i].active &&
            g_touch_controls.fingers[i].button == -2)
            return true;
    return false;
}
void touch_overlay_reset(void) {
    tc_release(&g_touch_controls);
    memset(contacts, 0, sizeof contacts);
}
void touch_overlay_set_mask(unsigned mask) {
    tc_set_visible(&g_touch_controls, mask);
}
void touch_overlay_active(bool enabled) {
    if (active && !enabled)
        touch_overlay_reset();
    active = enabled;
}
bool touch_overlay_event(const plat_event *ev) {
    if (!active || ev->type < PEV_POINTER_DOWN || ev->type > PEV_POINTER_UP)
        return false;
    int w, h;
    plat_drawable_size(&w, &h);
    tc_configure(&g_touch_controls, w, h, g_cfg.direct_touch, g_cfg.touch_pad);
    int kind = ev->type == PEV_POINTER_DOWN ? TC_DOWN
               : ev->type == PEV_POINTER_UP ? TC_UP
                                            : TC_MOVE;
    if (kind == TC_DOWN) {
        for (int i = 0; i < 16; i++)
            if (!contacts[i].active) {
                contacts[i].id = ev->pointer_id;
                contacts[i].active = true;
                break;
            }
    }
    if (kind == TC_UP) {
        for (int i = 0; i < 16; i++)
            if (contacts[i].active && contacts[i].id == ev->pointer_id)
                contacts[i].active = false;
    }
    bool consumed =
        tc_pointer(&g_touch_controls, kind, ev->pointer_id, ev->x, ev->y);
    // First touch wakes a faded pad without stealing direct menu gestures.
    if (kind == TC_DOWN) {
        fade = fade_from = fade_target = 1;
        last_release = host_now_ms();
    }
    if (kind == TC_UP && !contacts_down())
        last_release = host_now_ms();
    return consumed;
}
void touch_overlay_flush(JSContext *ctx) {
    int count = g_touch_controls.transition_count;
    g_touch_controls.transition_count = 0;
    tc_transition events[256];
    memcpy(events, g_touch_controls.transitions,
           (size_t)count * sizeof *events);
    for (int i = 0; i < count; i++) {
        tc_transition event = events[i];
        JSValue args[2] = {JS_NewInt32(ctx, event.button),
                           JS_NewBool(ctx, event.down)};
        host_call_rt(ctx, "onTouchControl", 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
    }
}
bool touch_overlay_visible(void) {
    return active && g_cfg.touch_pad &&
           (fade > .001f || g_cfg.touch_pad_always);
}
static double ease(double t) {
    if (t <= 0)
        return 0;
    if (t >= 1)
        return 1;
    // CSS ease-out cubic-bezier(0,0,.58,1), retained transition timings.
    double lo = 0, hi = 1, u = .5;
    for (int i = 0; i < 14; i++) {
        u = (lo + hi) / 2;
        double x = 3 * (1 - u) * u * u * .58 + u * u * u;
        if (x < t)
            lo = u;
        else
            hi = u;
    }
    return 3 * (1 - u) * u * u + u * u * u;
}
static void quad(float out[12], float x, float y, float size,
                 const tc_pose *pose, bool shadow) {
    double rx = pose ? pose->tilt_x * 3.141592653589793 / 180 : 0,
           ry = pose ? pose->tilt_y * 3.141592653589793 / 180 : 0;
    double scale = pose ? 1 - pose->depth : 1;
    for (int i = 0; i < 4; i++) {
        double a = ((i & 1) ? size / 2 : -size / 2) * scale,
               b = ((i >> 1) ? size / 2 : -size / 2) * scale, z = 0;
        if (pose && !shadow) {
            double xx = a * cos(ry), zz = -a * sin(ry);
            a = xx;
            z = b * sin(rx) + zz * cos(rx);
            b = b * cos(rx) - zz * sin(rx);
        }
        double weight = (430 - z) / 430;
        out[i * 3] = (float)(x + size / 2 + a / weight +
                             (shadow && pose ? pose->shadow_x : 0));
        out[i * 3 + 1] = (float)(y + size / 2 + b / weight +
                                 (shadow && pose ? pose->shadow_y : 0));
        out[i * 3 + 2] = (float)weight;
    }
}
static void localize(float p[12], float x, float y, float size) {
    for (int i = 0; i < 4; i++) {
        p[i * 3] = (p[i * 3] - (x - size * .1f)) * 512 / (size * 1.2f);
        p[i * 3 + 1] = (p[i * 3 + 1] - (y - size * .1f)) * 512 / (size * 1.2f);
    }
}
void touch_overlay_draw(void) {
    if (!touch_overlay_visible())
        return;
    int w, h;
    plat_drawable_size(&w, &h);
    if (w <= 0 || h <= 0)
        return;
    tc_configure(&g_touch_controls, w, h, g_cfg.direct_touch, g_cfg.touch_pad);
    double now = host_now_ms();
    if (!last_release)
        last_release = now;
    float goal = !g_cfg.touch_pad_always && !contacts_down() &&
                         now - last_release >= 2000
                     ? 0
                     : 1;
    if (goal != fade_target) {
        fade_from = fade;
        fade_target = goal;
        fade_start = now;
    }
    fade = fade_from +
           (fade_target - fade_from) * (float)ease((now - fade_start) / 180);
    if (fade < .001f)
        return;
    if (last_epoch != g_touch_controls.reset_epoch) {
        memset(&shown, 0, sizeof shown);
        memset(&from, 0, sizeof from);
        memset(&target, 0, sizeof target);
        last_epoch = g_touch_controls.reset_epoch;
    }
    // Sample the running transition BEFORE accepting another thumb position.
    // Restarting first makes progress zero on every moving frame, freezing the
    // artwork until the finger stops. CSS samples the current presentation
    // before retargeting; keep that behavior at every input/render rate.
    double progress =
        ease((now - pose_start) / (pose_duration ? pose_duration : 1));
#define LERP(field)                                                            \
    shown.field = from.field + (target.field - from.field) * progress
    LERP(nx);
    LERP(ny);
    LERP(distance);
    LERP(tilt_x);
    LERP(tilt_y);
    LERP(shadow_x);
    LERP(shadow_y);
    LERP(depth);
    for (int i = 0; i < 4; i++)
        LERP(light[i]);
#undef LERP
    if (memcmp(&target, &g_touch_controls.pose, sizeof target)) {
        from = shown;
        target = g_touch_controls.pose;
        pose_start = now;
        pose_duration = dpad_down() ? 38 : 115;
    }
    static uint8_t *layers[TA_LAYERS];
    for (int i = 0; i < TA_LAYERS; i++)
        if (!layers[i])
            layers[i] = touch_art_layer(i, 256);
    const tc_button *d = &g_touch_controls.dpad;
    bool owner = false;
    for (int i = 0; i < 16; i++)
        if (g_touch_controls.fingers[i].active &&
            g_touch_controls.fingers[i].button == -2)
            owner = true;
    float opacity =
        fade * (owner ? (g_touch_controls.dpad_button >= 0 ? .58f : .4f) : .3f);
    float points[12], size = d->w * .96f, x = d->x + d->w * .02f,
                      y = d->y + d->h * .02f;
    static tc_pose rendered_pose;
    static bool group_ready;
    static float rendered_size;
    if (!group_ready || rendered_size != size ||
        memcmp(&rendered_pose, &shown, sizeof shown)) {
        if (overlay_group_begin(512)) {
            quad(points, x, y, size, NULL, false);
            localize(points, x, y, size);
            overlay_rgba_quad(16, layers[TA_SOCKET], 256, 256, false, points,
                              1);
            quad(points, x, y, size, &shown, true);
            localize(points, x, y, size);
            overlay_rgba_quad(17, layers[TA_SHADOW], 256, 256, false, points,
                              1);
            quad(points, x, y, size, &shown, false);
            localize(points, x, y, size);
            overlay_rgba_quad(18, layers[TA_FACE], 256, 256, false, points, 1);
            // Warm every chevron texture before first input. Static poses reuse
            // the composite; moving poses change GPU geometry, never pixels.
            for (int i = 0; i < 4; i++)
                if (!group_ready || shown.light[i] > .001)
                    overlay_rgba_quad(19 + i, layers[TA_UP + i], 256, 256,
                                      false, points, (float)shown.light[i]);
            overlay_group_end();
            rendered_pose = shown;
            rendered_size = size;
            group_ready = true;
        }
    }
    // Composite opacity once, matching CSS group opacity rather than making
    // overlapping socket/face/shadow layers cumulatively more opaque.
    if (group_ready)
        overlay_group_draw(x - size * .1f, y - size * .1f, size * 1.2f,
                           size * 1.2f, opacity);
    // Action controls update only on pressed/visibility/size changes, never on
    // D-pad pose movement. Two cached layers preserve idle/active opacity.
    static uint8_t *images[2];
    static int old_h;
    static unsigned revision = ~0u;
    int iw = 640, ih = (int)(640.0 * h / w);
    bool changed = old_h != ih || revision != g_touch_controls.action_revision;
    if (old_h != ih) {
        for (int i = 0; i < 2; i++) {
            free(images[i]);
            images[i] = calloc((size_t)iw * ih, 4);
        }
        old_h = ih;
    }
    if (!images[0] || !images[1])
        return;
    static plutovg_font_face_t *font;
    if (!font) {
        char p[900];
        snprintf(p, sizeof p, "%s/launcher/emerald.ttf", g_cfg.app_dir);
        font = plutovg_font_face_load_from_file(p, 0);
    }
    if (changed)
        for (int pass = 0; pass < 2; pass++) {
            memset(images[pass], 0, (size_t)iw * ih * 4);
            plutovg_surface_t *surface =
                plutovg_surface_create_for_data(images[pass], iw, ih, iw * 4);
            plutovg_canvas_t *c = plutovg_canvas_create(surface);
            for (int n = 0; n < g_touch_controls.count; n++) {
                const tc_button *button = &g_touch_controls.layout[n];
                bool pressed = g_touch_controls.buttons[button->button] > .5f;
                if (!(g_touch_controls.visible_mask & (1u << button->button)) ||
                    pressed != (pass == 1))
                    continue;
                float bx = button->x * iw / w, by = button->y * iw / w,
                      bw = button->w * iw / w, bh = button->h * iw / w;
                touch_art_button(c, bx, by, bw, bh, button->button < 2,
                                 pressed);
                if (font) {
                    float fs = button->button < 2 ? bw : bh;
                    if (button->button == 9)
                        fs = bh * .72f;
                    plutovg_rect_t bounds;
                    plutovg_font_face_text_extents(font, fs, button->label, -1,
                                                   PLUTOVG_TEXT_ENCODING_UTF8,
                                                   &bounds);
                    plutovg_canvas_set_font(c, font, fs);
                    plutovg_canvas_set_rgba(c, .97f, .98f, .99f, .98f);
                    plutovg_canvas_fill_text(
                        c, button->label, -1, PLUTOVG_TEXT_ENCODING_UTF8,
                        bx + (bw - bounds.w) / 2 - bounds.x,
                        by + (bh - bounds.h) / 2 - bounds.y);
                }
            }
            plutovg_canvas_destroy(c);
            plutovg_surface_destroy(surface);
            plutovg_convert_argb_to_rgba(images[pass], images[pass], iw, ih,
                                         iw * 4);
        }
    revision = g_touch_controls.action_revision;
    float screen[12] = {0, 0,        1, (float)w, 0,        1,
                        0, (float)h, 1, (float)w, (float)h, 1};
    overlay_rgba_quad(23, images[0], iw, ih, changed, screen, fade * .3f);
    overlay_rgba_quad(24, images[1], iw, ih, changed, screen, fade * .58f);
}
