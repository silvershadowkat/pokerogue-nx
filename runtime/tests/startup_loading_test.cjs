// SPDX-License-Identifier: AGPL-3.0-only
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const {EventEmitter}=require('node:events');
const source=fs.readFileSync('runtime/js/startup-loading.js','utf8');
let calls=0,battle;
const rt={frameHooks:[]};
const N={startupReady(){calls++;}};
vm.runInNewContext(source,{__rt:rt,__native:N});
function frame(){for(const hook of [...rt.frameHooks])hook();}
function game(){return {events:new EventEmitter(),scene:{getScene:()=>battle}};}
frame();assert.equal(calls,0);
const first=game();rt.renderer={game:first};frame();
assert.equal(first.events.listenerCount('postrender'),1);
first.events.emit('postrender');assert.equal(calls,0);
battle={sys:{isActive:()=>false},ui:{mode:1,visible:true,getHandler:()=>({active:true})}};
first.events.emit('postrender');assert.equal(calls,0);
battle.sys.isActive=()=>true;battle.ui.mode=35;
first.events.emit('postrender');assert.equal(calls,0);
battle.ui.mode=1;battle.ui.visible=false;
first.events.emit('postrender');assert.equal(calls,0);
battle.ui.visible=true;battle.ui.getHandler=()=>({active:false});
first.events.emit('postrender');assert.equal(calls,0);
battle.ui.mode=0;battle.ui.getHandler=()=>({active:true,message:{text:'',visible:true}});
first.events.emit('postrender');assert.equal(calls,0,'An empty MESSAGE handler is still a blank startup frame');
battle.ui.mode=1;
battle.ui.getHandler=()=>{throw Error('UI still constructing');};
first.events.emit('postrender');assert.equal(calls,0);
const second=game();rt.renderer.game=second;frame();
assert.equal(first.events.listenerCount('postrender'),0);
assert.equal(second.events.listenerCount('postrender'),1);
battle.ui.getHandler=()=>({active:true});
frame();assert.equal(calls,0,'A ready handler must actually be rendered first');
second.events.emit('postrender');assert.equal(calls,1);
assert.equal(second.events.listenerCount('postrender'),0);
assert.equal(rt.frameHooks.length,0);
second.events.emit('postrender');frame();assert.equal(calls,1);
let welcomeCalls=0;
const welcomeRT={frameHooks:[]},welcomeEvents=new EventEmitter();
const welcome={active:true,message:{text:'Welcome',visible:false}};
welcomeRT.renderer={game:{events:welcomeEvents,scene:{getScene:()=>({
  sys:{isActive:()=>true},ui:{mode:0,visible:true,getHandler:()=>welcome}
})}}};
vm.runInNewContext(source,{__rt:welcomeRT,__native:{startupReady(){welcomeCalls++;}}});
welcomeRT.frameHooks[0]();welcomeEvents.emit('postrender');assert.equal(welcomeCalls,0);
welcome.message.visible=true;welcomeEvents.emit('postrender');assert.equal(welcomeCalls,1,'Visible welcome dialogue must release controls before the title screen');
console.log('PASS: loading feedback survives renderer creation, asynchronous/partial/hidden/loading scenes; ends once after rendered active UI and cleans listeners');
