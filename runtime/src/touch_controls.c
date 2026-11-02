// SPDX-License-Identifier: AGPL-3.0-only
#include "touch_controls.h"
#include <string.h>
static void rebuild(touch_controls *s) {
    float next[18] = {0};
    for (int n=0;n<16;n++) if(s->fingers[n].active && s->fingers[n].button>=0)
        next[s->fingers[n].button]=1;
    if(memcmp(next,s->buttons,sizeof next)) { memcpy(s->buttons,next,sizeof next);s->revision++; }
}
void tc_release(touch_controls *s) {
    memset(s->fingers,0,sizeof s->fingers);rebuild(s);
}
static void add(touch_controls *s,float x,float y,float w,float h,int button,const char *label) {
    s->layout[s->count++]=(tc_button){x,y,w,h,button,label};
}
void tc_configure(touch_controls *s,int width,int height,bool direct,bool pad) {
    if(s->width==width&&s->height==height&&s->direct==direct&&s->pad==pad)return;
    tc_release(s);s->width=width;s->height=height;s->direct=direct;s->pad=pad;s->count=0;s->revision++;
    float scale=width/1280.0f;if(height/720.0f<scale)scale=height/720.0f;
    float r=52*scale,gap=8*scale,left=28*scale,bottom=height-24*scale;
    add(s,left+r+gap,bottom-3*r-2*gap,r,r,12,"^");
    add(s,left+r+gap,bottom-r,r,r,13,"v");
    add(s,left,bottom-2*r-gap,r,r,14,"<");
    add(s,left+2*r+2*gap,bottom-2*r-gap,r,r,15,">");
    float right=width-28*scale;
    add(s,right-r,bottom-2*r-gap,r,r,0,"A");
    add(s,right-2*r-gap,bottom-r,r,r,1,"B");
    add(s,right-2*r-gap,bottom-3*r-2*gap,r,r,2,"X");
    add(s,right-3*r-2*gap,bottom-2*r-gap,r,r,3,"Y");
    // Keep shoulders away from the bottom dialogue/attack area. On 4:3
    // devices these sit in the upper letterbox instead of covering game text.
    float top=24*scale;
    add(s,left,top,80*scale,36*scale,4,"L");
    add(s,left+92*scale,top,80*scale,36*scale,6,"ZL");
    add(s,right-80*scale,top,80*scale,36*scale,5,"R");
    add(s,right-172*scale,top,80*scale,36*scale,7,"ZR");
    add(s,width/2-156*scale,bottom-40*scale,96*scale,40*scale,8,"INFO");
    add(s,width/2-48*scale,bottom-40*scale,96*scale,40*scale,9,"MENU");
    add(s,width/2+60*scale,bottom-40*scale,96*scale,40*scale,17,"START");
}
static int hit(touch_controls *s,float x,float y) {
    for(int n=0;n<s->count;n++){tc_button *b=&s->layout[n];if(x>=b->x&&x<b->x+b->w&&y>=b->y&&y<b->y+b->h)return b->button;}
    return -1;
}
bool tc_pointer(touch_controls *s,int kind,int id,float x,float y) {
    tc_finger *finger=NULL;
    for(int n=0;n<16;n++)if(s->fingers[n].active&&s->fingers[n].id==id){finger=&s->fingers[n];break;}
    if(finger){
        if(kind==TC_UP)finger->active=false;
        else finger->button=hit(s,x,y);
        rebuild(s);return true;
    }
    if(kind!=TC_DOWN)return !s->direct;
    int button=s->pad?hit(s,x,y):-1;
    if(button<0)return !s->direct;
    for(int n=0;n<16;n++)if(!s->fingers[n].active){s->fingers[n]=(tc_finger){id,button,true};rebuild(s);return true;}
    return true;
}
