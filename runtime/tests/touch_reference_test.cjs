// SPDX-License-Identifier: AGPL-3.0-only
// Differential checks against the retained original TS, not another rewrite.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),cp=require('node:child_process');
const ts=require('typescript');
const provenance=JSON.parse(fs.readFileSync('runtime/tests/reference/silvershadow-provenance.json','utf8'));
for(const [name,hash] of Object.entries(provenance.files)){
  const source=fs.readFileSync('runtime/tests/reference/'+name);
  const original=source.subarray(source.indexOf(10)+1);
  assert.equal(require('node:crypto').createHash('sha256').update(original).digest('hex'),hash,'Original controller fixture changed: '+name);
}
const driver=process.env.PKR_TOUCH_REFERENCE_DRIVER||require('node:path').resolve('runtime/build-tests/touch-reference-driver');
const exe=process.platform==='win32'?'wsl':driver;
const args=process.platform==='win32'?['-d','Ubuntu','--',driver.replace(/\\/g,'/').replace(/^([a-z]):/i,(_,d)=>'/mnt/'+d.toLowerCase())]:[];
const runNative=(input,maxBuffer=16777216)=>cp.spawnSync(exe,args,{input,encoding:'utf8',maxBuffer,timeout:60000});
function load(name){const exports={};vm.runInNewContext(ts.transpileModule(fs.readFileSync('runtime/tests/reference/'+name,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,{exports});return exports;}
const input=load('silvershadow-touch-input.ts'),visual=load('silvershadow-dpad-visual.ts');
const keys={UP:12,DOWN:13,LEFT:14,RIGHT:15,A:0,B:1};const names={12:'UP',13:'DOWN',14:'LEFT',15:'RIGHT','-1':null};
const commands=[],expected=[];
let seed=20817;function random(){seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;}
const vectors=[[0,0,100], [16,0,100],[15.999,15.999,100],[25,-25,100],[-25,25,100],[0,-30,100],[20,40,100],[40,20,100],[1000,-1000,100]];
for(let i=0;i<3000;i++)vectors.push([(random()-.5)*2000,(random()-.5)*2000,1+random()*1000]);
for(const [x,y,w] of vectors){for(const direction of [-1,12,13,14,15]){
 commands.push(`v ${x} ${y} ${w} ${direction}`);
 const d=input.resolveDpadDirection(x,y,w),p=visual.calculateDpadVisualPose(x,y,w,names[direction]);
 expected.push([d==null?-1:keys[d],p.normalizedX,p.normalizedY,p.normalizedDistance,p.tiltXDegrees,p.tiltYDegrees,p.shadowX,p.shadowY,p.pressedDepth,p.lightUp,p.lightRight,p.lightDown,p.lightLeft]);
}}
const native=runNative(commands.join('\n')+'\n');
assert.equal(native.status,0,native.stderr);const lines=native.stdout.trim().split(/\r?\n/);assert.equal(lines.length,expected.length);
lines.forEach((line,i)=>line.trim().split(/\s+/).map(Number).forEach((n,k)=>assert.ok(Math.abs(n-expected[i][k])<1e-8,`${i}/${k}: ${n} != ${expected[i][k]}`)));
const geometryResult=runNative('g\n');assert.equal(geometryResult.status,0,geometryResult.stderr);
const geometry=geometryResult.stdout.trim().split(/\s+/).map(Number);
const [x,y,w,h]=geometry,cx=Math.fround(x+w/2),cy=Math.fround(y+h/2);
let transitions=[],mask=0;const state=new input.SilverShadowTouchInputState({press(key){mask|=1<<keys[key];transitions.push([keys[key],1]);},release(key){mask&=~(1<<keys[key]);transitions.push([keys[key],0]);}});
const ops=[],states=[];
function step(kind,id,px,py,role){px=Math.fround(px);py=Math.fround(py);ops.push(`e ${kind} ${id} ${px} ${py}`);transitions=[];
 if(kind===0){if(role==='dpad')state.captureDpad(id,px,py,cx,cy,w*.84);else if(role)state.pressAction(id,role);}
 else if(kind===1)state.moveDpad(id,px,py,cx,cy,w*.84);
 else state.releasePointer(id);
 states.push([mask,state.dpadOwner??-1,state.activeDirection?keys[state.activeDirection]:-1,transitions.length,...transitions.flat()]);}
step(0,1,cx,cy,'dpad');step(1,1,cx+w,cy);step(1,1,cx,cy-w);step(1,1,cx,cy);step(0,2,cx-w/3,cy,'dpad');step(1,2,cx,cy+w);
const size=960*.2;const ax=1280-16-size/2,ay=960-16-size*1.5,bx=1280-16-2.2*size+size/2,by=960-16-size/2;
step(0,3,ax,ay,'A');step(0,4,ax,ay,'A');step(1,3,600,10);step(0,5,bx,by,'B');step(2,3,600,10);step(2,4,ax,ay);step(3,1,cx,cy);step(2,2,cx,cy);step(2,5,bx,by);
step(0,6,cx,cy-w/3,'dpad');step(0,7,ax,ay,'A');transitions=[];state.reset();ops.push('r');states.push([0,-1,-1,transitions.length,...transitions.flat()]);
const result=runNative(ops.join('\n')+'\n');assert.equal(result.status,0,result.stderr);
result.stdout.trim().split(/\r?\n/).forEach((line,i)=>assert.deepEqual(line.trim().split(/\s+/).map(Number),states[i],`pointer trace ${i}`));
console.log(`PASS: ${expected.length} original TS/native pose+resolver comparisons and ${states.length} multi-pointer/ordered-transition trace steps`);
