// SPDX-License-Identifier: AGPL-3.0-only
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const crypto = require('node:crypto');
const key = 'x0i2O7WRiANTqPmZ';
function derive(password,salt) {
  let result=Buffer.alloc(0),previous=Buffer.alloc(0);
  while(result.length<48){previous=crypto.createHash('md5').update(Buffer.concat([previous,Buffer.from(password),salt])).digest();result=Buffer.concat([result,previous]);}
  return [result.subarray(0,32),result.subarray(32,48)];
}
function encrypt(text) {
  const salt=Buffer.from('12345678'),[k,iv]=derive(key,salt),cipher=crypto.createCipheriv('aes-256-cbc',k,iv);
  return Buffer.concat([Buffer.from('Salted__'),salt,cipher.update(text),cipher.final()]).toString('base64');
}
const native={clientMode:'offline',utf8Decode:b=>Buffer.from(b).toString('utf8'),aesDecrypt(text,password){
  const raw=Buffer.from(text,'base64'),[k,iv]=derive(password,raw.subarray(8,16)),cipher=crypto.createDecipheriv('aes-256-cbc',k,iv);
  return [Buffer.concat([cipher.update(raw.subarray(16)),cipher.final()]).toString('utf8')];
}};
const rt={};
vm.runInNewContext(fs.readFileSync('runtime/js/save-import-policy.js','utf8'),{__rt:rt,__native:native});
const check=value=>rt.saveImportError(Buffer.from(value));
for(const data of [{silverShadowDebug:true},{system:{silverShadowDebug:true}},{slots:[{party:[],silverShadowDebug:true}]}]){
  assert.match(check(JSON.stringify(data)),/Debug mode/);
  assert.match(check(encrypt(JSON.stringify(data))),/Debug mode/);
}
for(const data of [{party:[],seed:'stock'},{settings:{language:'en'}},{description:'silverShadowDebug',silverShadowDebug:false}]){
  assert.equal(check(JSON.stringify(data)),null);
  assert.equal(check(encrypt(JSON.stringify(data))),null);
}
assert.equal(check('invalid file'),null,'stock importer still handles its own invalid file errors');
native.clientMode='debug';assert.equal(check(encrypt('{"silverShadowDebug":true}')),null);
native.clientMode='online';assert.match(check('{}'),/unavailable/);
console.log('Plaintext/encrypted Debug provenance, stock imports and Online restriction PASS');
