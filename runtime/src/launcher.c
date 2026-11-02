// SPDX-License-Identifier: AGPL-3.0-only
// Mode selection before JS/auth initialization. Only official downloaded artwork
// and the official emerald font are used; no game files ship inside the app.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "gles.h"
#include "host.h"
#include "storage_file.h"
#include "plutovg.h"
#include "stb_image.h"

#define W 960
#define H 540
#define PANEL_X 315
#define PANEL_Y 210
#define PANEL_W 330
#define ROW_H 45
#define TOG_Y 438
#define TOG_W 300
#define TOG_H 30

enum { M_ONLINE, M_OFFLINE, M_DEBUG, M_UPDATE, M_BENCH };
typedef struct { const char *label, *key; bool *value; } toggle;
static const toggle toggles[] = {
    {"Tap menus", "directtouch", &g_cfg.direct_touch},
    {"Touch pad", "touchpad", &g_cfg.touch_pad},
    {"30 fps limit", "fps30", &g_cfg.fps30},
};
#define N_TOGGLES ((int)(sizeof toggles / sizeof toggles[0]))
typedef struct { const char *remote, *file; uint8_t *pixels; int w,h; } artwork;
static artwork art[] = {
    {"images/arenas/plains_bg.png", "plains_bg.png", NULL,0,0},
    {"images/logo.png", "logo.png", NULL,0,0},
    {"images/ui/windows/window_1.png", "window_1.png", NULL,0,0},
    {"images/ui/cursor.png", "cursor.png", NULL,0,0},
};
static uint8_t image[W*H*4];
static plutovg_font_face_t *font;
static plutovg_canvas_t *canvas;
static bool theme_loaded;
static int panel_y(int n){return n>3?180:PANEL_Y;}

static char *read_asset(const char *path,size_t *length) {
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    char *data=n>0&&n<=8*1024*1024?malloc((size_t)n):NULL;
    if(data&&fread(data,1,(size_t)n,f)!=(size_t)n){free(data);data=NULL;}
    fclose(f);if(data)*length=(size_t)n;return data;
}
static bool cached_asset(const char *dir,const char *remote,const char *file,bool *network_ok) {
    char path[900];snprintf(path,sizeof path,"%s/%s",dir,file);
    struct stat st;if(stat(path,&st)==0&&st.st_size>0)return true;
    if(!*network_ok)return false;
    char url[800];snprintf(url,sizeof url,"https://pokerogue.net/%s",remote);
    size_t n=0;long status=0;
    char *data=net_get_threadsafe(url,&n,&status);
    bool ok=data&&status==200&&n>0&&n<=8*1024*1024&&storage_write_file(path,data,n);
    free(data);
    if(!ok){*network_ok=false;host_log("[start] official menu asset unavailable (HTTP %ld)",status);}
    return ok;
}
static void load_theme(void) {
    if(theme_loaded)return;
    theme_loaded=true;
    char dir[800],path[900];snprintf(dir,sizeof dir,"%s/launcher",g_cfg.app_dir);
    mkdir(dir,0777);bool network_ok=true;
    // The font is cached first so even a partial first download gives a readable menu.
    snprintf(path,sizeof path,"%s/emerald.ttf",dir);
    if(cached_asset(dir,"fonts/pokemon-emerald-pro.ttf","emerald.ttf",&network_ok))
        font=plutovg_font_face_load_from_file(path,0);
    for(int i=0;i<4;i++)if(cached_asset(dir,art[i].remote,art[i].file,&network_ok)){
        snprintf(path,sizeof path,"%s/%s",dir,art[i].file);size_t n=0;char *data=read_asset(path,&n);
        if(data){int comp;art[i].pixels=stbi_load_from_memory((uint8_t*)data,(int)n,&art[i].w,&art[i].h,&comp,4);free(data);}
    }
}
static void blit(const artwork *a,int sx,int sy,int sw,int sh,int x,int y,int w,int h) {
    if(!a->pixels||w<=0||h<=0)return;
    for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){
        int px=x+xx,py=y+yy;if(px<0||py<0||px>=W||py>=H)continue;
        const uint8_t *s=a->pixels+((sy+yy*sh/h)*a->w+sx+xx*sw/w)*4;
        uint8_t *d=image+(py*W+px)*4;int alpha=s[3];
        for(int k=0;k<3;k++)d[k]=(uint8_t)((s[k]*alpha+d[k]*(255-alpha)+127)/255);
        d[3]=255;
    }
}
static void picture(int i,int x,int y,int w,int h){blit(&art[i],0,0,art[i].w,art[i].h,x,y,w,h);}
static void window_box(int x,int y,int w,int h){
    const artwork *a=&art[2];if(!a->pixels)return;
    // Same eight-pixel nine-slice borders as upstream's addWindow, scaled 3x.
    for(int j=0;j<3;j++)for(int i=0;i<3;i++){
        int dx=x+(i==0?0:i==1?24:w-24),dy=y+(j==0?0:j==1?24:h-24);
        blit(a,i*8,j*8,8,8,dx,dy,i==1?w-48:24,j==1?h-48:24);
    }
}
static void text(int x,int y,const char *s,int size,bool dim,bool selected){
    if(!font){const uint8_t c[4]={255,255,255,255};overlay_blit_text(image,W,H,x,y,s,size>=30?2:1,c);return;}
    plutovg_canvas_set_font(canvas,font,(float)size);
    plutovg_canvas_set_rgba(canvas,0.20f,0.16f,0.23f,1);
    plutovg_canvas_fill_text(canvas,s,-1,PLUTOVG_TEXT_ENCODING_UTF8,x+2,y+size+2);
    if(selected)plutovg_canvas_set_rgba(canvas,1,0.86f,0.45f,1);
    else if(dim)plutovg_canvas_set_rgba(canvas,0.65f,0.65f,0.70f,1);
    else plutovg_canvas_set_rgba(canvas,0.97f,0.97f,0.97f,1);
    plutovg_canvas_fill_text(canvas,s,-1,PLUTOVG_TEXT_ENCODING_UTF8,x,y+size);
}
static void centered(int y,const char *s,int size,bool dim){
    float width=font?plutovg_font_face_text_extents(font,(float)size,s,-1,PLUTOVG_TEXT_ENCODING_UTF8,NULL):(float)strlen(s)*8;
    text((int)((W-width)/2),y,s,size,dim,false);
}
static void render(int sel,const int *modes,int n,bool offline_ok,const char *version,const char *update){
    memset(image,0,sizeof image);for(int i=0;i<W*H;i++)image[i*4+3]=255;
    picture(0,0,0,W,H);picture(1,255,54,450,99);
    int top=panel_y(n);
    window_box(PANEL_X,top,PANEL_W,24+ROW_H*n);
    window_box(22,TOG_Y-6,W-44,74);
    if(sel<n)picture(3,PANEL_X+22,top+25+ROW_H*sel,18,30);
    else picture(3,30+(sel-n)*TOG_W,TOG_Y+9,12,20);
    plutovg_convert_rgba_to_argb(image,image,W,H,W*4);
    plutovg_surface_t *surface=plutovg_surface_create_for_data(image,W,H,W*4);
    canvas=plutovg_canvas_create(surface);
    for(int i=0;i<n;i++){
        const char *name=modes[i]==M_ONLINE?"Online":modes[i]==M_OFFLINE?"Offline":modes[i]==M_DEBUG?"Debug":modes[i]==M_UPDATE?"Update offline game":"Shader benchmark";
        text(PANEL_X+56,top+15+i*ROW_H,name,36,modes[i]==M_DEBUG,sel==i);
    }
    char message[150];int current=sel<n?modes[sel]:-1;
    if(current==M_ONLINE)snprintf(message,sizeof message,"Official account, cloud saves and Daily Run");
    else if(current==M_OFFLINE)snprintf(message,sizeof message,offline_ok?"Installed version %s - saves stay on this device":"Download the offline game (about 530 MB)",version);
    else if(current==M_DEBUG)snprintf(message,sizeof message,"Cheats and custom modes - coming in a later update");
    else if(current==M_UPDATE)snprintf(message,sizeof message,"Version %s available - choose to download",update);
    else snprintf(message,sizeof message,"Touch, physical controls, or both");
    centered(TOG_Y-51,message,24,false);
    for(int i=0;i<N_TOGGLES;i++){
        const char *value=*toggles[i].value?"On":"Off";
        if(i==1&&g_cfg.touch_pad)value=g_cfg.touch_pad_always?"Always":"Fade";
        char label[80];snprintf(label,sizeof label,"%s: %s",toggles[i].label,value);
        text(52+i*TOG_W,TOG_Y,label,24,false,sel==n+i);
    }
    centered(TOG_Y+35,"PokeRogue NX 0.2.0-rc7",18,true);
    plutovg_canvas_destroy(canvas);canvas=NULL;plutovg_surface_destroy(surface);
    plutovg_convert_argb_to_rgba(image,image,W,H,W*4);
}
static int hit(int n,float x,float y){
    int top=panel_y(n);
    if(x>=PANEL_X&&x<PANEL_X+PANEL_W&&y>=top+12&&y<top+12+n*ROW_H)return (int)(y-top-12)/ROW_H;
    if(y>=TOG_Y&&y<TOG_Y+TOG_H&&x>=30&&x<W-30)return n+(int)(x-30)/TOG_W;
    return -1;
}
static int list_modes(int *modes,const char *update,bool bench){
    int n=0;modes[n++]=M_ONLINE;modes[n++]=M_OFFLINE;modes[n++]=M_DEBUG;
    if(update)modes[n++]=M_UPDATE;
    if(bench)modes[n++]=M_BENCH;
    return n;
}
static int move(int sel,int n,int dy,int dx){
    if(dx&&sel>=n)return n+(sel-n+(dx>0?1:N_TOGGLES-1))%N_TOGGLES;
    if(dy<0)return sel==0?n+N_TOGGLES-1:sel>=n?n-1:sel-1;
    if(dy>0)return sel<n-1?sel+1:sel<n?n:0;
    return sel;
}
int launcher_run(bool offline_ok,const char *version,bool bench,void (*poll)(bool *quit)){
    load_theme();
    if(offline_ok)offline_update_check_start();
    const char *update=NULL;int modes[5],n=list_modes(modes,update,bench),sel=0;
    char path[800],saved[32]="";snprintf(path,sizeof path,"%s/launcher.txt",g_cfg.app_dir);
    FILE *f=fopen(path,"r");if(f){fgets(saved,sizeof saved,f);fclose(f);if(!strncmp(saved,"offline",7))sel=1;}
    bool prev[18]={0},prev_dir[4]={0},dirty=true,chosen=false,quit=false,armed=false;
    while(!chosen&&!quit){
        poll(&quit);
        if(offline_ok&&!update&&(update=offline_update_available(version))){int old=n;n=list_modes(modes,update,bench);if(sel>=old)sel+=n-old;dirty=true;}
        int sw,sh;plat_drawable_size(&sw,&sh);
        float scale=(float)sw/W;if((float)sh/H<scale)scale=(float)sh/H;
        float ox=(sw-W*scale)/2,oy=(sh-H*scale)/2;
        int pick=-1;plat_event ev;
        while(plat_poll_event(&ev)){
            if(ev.type==PEV_QUIT)quit=true;
            else if(ev.type==PEV_KEY_DOWN&&ev.key){
                if(!strcmp(ev.key,"ArrowUp"))sel=move(sel,n,-1,0),dirty=true;
                else if(!strcmp(ev.key,"ArrowDown"))sel=move(sel,n,1,0),dirty=true;
                else if(!strcmp(ev.key,"ArrowLeft"))sel=move(sel,n,0,-1),dirty=true;
                else if(!strcmp(ev.key,"ArrowRight"))sel=move(sel,n,0,1),dirty=true;
                else if(!strcmp(ev.key,"Enter")||!strcmp(ev.key," "))pick=sel;
            }else if(ev.type==PEV_POINTER_DOWN){int b=hit(n,(ev.x-ox)/scale,(ev.y-oy)/scale);if(b>=0&&b<n+N_TOGGLES)sel=pick=b,dirty=true;}
        }
        plat_gamepad pad;
        if(plat_gamepads(&pad,1)>0){
            bool now[18],dir[4]={pad.buttons[12]>.5f||pad.axes[1]<-.6f,pad.buttons[13]>.5f||pad.axes[1]>.6f,pad.buttons[14]>.5f||pad.axes[0]<-.6f,pad.buttons[15]>.5f||pad.axes[0]>.6f};
            bool any=false;for(int i=0;i<18;i++)now[i]=pad.buttons[i]>.5f,any|=now[i];for(int i=0;i<4;i++)any|=dir[i];
            if(!armed)armed=!any;
            else{
                if(dir[0]&&!prev_dir[0])sel=move(sel,n,-1,0),dirty=true;
                if(dir[1]&&!prev_dir[1])sel=move(sel,n,1,0),dirty=true;
                if(dir[2]&&!prev_dir[2])sel=move(sel,n,0,-1),dirty=true;
                if(dir[3]&&!prev_dir[3])sel=move(sel,n,0,1),dirty=true;
                if((now[0]&&!prev[0])||(now[9]&&!prev[9]))pick=sel;
            }
            memcpy(prev,now,sizeof prev);memcpy(prev_dir,dir,sizeof prev_dir);
        }
        if(pick>=n){
            int i=pick-n;
            if(i==1){int next=g_cfg.touch_pad?(g_cfg.touch_pad_always?0:2):1;g_cfg.touch_pad=next!=0;g_cfg.touch_pad_always=next==2;config_set("touchpad",next==2?"2":next==1?"1":"0");}
            else{*toggles[i].value=!*toggles[i].value;config_set(toggles[i].key,*toggles[i].value?"1":"0");}
            dirty=true;
        }
        else if(pick>=0&&modes[pick]!=M_DEBUG)chosen=true;
        glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,sw,sh);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
        if(dirty)render(sel,modes,n,offline_ok,version,update);
        overlay_rgba_at(14,image,W,H,dirty,ox,oy,W*scale,H*scale);dirty=false;
        host_frame_drawn();plat_swap();
#ifndef __vita__
        struct timespec ts={0,16*1000000L};nanosleep(&ts,NULL);
#endif
    }
    if(quit)return -1;
    int mode=modes[sel];
    if(mode==M_UPDATE)return 3;
    if(mode==M_OFFLINE&&!offline_ok)return 2;
    f=fopen(path,"w");if(f){fputs(mode==M_OFFLINE?"offline\n":"online\n",f);fclose(f);}
    return mode==M_BENCH?4:mode==M_OFFLINE?1:0;
}
