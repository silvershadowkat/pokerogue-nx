// SPDX-License-Identifier: AGPL-3.0-only
// Original SilverShadow SVG paths/material, adapted from the retained patch.
// No generated artwork, fallback PNGs, game assets or cheats are included.
#include "touch_art.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
static const char *cross =
    "M38 4Q34 4 34 8V34H8Q4 34 4 38V62Q4 66 8 66H34V92Q34 96 38 96H62Q66 96 66 "
    "92V66H92Q96 66 96 62V38Q96 34 92 34H66V8Q66 4 62 4Z";
static const char *chevrons[] = {"M42 20L50 12L58 20", "M80 42L88 50L80 58",
                                 "M42 80L50 88L58 80", "M20 42L12 50L20 58"};
static void path(plutovg_canvas_t *c, const char *svg, bool fill) {
    plutovg_path_t *p = plutovg_path_create();
    plutovg_path_parse(p, svg, -1);
    if (fill)
        plutovg_canvas_fill_path(c, p);
    else
        plutovg_canvas_stroke_path(c, p);
    plutovg_path_destroy(p);
}
uint8_t *touch_art_layer(int layer, int size) {
    plutovg_surface_t *surface = plutovg_surface_create(size, size);
    if (!surface)
        return NULL;
    plutovg_canvas_t *c = plutovg_canvas_create(surface);
    plutovg_canvas_scale(c, size / 100.f, size / 100.f);
    if (layer == TA_SOCKET) {
        const plutovg_gradient_stop_t stops[] = {
            {0, {.012f, .016f, .024f, .5f}},
            {.34f, {.012f, .016f, .024f, .5f}},
            {.52f, {.012f, .016f, .024f, .28f}},
            {.72f, {.012f, .016f, .024f, 0}},
            {1, {.012f, .016f, .024f, 0}}};
        plutovg_canvas_set_radial_gradient(c, 50, 50, 55.1543f, 50, 50, 0,
                                           PLUTOVG_SPREAD_METHOD_PAD, stops, 5,
                                           NULL);
        plutovg_canvas_circle(c, 50, 50, 39);
        plutovg_canvas_fill(c);
        plutovg_canvas_set_rgba(c, .867f, .894f, .929f, .2f);
        plutovg_canvas_set_line_width(c, .5f);
        plutovg_canvas_circle(c, 50, 50, 39);
        plutovg_canvas_stroke(c);
    } else {
        plutovg_canvas_translate(c, 4, 4);
        plutovg_canvas_scale(c, .92f, .92f);
        if (layer == TA_SHADOW) {
            plutovg_canvas_set_rgba(c, 0, 0, 0, .62f);
            plutovg_canvas_round_rect(c, 34, 0, 32, 100, 8, 8);
            plutovg_canvas_fill(c);
            plutovg_canvas_round_rect(c, 0, 34, 100, 32, 8, 8);
            plutovg_canvas_fill(c);
        } else if (layer == TA_FACE) {
            plutovg_canvas_set_rgba(c, 12 / 255.f, 14 / 255.f, 18 / 255.f, .9f);
            path(c, cross, true);
            plutovg_canvas_set_rgba(c, 221 / 255.f, 228 / 255.f, 237 / 255.f,
                                    .82f);
            plutovg_canvas_set_line_width(c, 1.8f);
            path(c, cross, false);
            plutovg_canvas_save(c);
            plutovg_canvas_translate(c, -.5f, -.7f);
            plutovg_canvas_set_rgba(c, 1, 1, 1, .42f);
            plutovg_canvas_set_line_width(c, .65f);
            path(c, cross, false);
            plutovg_canvas_restore(c);
            plutovg_canvas_set_rgba(c, 0, 0, 0, .58f);
            plutovg_canvas_set_line_width(c, 2);
            path(c, "M40 12H60V40H88V60H60V88H40V60H12V40H40Z", false);
            plutovg_canvas_set_rgba(c, 37 / 255.f, 41 / 255.f, 48 / 255.f, .9f);
            plutovg_canvas_circle(c, 50, 50, 10);
            plutovg_canvas_fill(c);
            plutovg_canvas_set_rgba(c, 221 / 255.f, 228 / 255.f, 237 / 255.f,
                                    .48f);
            plutovg_canvas_set_line_width(c, 1.4f);
            plutovg_canvas_circle(c, 50, 50, 10);
            plutovg_canvas_stroke(c);
        } else if (layer >= TA_UP && layer <= TA_LEFT) {
            plutovg_canvas_set_line_cap(c, PLUTOVG_LINE_CAP_ROUND);
            plutovg_canvas_set_line_join(c, PLUTOVG_LINE_JOIN_ROUND);
            plutovg_canvas_set_rgba(c, 240 / 255.f, 44 / 255.f, 62 / 255.f,
                                    .34f);
            plutovg_canvas_set_line_width(c, 7);
            path(c, chevrons[layer - TA_UP], false);
            plutovg_canvas_set_rgba(c, 240 / 255.f, 44 / 255.f, 62 / 255.f, 1);
            plutovg_canvas_set_line_width(c, 3.2f);
            path(c, chevrons[layer - TA_UP], false);
        }
    }
    uint8_t *rgba = malloc((size_t)size * size * 4);
    if (rgba)
        plutovg_convert_argb_to_rgba(rgba, plutovg_surface_get_data(surface),
                                     size, size,
                                     plutovg_surface_get_stride(surface));
    plutovg_canvas_destroy(c);
    plutovg_surface_destroy(surface);
    return rgba;
}
void touch_art_button(plutovg_canvas_t *c, float x, float y, float w, float h,
                      bool circle, bool pressed) {
    float radius = circle ? w / 2 : 4, depth = pressed ? 1 : 0;
    plutovg_canvas_save(c);
    plutovg_canvas_translate(c, x + (pressed ? .015f * w : 0),
                             y + depth + (pressed ? .015f * h : 0));
    if (pressed)
        plutovg_canvas_scale(c, .97f, .97f);
    const plutovg_gradient_stop_t stops[] = {
        {0,
         {pressed ? .184f : .145f, pressed ? .192f : .161f,
          pressed ? .216f : .188f, pressed ? .94f : .9f}},
        {pressed ? .7f : .64f,
         {pressed ? .027f : .047f, pressed ? .031f : .055f,
          pressed ? .043f : .071f, pressed ? .94f : .9f}}};
    float gx = w * (pressed ? .45f : .38f), gy = h * (pressed ? .38f : .28f),
          radius_max = hypotf(fmaxf(gx, w - gx), fmaxf(gy, h - gy));
    plutovg_canvas_set_radial_gradient(c, gx, gy, radius_max, gx, gy, 0,
                                       PLUTOVG_SPREAD_METHOD_PAD, stops, 2,
                                       NULL);
    plutovg_canvas_round_rect(c, 0, 0, w, h, radius, radius);
    plutovg_canvas_fill(c);
    plutovg_canvas_set_line_width(c, .7f);
    plutovg_canvas_set_rgba(c, pressed ? 240 / 255.f : 221 / 255.f,
                            pressed ? 44 / 255.f : 228 / 255.f,
                            pressed ? 62 / 255.f : 237 / 255.f,
                            pressed ? 1 : .82f);
    plutovg_canvas_round_rect(c, .5f, .5f, w - 1, h - 1, radius, radius);
    plutovg_canvas_stroke(c);
    plutovg_canvas_set_rgba(c, pressed ? 240 / 255.f : 221 / 255.f,
                            pressed ? 44 / 255.f : 228 / 255.f,
                            pressed ? 62 / 255.f : 237 / 255.f,
                            pressed ? .9f : .48f);
    plutovg_canvas_round_rect(c, 3, 3, w - 6, h - 6, radius, radius);
    plutovg_canvas_stroke(c);
    plutovg_canvas_restore(c);
}
