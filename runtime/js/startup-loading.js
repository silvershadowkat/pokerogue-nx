// SPDX-License-Identifier: AGPL-3.0-only
// Observe the first rendered game UI; do not alter scenes, input or gameplay.
(function(g){
  'use strict';
  const rt=g.__rt,N=g.__native;
  let watched=null,finished=false;
  function rendered(){
    if(finished)return;
    try{
      const battle=watched?.scene?.getScene('battle'),ui=battle?.ui;
      if(!ui||ui.mode===35||ui.visible===false||!battle.sys?.isActive?.())return;
      if(!ui.getHandler?.()?.active)return;
      finished=true;
      watched.events.off('postrender',rendered);
      const index=rt.frameHooks.indexOf(observe);
      if(index>=0)rt.frameHooks.splice(index,1);
      N.startupReady();
    }catch(_){/* A partially constructed scene is not a ready game screen. */}
  }
  function observe(){
    const game=rt.renderer?.game;
    if(finished||game===watched||!game?.events?.on)return;
    watched?.events?.off?.('postrender',rendered);
    watched=game;
    game.events.on('postrender',rendered);
  }
  rt.frameHooks.push(observe);
})(globalThis);
