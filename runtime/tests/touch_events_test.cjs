// SPDX-License-Identifier: AGPL-3.0-only
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const events=[],timers=new Map(),masks=[];let nextTimer=0,resets=0;
let scene={ui:{mode:32}};
const rt={frameHooks:[],renderer:{game:{events:{emit:(kind,data)=>events.push([kind,data.button,data.controller_type,data.isTouch])},scene:{getScene:()=>scene}}}};
const native={resetTouchControls:()=>resets++,setTouchControlsMask:mask=>masks.push(mask),gamepads:()=>{throw Error('Touch must not alter physical pad state');}};
vm.runInNewContext(fs.readFileSync('runtime/js/touch-events.js','utf8'),{__rt:rt,__native:native,setInterval:(fn,ms)=>{assert.equal(ms,250);timers.set(++nextTimer,fn);return nextTimer;},clearInterval:id=>timers.delete(id)});
rt.frameHooks[0]();assert.equal(resets,1);assert.equal(masks.at(-1),(1<<0)|(1<<1)|(1<<9));
rt.onTouchControl(15,true);rt.onTouchControl(15,true);assert.equal(timers.size,1);assert.equal(events.length,1);
for(const fn of timers.values())fn();assert.equal(events.length,2);
rt.onTouchControl(15,false);rt.onTouchControl(12,true);assert.deepEqual(events.slice(-2).map(x=>x.slice(0,2)),[['input_up',3],['input_down',0]]);
assert.equal(timers.size,1);rt.onTouchControl(0,true);assert.equal(timers.size,2);rt.onTouchControl(12,false);assert.equal(timers.size,1);rt.onTouchControl(0,false);assert.equal(timers.size,0);
assert.ok(events.every(x=>x[2]==='keyboard'&&x[3]===true));
scene.ui.mode=10;rt.frameHooks[0]();assert.equal(masks.at(-1),0x3ff);
scene.ui.mode=2;rt.frameHooks[0]();assert.ok(masks.at(-1)&(1<<7));assert.ok(masks.at(-1)&(1<<8));assert.equal(masks.at(-1)&(1<<2),0);
scene.ui.mode=18;rt.frameHooks[0]();assert.ok(masks.at(-1)&(1<<2));assert.ok(masks.at(-1)&(1<<4));
rt.onTouchControl(1,true);assert.equal(timers.size,1);scene={ui:{mode:32}};rt.frameHooks[0]();assert.equal(timers.size,0);assert.equal(resets,2);
console.log('PASS: original keyboard/touch event contract, 250 ms repeats, ordered transitions, independent physical input, contextual controls and scene cleanup');
