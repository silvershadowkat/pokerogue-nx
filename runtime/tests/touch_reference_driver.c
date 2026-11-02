// SPDX-License-Identifier: AGPL-3.0-only
#include "touch_controls.h"
#include <stdio.h>
int main(void) {
    touch_controls s = {0};
    tc_configure(&s, 1280, 960, true, true);
    char op;
    while (scanf(" %c", &op) == 1) {
        if (op == 'g') {
            printf("%.9g %.9g %.9g %.9g\n", s.dpad.x, s.dpad.y, s.dpad.w,
                   s.dpad.h);
            fflush(stdout);
            continue;
        }
        if (op == 'v') {
            double x, y, w;
            int d;
            scanf("%lf%lf%lf%d", &x, &y, &w, &d);
            tc_pose p = tc_visual_pose(x, y, w, d);
            printf("%d", tc_resolve(x, y, w));
            const double a[] = {p.nx,       p.ny,       p.distance, p.tilt_x,
                                p.tilt_y,   p.shadow_x, p.shadow_y, p.depth,
                                p.light[0], p.light[1], p.light[2], p.light[3]};
            for (int i = 0; i < 12; i++)
                printf(" %.12g", a[i]);
            puts("");
            continue;
        }
        if (op == 'e') {
            int kind, id;
            float x, y;
            scanf("%d%d%f%f", &kind, &id, &x, &y);
            tc_pointer(&s, kind, id, x, y);
        }
        if (op == 'r')
            tc_release(&s);
        unsigned mask = 0;
        int owner = -1;
        for (int i = 0; i < 18; i++)
            if (s.buttons[i])
                mask |= 1u << i;
        for (int i = 0; i < 16; i++)
            if (s.fingers[i].active && s.fingers[i].button == -2)
                owner = s.fingers[i].id;
        printf("%u %d %d %d", mask, owner, s.dpad_button, s.transition_count);
        for (int i = 0; i < s.transition_count; i++)
            printf(" %d %d", s.transitions[i].button, s.transitions[i].down);
        puts("");
        s.transition_count = 0;
    }
    return 0;
}
