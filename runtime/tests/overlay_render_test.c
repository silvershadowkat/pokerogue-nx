// SPDX-License-Identifier: AGPL-3.0-only
// Actual ES2 shader/texture/opacity/legacy-overlay regression, headless EGL.
#include "host.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
void host_log(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
const char *host_version_string(void) { return "test"; }
void plat_drawable_size(int *w, int *h) {
    *w = 320;
    *h = 180;
}
void plat_swap(void) {}
static void expect(int x, int y, int r, int g, int b) {
    unsigned char p[4];
    glReadPixels(x, 179 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    assert(abs((int)p[0] - r) <= 2 && abs((int)p[1] - g) <= 2 &&
           abs((int)p[2] - b) <= 2);
}
int main(void) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(display, NULL, NULL));
    const EGLint config[] = {EGL_SURFACE_TYPE,
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
    EGLint n;
    assert(eglChooseConfig(display, config, &cfg, 1, &n) && n);
    const EGLint attributes[] = {EGL_WIDTH, 320, EGL_HEIGHT, 180, EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display, cfg, attributes);
    const EGLint client[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(display, cfg, EGL_NO_CONTEXT, client);
    assert(eglMakeCurrent(display, surface, surface, context));
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    const unsigned char rgba[] = {255, 0, 0,   255, 0,   255, 0,   255,
                                  0,   0, 255, 255, 255, 255, 255, 255};
    float p[] = {20, 20, 1, 120, 20, 1, 20, 120, 1, 120, 120, 1};
    overlay_rgba_quad(16, rgba, 2, 2, true, p, .5f);
    expect(30, 30, 128, 0, 0);
    expect(110, 30, 0, 128, 0);
    expect(30, 110, 0, 0, 128);
    overlay_rgba_at(17, rgba, 2, 2, true, 140, 20, 100, 100);
    expect(150, 30, 255, 0, 0);
    expect(230, 30, 0, 255, 0);
    expect(150, 110, 0, 0, 255);
    // Different homogeneous weights must preserve orientation/visible geometry.
    const float tilted[] = {20, 130, .8f, 120, 125, 1.2f,
                            25, 170, .9f, 115, 175, 1.1f};
    overlay_rgba_quad(18, rgba, 2, 2, true, tilted, 1);
    expect(35, 140, 255, 0, 0);
    assert(overlay_group_begin(64));
    const unsigned char white[] = {255, 255, 255, 255};
    overlay_rgba_at(19, white, 1, 1, true, 0, 0, 64, 64);
    overlay_rgba_at(19, white, 1, 1, false, 0, 0, 64, 64);
    overlay_group_end();
    overlay_group_draw(250, 125, 50, 50, .5f);
    expect(270, 150, 128, 128, 128);
    unsigned char red[16] = {255, 0, 0, 255, 255, 0, 0, 255,
                             255, 0, 0, 255, 255, 0, 0, 255};
    int ox, oy;
    overlay_image_draw(red, 2, 2, true, &ox, &oy);
    expect(160, 90, 255, 0, 0);
    assert(glGetError() == GL_NO_ERROR);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    puts("PASS: ES2 shader, alpha, cached texture orientation, culling, "
         "perspective weights and legacy keyboard overlay");
    return 0;
}
