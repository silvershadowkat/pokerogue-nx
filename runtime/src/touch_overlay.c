// SPDX-License-Identifier: AGPL-3.0-only
// Code-drawn controls only: no game art, image models or external assets.
#include "host.h"
#include "touch_controls.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "plutovg.h"
touch_controls g_touch_controls;
static bool active;
void touch_overlay_active(bool enabled) {
    if(active&&!enabled)tc_release(&g_touch_controls);
    active=enabled;
}
bool touch_overlay_event(const plat_event *ev) {
    if(!active||ev->type<PEV_POINTER_DOWN||ev->type>PEV_POINTER_UP)return false;
    int w,h;plat_drawable_size(&w,&h);
    tc_configure(&g_touch_controls,w,h,g_cfg.direct_touch,g_cfg.touch_pad);
    int kind=ev->type==PEV_POINTER_DOWN?TC_DOWN:ev->type==PEV_POINTER_UP?TC_UP:TC_MOVE;
    return tc_pointer(&g_touch_controls,kind,ev->pointer_id,ev->x,ev->y);
}
void touch_overlay_merge(plat_gamepad *out,int *count,int max) {
    if(!active||!g_cfg.touch_pad||max<1)return;
    if(!*count){memset(out,0,sizeof *out);out->connected=true;out->id="PokeRogue NX Touch";*count=1;}
    for(int b=0;b<18;b++)if(g_touch_controls.buttons[b]>out->buttons[b])out->buttons[b]=g_touch_controls.buttons[b];
}
bool touch_overlay_visible(void){return active&&g_cfg.touch_pad;}
void touch_overlay_draw(void) {
    if(!touch_overlay_visible())return;
    int w,h;plat_drawable_size(&w,&h);tc_configure(&g_touch_controls,w,h,g_cfg.direct_touch,g_cfg.touch_pad);
    int iw=640,ih=(int)(640.0*h/w);static uint8_t *image;static int old_h;static unsigned old_revision=~0u;
    bool changed=old_h!=ih||old_revision!=g_touch_controls.revision;
    if(old_h!=ih){free(image);image=calloc((size_t)iw*ih,4);old_h=ih;}
    if(!image)return;
    if(changed){
        memset(image,0,(size_t)iw*ih*4);
        for(int n=0;n<g_touch_controls.count;n++){
            const tc_button *b=&g_touch_controls.layout[n];
            int x=(int)(b->x*iw/w),y=(int)(b->y*iw/w),bw=(int)(b->w*iw/w),bh=(int)(b->h*iw/w),radius=7;
            bool pressed=g_touch_controls.buttons[b->button]>0.5f;
            for(int yy=0;yy<bh;yy++)for(int xx=0;xx<bw;xx++){
                int dx=xx<radius?radius-xx:xx>=bw-radius?xx-(bw-radius-1):0;
                int dy=yy<radius?radius-yy:yy>=bh-radius?yy-(bh-radius-1):0;
                if(dx*dx+dy*dy>radius*radius)continue;
                int px=x+xx,py=y+yy;if(px<0||py<0||px>=iw||py>=ih)continue;
                uint8_t *p=image+((size_t)py*iw+px)*4;
                p[0]=pressed?75:22;p[1]=pressed?124:32;p[2]=pressed?160:47;p[3]=pressed?235:174;
                if(xx<2||yy<2||xx>=bw-2||yy>=bh-2){p[0]=143;p[1]=177;p[2]=201;p[3]=210;}
            }
        }
        // Reuse the downloaded game font; the controls have no artwork files.
        static plutovg_font_face_t *font;
        if(!font){char path[900];snprintf(path,sizeof path,"%s/launcher/emerald.ttf",g_cfg.app_dir);font=plutovg_font_face_load_from_file(path,0);}
        if(font){
            plutovg_convert_rgba_to_argb(image,image,iw,ih,iw*4);
            plutovg_surface_t *surface=plutovg_surface_create_for_data(image,iw,ih,iw*4);
            plutovg_canvas_t *c=plutovg_canvas_create(surface);plutovg_canvas_set_font(c,font,15);
            plutovg_canvas_set_rgba(c,.94f,.96f,.98f,1);
            for(int n=0;n<g_touch_controls.count;n++){
                const tc_button *b=&g_touch_controls.layout[n];
                float tw=plutovg_font_face_text_extents(font,15,b->label,-1,PLUTOVG_TEXT_ENCODING_UTF8,NULL);
                float x=b->x*iw/w+(b->w*iw/w-tw)/2,y=b->y*iw/w+(b->h*iw/w-15)/2+15;
                plutovg_canvas_fill_text(c,b->label,-1,PLUTOVG_TEXT_ENCODING_UTF8,x,y);
            }
            plutovg_canvas_destroy(c);plutovg_surface_destroy(surface);plutovg_convert_argb_to_rgba(image,image,iw,ih,iw*4);
        }else{
            const uint8_t fg[4]={238,244,250,255};
            for(int n=0;n<g_touch_controls.count;n++){const tc_button *b=&g_touch_controls.layout[n];overlay_blit_text(image,iw,ih,(int)(b->x*iw/w+(b->w*iw/w-strlen(b->label)*8)/2),(int)(b->y*iw/w+(b->h*iw/w-8)/2),b->label,1,fg);}
        }
        old_revision=g_touch_controls.revision;
    }
    overlay_rgba_at(15,image,iw,ih,changed,0,0,(float)w,(float)h);
}
