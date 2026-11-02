// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "plutovg.h"
#include <stdbool.h>
#include <stdint.h>
enum {
    TA_SOCKET,
    TA_SHADOW,
    TA_FACE,
    TA_UP,
    TA_RIGHT,
    TA_DOWN,
    TA_LEFT,
    TA_LAYERS
};
uint8_t *touch_art_layer(int layer, int size);
void touch_art_button(plutovg_canvas_t *c, float x, float y, float w, float h,
                      bool circle, bool pressed);
