// SPDX-License-Identifier: AGPL-3.0-only
// Native overlay integration under headless ES2; no account or game session.
#include "host.h"
#include "touch_art.h"
#include "touch_controls.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern touch_controls g_touch_controls;
host_config g_cfg = {
    .app_dir = "work/egl-test", .direct_touch = true, .touch_pad = true};
static double now = 1000;
static int uploads, event_count, events[32][2];
double host_now_ms(void) { return now; }
void host_log(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
const char *host_version_string(void) { return "test"; }
void plat_drawable_size(int *w, int *h) {
    *w = 1280;
    *h = 960;
}
void plat_swap(void) {}
void JS_FreeValue(JSContext *ctx, JSValue value) {
    (void)ctx;
    (void)value;
}
void host_call_rt(JSContext *ctx, const char *name, int argc,
                  JSValueConst *argv) {
    (void)ctx;
    assert(!strcmp(name, "onTouchControl") && argc == 2);
    assert(event_count < 32);
    events[event_count][0] = JS_VALUE_GET_INT(argv[0]);
    events[event_count++][1] = JS_VALUE_GET_INT(argv[1]);
}
void __real_glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,
                         GLenum, const void *);
void __wrap_glTexImage2D(GLenum t, GLint l, GLint in, GLsizei w, GLsizei h,
                         GLint b, GLenum f, GLenum ty, const void *p) {
    uploads++;
    __real_glTexImage2D(t, l, in, w, h, b, f, ty, p);
}
static bool pointer(plat_event_type type, int id, float x, float y) {
    plat_event e = {.type = type, .pointer_id = id, .x = x, .y = y};
    return touch_overlay_event(&e);
}
static void draw(void) {
    glClearColor(.43f, .60f, .40f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    touch_overlay_draw();
    assert(glGetError() == GL_NO_ERROR);
}
static void png(void) {
    unsigned char *pixels = malloc(1280 * 960 * 4),
                  *flipped = malloc(1280 * 960 * 4);
    assert(pixels && flipped);
    glReadPixels(0, 0, 1280, 960, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (int y = 0; y < 960; y++)
        memcpy(flipped + y * 1280 * 4, pixels + (959 - y) * 1280 * 4, 1280 * 4);
    plutovg_convert_rgba_to_argb(flipped, flipped, 1280, 960, 1280 * 4);
    plutovg_surface_t *s =
        plutovg_surface_create_for_data(flipped, 1280, 960, 1280 * 4);
    assert(plutovg_surface_write_to_png(
        s, "work/screens/silvershadow-native-overlay.png"));
    plutovg_surface_destroy(s);
    free(pixels);
    free(flipped);
}
int main(void) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(display, NULL, NULL));
    const EGLint attr[] = {EGL_SURFACE_TYPE,
                           EGL_PBUFFER_BIT,
                           EGL_RENDERABLE_TYPE,
                           EGL_OPENGL_ES2_BIT,
                           EGL_RED_SIZE,
                           8,
                           EGL_GREEN_SIZE,
                           8,
                           EGL_BLUE_SIZE,
                           8,
                           EGL_ALPHA_SIZE,
                           8,
                           EGL_NONE};
    EGLConfig cfg;
    EGLint count;
    assert(eglChooseConfig(display, attr, &cfg, 1, &count) && count);
    const EGLint size[] = {EGL_WIDTH, 1280, EGL_HEIGHT, 960, EGL_NONE},
                 client[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display, cfg, size);
    EGLContext ctx = eglCreateContext(display, cfg, EGL_NO_CONTEXT, client);
    assert(eglMakeCurrent(display, surface, surface, ctx));
    touch_overlay_active(true);
    draw();
    assert(uploads == 10);
    uploads = 0;
    float cx = g_touch_controls.dpad.x + g_touch_controls.dpad.w / 2,
          cy = g_touch_controls.dpad.y + g_touch_controls.dpad.h / 2;
    assert(pointer(PEV_POINTER_DOWN, 1, cx, cy));
    draw();
    assert(uploads == 0);
    assert(g_touch_controls.dpad_button == -1);
    assert(pointer(PEV_POINTER_MOVE, 1, cx + 130, cy - 80));
    now = 1100;
    draw();
    now = 1140;
    draw();
    assert(uploads == 0);
    assert(g_touch_controls.buttons[15]);
    touch_overlay_flush(NULL);
    assert(event_count == 1 && events[0][0] == 15 && events[0][1]);
    assert(pointer(PEV_POINTER_MOVE, 1, cx, cy - 130));
    touch_overlay_flush(NULL);
    assert(event_count == 3 && events[1][0] == 15 && !events[1][1] &&
           events[2][0] == 12 && events[2][1]);
    now = 1160;
    draw();
    now = 1200;
    draw();
    assert(uploads == 0);
    tc_button a = g_touch_controls.layout[0];
    assert(pointer(PEV_POINTER_DOWN, 2, a.x + a.w / 2, a.y + a.h / 2));
    draw();
    assert(uploads == 2);
    pointer(PEV_POINTER_MOVE, 1, cx + 130, cy - 80);
    now = 1240;
    draw();
    now = 1280;
    draw();
    assert(uploads == 2);
    png();
    assert(pointer(PEV_POINTER_MOVE, 2, 600, 100));
    assert(g_touch_controls.buttons[0]);
    assert(pointer(PEV_POINTER_UP, 2, 600, 100));
    assert(!g_touch_controls.buttons[0]);
    assert(pointer(PEV_POINTER_UP, 1, cx, cy));
    touch_overlay_flush(NULL);
    touch_overlay_active(false);
    touch_overlay_active(true);
    draw();
    assert(!pointer(PEV_POINTER_DOWN, 3, 600, 200));
    now = 5000;
    draw();
    assert(touch_overlay_visible());
    assert(!pointer(PEV_POINTER_UP, 3, 600, 200));
    now = 7200;
    draw();
    now = 7400;
    draw();
    assert(!touch_overlay_visible());
    assert(pointer(PEV_POINTER_DOWN, 4, cx, cy));
    assert(touch_overlay_visible());
    assert(g_touch_controls.dpad_button == -1);
    pointer(PEV_POINTER_MOVE, 4, cx + 130, cy);
    assert(g_touch_controls.buttons[15]);
    touch_overlay_active(false);
    assert(!g_touch_controls.buttons[15]);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, ctx);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    puts("PASS: native ES2 controller, neutral-to-slide, ordered events, zero "
         "texture uploads during D-pad movement, action capture, fade/wake and "
         "lifecycle release");
    return 0;
}
