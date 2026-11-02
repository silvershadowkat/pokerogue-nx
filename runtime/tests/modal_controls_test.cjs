// SPDX-License-Identifier: AGPL-3.0-only
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const UP=0, DOWN=1, LEFT=2, RIGHT=3, SUBMIT=4, A=5, B=6;
function visual() { return {visible:true,active:true,setTint(t){this.tint=t;},clearTint(){delete this.tint;}}; }
let login=0,register=0,submitted=0,cancelled=0,focused=[],busy=false,originalCalls=0;
const button=fn=>Object.assign(visual(),{input:{enabled:true},emit(name){assert.equal(name,'pointerdown');fn();}});
const choice={active:true,modalContainer:visual(),inputs:[],buttonBgs:[button(()=>login++),button(()=>register++)],processInput(){originalCalls++;return false;}};
const username={},password={};
const field=(node,name)=>({node,setFocus(){focused.push(name);}});
const form={active:true,modalContainer:visual(),inputs:[field(username,'username'),field(password,'password')],
  inputContainers:[{...visual(),list:[visual()]},{...visual(),list:[visual()]}],
  buttonBgs:[button(()=>submitted++),button(()=>cancelled++)],cancelAction:()=>cancelled++,
  processInput(button){originalCalls++;if(button===SUBMIT){submitted++;return true;}return false;}};
let handler=choice;
const scene={ui:{getHandler:()=>handler,overlayActive:false},tweens:{getTweensOf:()=>busy?[{}]:[]}};
const rt={frameHooks:[],renderer:{game:{scene:{getScene:()=>scene}}}};
vm.runInNewContext(fs.readFileSync('runtime/js/modal-controls.js','utf8'),{__rt:rt});
const tick=()=>rt.frameHooks.forEach(fn=>fn());
const press=b=>handler.processInput(b);
tick();assert.equal(choice.buttonBgs[0].tint,0xbbbbbb);
press(RIGHT);assert.equal(choice.buttonBgs[1].tint,0xbbbbbb);press(A);assert.equal(register,1);assert.equal(login,0);
press(LEFT);press(A);assert.equal(login,1);
press(DOWN);press(UP);assert.equal(choice.buttonBgs[0].tint,0xbbbbbb);
busy=true;assert.equal(press(A),false);assert.equal(login,1);busy=false;
scene.ui.overlayActive=true;assert.equal(press(A),false);assert.equal(login,1);scene.ui.overlayActive=false;
handler=form;tick();press(A);assert.deepEqual(focused,['username']);
rt.onFormTextComplete(username,password,true);press(A);assert.deepEqual(focused,['username','password']);
rt.onFormTextComplete(password,null,true);assert.equal(form.buttonBgs[0].tint,0xbbbbbb);
press(A);assert.equal(submitted,1,'A must activate Login after the last keyboard, without a screen tap');
press(RIGHT);press(A);assert.equal(cancelled,1,'D-pad and A must reach Back');
press(B);assert.equal(cancelled,2);
press(SUBMIT);assert.equal(submitted,2,'Start retains the official form submit callback');
press(UP);rt.onFormTextComplete(password,null,false);assert.equal(form.buttonBgs[0].tint,undefined,'Cancelled text entry must not jump to submit');
form.buttonBgs[1].visible=false;rt.onFormTextComplete(password,null,true);press(RIGHT);press(RIGHT);press(RIGHT);press(A);assert.equal(submitted,3,'Hidden Back must be skipped');
assert.equal(originalCalls,1,'Only official Start submit is delegated in this flow');
console.log('PASS: Login/Register D-pad selection, A confirm, field navigation, final-password-to-Login, Back, Start, hidden controls and animation/overlay guards');

