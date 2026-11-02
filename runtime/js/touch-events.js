// SPDX-License-Identifier: AGPL-3.0-only
// The original touch event contract/repeat rate, independent of physical bindings.
(function(g){
  'use strict';
  const rt=g.__rt,N=g.__native,held=new Map();
  const buttons=[5,6,10,11,9,12,13,14,8,7,-1,-1,0,1,2,3,-1,4];
  let scene=null,lastMask=-1;
  function emit(button,down){
    const game=rt.renderer?.game;
    if(!game?.events)return false;
    game.events.emit(down?'input_down':'input_up',{controller_type:'keyboard',button,isTouch:true});
    return true;
  }
  rt.onTouchControl=(raw,down)=>{
    const button=buttons[raw];if(button==null||button<0)return;
    if(down){
      if(held.has(raw)||!emit(button,true))return;
      held.set(raw,g.setInterval(()=>emit(button,true),250));
    }else if(held.has(raw)){
      g.clearInterval(held.get(raw));held.delete(raw);emit(button,false);
    }
  };
  function reset(){for(const [raw,timer] of held){g.clearInterval(timer);emit(buttons[raw],false);}held.clear();}
  rt.frameHooks.push(()=>{
    const next=rt.renderer?.game?.scene.getScene('battle');
    if(next!==scene){reset();scene=next;N.resetTouchControls?.();lastMask=-1;}
    const mode=scene?.ui?.mode;
    let mask=(1<<0)|(1<<1)|(1<<9); // A, B, Start
    if(mode===10)mask=0x3ff;
    else{
      if((mode>=18&&mode<=24)||mode===37)mask|=(1<<2)|(1<<4);
      if([29,31,41].includes(mode))mask|=(1<<2)|(1<<4);
      if(mode===31)mask|=(1<<3)|(1<<5);
      if(mode===41)mask|=1<<5;
      if([2,3,4,5,29,31].includes(mode))mask|=1<<7;
      if([2,3,4,5,6,29,31].includes(mode))mask|=1<<8;
    }
    if(mask!==lastMask){lastMask=mask;N.setTouchControlsMask?.(mask);}
  });
})(globalThis);
