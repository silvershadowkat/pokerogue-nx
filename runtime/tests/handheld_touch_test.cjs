// SPDX-License-Identifier: AGPL-3.0-only
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
let fired=0,focused=0;
const events={};
const rect=(x,y,w,h)=>({visible:true,getBounds:()=>({x,y,width:w,height:h})});
const button={...rect(100,50,60,20),emit:()=>fired++};
const handler={active:true,modalContainer:{visible:true},buttonBgs:[button],buttonLabels:[{text:'Login'}],
  inputs:[{setFocus:()=>focused++}],inputContainers:[rect(100,80,60,20)]};
const canvas={width:320,height:180,addEventListener:(name,fn)=>events[name]=fn};
const scene={ui:{mode:32,getHandler:()=>handler},tweens:{getTweensOf:()=>[]}};
const game={canvas,scene:{getScene:()=>scene}};
const native={directTouch:true,screenSize:()=>({width:640,height:480}),log:()=>{}};
const rt={frameHooks:[],renderer:{game}};
vm.runInNewContext(fs.readFileSync('runtime/js/handheld-touch.js','utf8'),{window:{},__native:native,__rt:rt});
rt.frameHooks[0]();
const send=(kind,x,y,id=1)=>events[kind]({clientX:x,clientY:y,pointerId:id});
const tap=(x,y)=>{send('pointerdown',x,y);send('pointerup',x,y);};
tap(220,180);assert.equal(fired,1,'Letterboxed screen tap must activate the official login callback once');
tap(220,230);assert.equal(focused,1,'Field tap must focus the official input');
handler.modalContainer.visible=false;tap(220,180);assert.equal(fired,1,'Hidden modal cannot activate');handler.modalContainer.visible=true;
native.directTouch=false;tap(220,180);assert.equal(fired,1,'Pad-only mode must not use direct selection');native.directTouch=true;
send('pointerdown',220,180,1);send('pointerup',220,180,2);assert.equal(fired,1,'Another finger cannot release a selection');send('pointerup',220,180,1);assert.equal(fired,2);
send('pointerdown',220,180);scene.ui.mode=33;send('pointerup',220,180);assert.equal(fired,2,'A mode transition cannot click through');
console.log('PASS: official modal callbacks, field focus, 4:3 letterbox, disabled touch, pointer ownership and mode transitions');
