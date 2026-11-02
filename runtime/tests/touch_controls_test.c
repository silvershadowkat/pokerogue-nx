// SPDX-License-Identifier: AGPL-3.0-only
#include "touch_controls.h"
#include <assert.h>
#include <stdio.h>
static tc_button button(touch_controls *s,int id){for(int n=0;n<s->count;n++)if(s->layout[n].button==id)return s->layout[n];assert(0);return s->layout[0];}
int main(void){
 touch_controls s={0};tc_configure(&s,1280,720,true,true);tc_button a=button(&s,0),b=button(&s,1);
 assert(tc_pointer(&s,TC_DOWN,1,a.x+5,a.y+5));assert(s.buttons[0]==1);
 assert(tc_pointer(&s,TC_DOWN,2,b.x+5,b.y+5));assert(s.buttons[0]==1&&s.buttons[1]==1);
 assert(tc_pointer(&s,TC_UP,1,600,200));assert(!s.buttons[0]&&s.buttons[1]);
 assert(tc_pointer(&s,TC_UP,2,b.x,b.y));assert(!s.buttons[1]);
 assert(tc_pointer(&s,TC_DOWN,3,a.x+5,a.y+5));assert(tc_pointer(&s,TC_DOWN,4,a.x+5,a.y+5));
 tc_pointer(&s,TC_UP,3,a.x,a.y);assert(s.buttons[0]);tc_pointer(&s,TC_UP,4,a.x,a.y);assert(!s.buttons[0]);
 // A game gesture is not stolen when it drags over the pad.
 assert(!tc_pointer(&s,TC_DOWN,5,600,200));assert(!tc_pointer(&s,TC_MOVE,5,a.x+5,a.y+5));assert(!s.buttons[0]);
 // An action stays held while dragging out, matching the original captured button.
 assert(tc_pointer(&s,TC_DOWN,6,a.x+5,a.y+5));assert(tc_pointer(&s,TC_MOVE,6,600,200));assert(s.buttons[0]);assert(tc_pointer(&s,TC_UP,6,600,200));
 tc_configure(&s,1280,720,false,true);assert(tc_pointer(&s,TC_DOWN,7,600,200));
 tc_configure(&s,1280,720,true,false);assert(!tc_pointer(&s,TC_DOWN,8,a.x+5,a.y+5));assert(!s.buttons[0]);
 tc_configure(&s,1280,720,false,false);assert(tc_pointer(&s,TC_DOWN,8,600,200));
 tc_configure(&s,1280,720,true,true);tc_pointer(&s,TC_DOWN,10,a.x+5,a.y+5);tc_release(&s);assert(!s.buttons[0]);
 tc_configure(&s,1600,1200,true,true);for(int n=0;n<s.count;n++){tc_button c=s.layout[n];assert(c.x>=0&&c.y>=0&&c.x+c.w<=1600&&c.y+c.h<=1200);}
 // Context changes release hidden actions, preserve the D-pad and avoid stale holds.
 a=button(&s,0);tc_pointer(&s,TC_DOWN,20,a.x+a.w/2,a.y+a.h/2);assert(s.buttons[0]);
 tc_set_visible(&s,1u<<9);assert(!s.buttons[0]);tc_pointer(&s,TC_UP,20,a.x,a.y);
 float cx=s.dpad.x+s.dpad.w/2,cy=s.dpad.y+s.dpad.h/2;
 tc_pointer(&s,TC_DOWN,21,cx,cy);assert(s.dpad_button==-1);
 tc_pointer(&s,TC_MOVE,21,cx+s.dpad.w/3,cy);assert(s.buttons[15]);
 tc_set_visible(&s,0x3ff);assert(s.buttons[15]);
 unsigned old=s.action_revision;tc_pointer(&s,TC_MOVE,21,cx,cy-s.dpad.w/3);assert(s.buttons[12]&&!s.buttons[15]);assert(s.action_revision==old);
 tc_pointer(&s,TC_CANCEL,21,0,0);assert(!s.buttons[12]);
 puts("Touch control ownership, multitouch, context visibility, cached actions, cancellation and 4:3 layout PASS");return 0;
}
