// SPDX-License-Identifier: AGPL-3.0-only
// Exercise the actual browser clock shim with fractional server-clock offsets.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const source = fs.readFileSync(path.join(__dirname, '../js/core.js'), 'utf8');

function boot(wall, offset) {
  const state = { wall, offset, monotonic: 1000 };
  const ctx = vm.createContext({
    state, __rt: {},
    __native: { logging: false, now: () => state.monotonic,
      clockOffset: () => state.offset, log: () => {} },
  });
  vm.runInContext('Date.now = () => state.wall;', ctx);
  vm.runInContext(source, ctx);
  return { state, ctx, evaluate: text => vm.runInContext(text, ctx) };
}

const cases = [
  [1793646945000, -241339.325], // The reported rejected save's .675 ms fraction.
  [1793646945000, 241339.325],
  [1793646945000, 0],
  [1793646945000, .999],
  [1793646945000, -.999],
  [-1000, .675],
  [-1000, -.675],
];
for (const [wall, offset] of cases) {
  const { evaluate } = boot(wall, offset);
  const expected = new Date(wall + offset).getTime();
  assert.equal(evaluate('Date.now()'), expected,
    'Date.now must use the same integer milliseconds as new Date().getTime()');
  assert.equal(evaluate('new Date().getTime()'), expected);
  assert.equal(evaluate('Number.isInteger(Date.now())'), true);
  const body = evaluate('JSON.stringify({system:{timestamp:Date.now()}, session:{timestamp:Date.now()}})');
  assert.match(body, /^\{"system":\{"timestamp":-?\d+\},"session":\{"timestamp":-?\d+\}\}$/,
    'The official save API expects integer JSON timestamps');
  assert.equal(evaluate('new Date(1234.9).getTime()'), 1234);
  assert.equal(evaluate('new Date("2020-02-29T00:00:00.000Z").toISOString()'), '2020-02-29T00:00:00.000Z');
  assert.equal(evaluate('Date.UTC(2020,1,29)'), Date.UTC(2020, 1, 29));
  assert.equal(evaluate('Date.parse("2020-02-29T00:00:00.000Z")'), Date.UTC(2020, 1, 29));
  assert.equal(evaluate('typeof Date()'), 'string');
  assert.equal(evaluate('new Date() instanceof Date'), true);
}

const { state, evaluate } = boot(1793646945000, 0);
const before = evaluate('performance.now()');
evaluate('globalThis.fired=false; setTimeout(()=>{fired=true;}, 10);');
state.offset = -241339.325; // A later HTTP response corrects wall time.
assert.equal(evaluate('Date.now()'), 1793646703660);
assert.equal(evaluate('performance.now()'), before);
state.monotonic += 9;
evaluate('__rt.onFrame(__native.now())');
assert.equal(evaluate('fired'), false);
state.monotonic += 1.5;
evaluate('__rt.onFrame(__native.now())');
assert.equal(evaluate('fired'), true);
assert.equal(evaluate('performance.now()'), before + 10.5,
  'High-resolution animation/timer time must retain its precision');
console.log('PASS: integer save timestamps with positive/negative fractional clock offsets; Date compatibility and high-resolution timers preserved');
