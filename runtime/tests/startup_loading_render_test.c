// SPDX-License-Identifier: AGPL-3.0-only
#include "host.h"
#include "plutovg.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

host_config g_cfg = {.app_dir = "work/egl-test"};
static int width = 1280, height = 720, uploads;
static double now = 1000;
double host_now_ms(void) { return now; }
void host_log(const char *fmt, ...) { (void)fmt; }
const char *host_version_string(void) { return "test"; }
void plat_drawable_size(int *w, int *h) {
    *w = width;
    *h = height;
}
void plat_swap(void) {}
void __real_glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,
                         const void *);
void __wrap_glTexImage2D(GLenum t, GLint l, GLint in, GLsizei w, GLsizei h, GLint b, GLenum f,
                         GLenum ty, const void *p) {
    uploads++;
    __real_glTexImage2D(t, l, in, w, h, b, f, ty, p);
}
static int check_bounds(bool empty) {
    unsigned char *p = malloc((size_t)width * height * 4);
    assert(p);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, p);
    double scale = fmin(width / 960., height / 540.);
    double x = (width - 960 * scale) / 2 + 12 * scale, y = (height - 540 * scale) / 2 + 488 * scale;
    int count = 0;
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++) {
            const unsigned char *v = p + ((size_t)(height - 1 - row) * width + col) * 4;
            if (v[0] != 0 || v[1] != 0 || v[2] != 255) {
                count++;
                assert(!empty);
                assert(col >= floor(x) && col < ceil(x + 256 * scale));
                assert(row >= floor(y) && row < ceil(y + 40 * scale));
            }
        }
    free(p);
    return count;
}
static void save_preview(void) {
    size_t bytes = (size_t)width * height * 4;
    unsigned char *p = malloc(bytes), *flipped = malloc(bytes);
    assert(p && flipped);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, p);
    for (int y = 0; y < height; y++)
        memcpy(flipped + (size_t)y * width * 4, p + (size_t)(height - 1 - y) * width * 4,
               (size_t)width * 4);
    plutovg_convert_rgba_to_argb(flipped, flipped, width, height, width * 4);
    plutovg_surface_t *s = plutovg_surface_create_for_data(flipped, width, height, width * 4);
    char name[150];
    snprintf(name, sizeof name, "work/screens/startup-loading-%dx%d.png", width, height);
    assert(plutovg_surface_write_to_png(s, name));
    plutovg_surface_destroy(s);
    free(p);
    free(flipped);
}
int main(void) {
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(d, NULL, NULL));
    EGLint attrs[] = {EGL_SURFACE_TYPE,
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
    assert(eglChooseConfig(d, attrs, &cfg, 1, &n) && n);
    EGLint sa[] = {EGL_WIDTH, 2400, EGL_HEIGHT, 1080, EGL_NONE},
           ca[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLSurface s = eglCreatePbufferSurface(d, cfg, sa);
    EGLContext c = eglCreateContext(d, cfg, EGL_NO_CONTEXT, ca);
    assert(eglMakeCurrent(d, s, s, c));
    const int shapes[][2] = {{1280, 720}, {1280, 960}, {2400, 1080}};
    for (int i = 0; i < 3; i++) {
        width = shapes[i][0];
        height = shapes[i][1];
        startup_loading_begin();
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        int before = uploads;
        startup_loading_draw(false);
        assert(uploads == before + 1);
        assert(check_bounds(false) > 100);
        now += 16;
        startup_loading_draw(false);
        assert(uploads == before + 1);
        now += 350;
        startup_loading_draw(false);
        assert(uploads == before + 2);
        assert(check_bounds(false) > 100 && glGetError() == GL_NO_ERROR);
        startup_loading_draw(true);
        save_preview();
        startup_loading_ready();
        assert(!startup_loading_active());
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        startup_loading_draw(false);
        assert(check_bounds(true) == 0 && uploads == before + 2);
    }
    startup_loading_begin();
    startup_loading_draw(true);
    unsigned char p[4];
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    assert(p[0] == 0 && p[1] == 0 && p[2] == 0);
    eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(d, c);
    eglDestroySurface(d, s);
    eglTerminate(d);
    puts("PASS: native loading label visible inside 16:9/4:3/wide-phone game viewports, cached "
         "between animation ticks, transparent over game and removed when ready");
}
