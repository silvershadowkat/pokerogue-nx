// SPDX-License-Identifier: AGPL-3.0-only
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../js/storage.js'), 'utf8');
const file = 'localStorage-offline.json';
const encoded = wave => JSON.stringify({ sessionData0: String(wave) });
function boot(files, recovery = true, failFirstWrite = false) {
  const writes = [];
  const ctx = vm.createContext({
    __native: {
      baseURL: 'https://offline.invalid/', storageBackupRecovery: recovery,
      readFile: name => files[name] ?? null, log() {}, now: () => 2000,
      writeFile(name, data) {
        writes.push([name, data]);
        if (failFirstWrite) { failFirstWrite = false; return false; }
        files[name] = data; delete files[name + '.tmp']; return true;
      },
    },
    __rt: { frameHooks: [] }, document: {}, location: { hostname: 'offline.invalid' },
  });
  vm.runInContext(source, ctx);
  return { ctx, writes };
}
const cases = [
  [{}, null],
  [{ [file]: encoded(1), [file + '.tmp']: encoded(11) }, '11'],
  [{ [file]: encoded(11), [file + '.tmp']: '{"sessionData0":' }, '11'],
  [{ [file + '.bak']: encoded(10) }, '10'],
  [{ [file]: 'broken', [file + '.bak']: encoded(9) }, '9'],
  [{ [file + '.tmp']: '[]', [file + '.bak']: encoded(8) }, '8'],
  [{ [file + '.tmp']: '{"sessionData0":11}', [file]: encoded(7) }, '7'],
];
for (const [files, expected] of cases) {
  const { ctx } = boot(files);
  assert.equal(ctx.localStorage.getItem('sessionData0'), expected);
  ctx.__rt.flushStorage();
  if (expected !== null) assert.equal(boot(files).ctx.localStorage.getItem('sessionData0'), expected);
}
const files = { [file + '.tmp']: encoded(11) };
const { ctx, writes } = boot(files, true, true);
ctx.__rt.flushStorage(); ctx.__rt.flushStorage();
assert.equal(writes.length, 2); // Failed queue submission is retried, not silently forgotten.
assert.equal(boot(files).ctx.localStorage.getItem('sessionData0'), '11');
assert.equal(boot({ [file]: encoded(2), [file + '.tmp']: encoded(3) }, false).ctx.localStorage.getItem('sessionData0'), '2');
console.log('PASS: interrupted-write recovery, corrupt candidate fallback, restart roundtrip, queue failure retry, unchanged non-Switch reads');
