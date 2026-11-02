// SPDX-License-Identifier: AGPL-3.0-only
// Native feedback across the renderer-created/game-not-ready startup gap.
#include "gles.h"
#include "host.h"
#include "plutovg.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool pending;
static double started;
static int last_tick = -1;
static uint8_t pixels[256 * 40 * 4];
static plutovg_font_face_t *font;
static bool tried_font;

void startup_loading_begin(void) {
    pending = true;
    started = host_now_ms();
    last_tick = -1;
}
bool startup_loading_active(void) { return pending; }
void startup_loading_ready(void) {
    if (pending)
        host_log("[startup] game UI rendered after %.2f s", (host_now_ms() - started) / 1000);
    pending = false;
}
void startup_loading_draw(bool clear) {
    if (!pending)
        return;
    int w, h;
    plat_drawable_size(&w, &h);
    if (w <= 0 || h <= 0)
        return;
    if (!tried_font) {
        tried_font = true;
        char path[1000];
        snprintf(path, sizeof path, "%s/launcher/emerald.ttf",
                 g_cfg.app_dir ? g_cfg.app_dir : g_cfg.data_dir);
        font = plutovg_font_face_load_from_file(path, 0);
    }
    int tick = (int)((host_now_ms() - started) / 350) % 4;
    bool changed = tick != last_tick;
    if (changed) {
        static const char *labels[] = {"Loading", "Loading.", "Loading..", "Loading..."};
        memset(pixels, 0, sizeof pixels);
        if (font) {
            plutovg_surface_t *s = plutovg_surface_create_for_data(pixels, 256, 40, 256 * 4);
            plutovg_canvas_t *c = plutovg_canvas_create(s);
            plutovg_canvas_set_font(c, font, 24);
            plutovg_canvas_set_rgba(c, 0, 0, 0, .9f);
            plutovg_canvas_fill_text(c, labels[tick], -1, PLUTOVG_TEXT_ENCODING_UTF8, 3, 31);
            plutovg_canvas_set_rgba(c, 1, 1, 1, 1);
            plutovg_canvas_fill_text(c, labels[tick], -1, PLUTOVG_TEXT_ENCODING_UTF8, 2, 30);
            plutovg_canvas_destroy(c);
            plutovg_surface_destroy(s);
            plutovg_convert_argb_to_rgba(pixels, pixels, 256, 40, 256 * 4);
        } else {
            const uint8_t white[] = {255, 255, 255, 255};
            overlay_blit_text(pixels, 256, 40, 2, 12, labels[tick], 2, white);
        }
        last_tick = tick;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendEquation(GL_FUNC_ADD);
    if (clear) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    // Anchor INSIDE the game's 16:9 viewport, never in optional letterbox bars.
    float scale = fminf(w / 960.f, h / 540.f);
    float x = (w - 960 * scale) / 2 + 12 * scale;
    float y = (h - 540 * scale) / 2 + (540 - 12 - 40) * scale;
    overlay_rgba_at(25, pixels, 256, 40, changed, x, y, 256 * scale, 40 * scale);
}
