// Compare the real browser C++/WASM module against the native C++ CLI, including traces.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const vm = require('node:vm');
const {spawnSync} = require('node:child_process');
const createEngine = require('../docs/engine.js');
function equivalent(wasm, native, location = 'result') {
  if (typeof wasm === 'number') {
    assert(Math.abs(wasm - native) < 0.00001, `${location}: ${wasm} != ${native}`);
  } else if (wasm && typeof wasm === 'object') {
    assert.deepEqual(Object.keys(wasm), Object.keys(native), location);
    for (const key of Object.keys(wasm)) equivalent(wasm[key], native[key], `${location}.${key}`);
  } else {
    assert.equal(wasm, native, location);
  }
}
(async () => {
  const engine = await createEngine({wasmBinary: fs.readFileSync(path.join(__dirname, '../docs/engine.wasm'))});
  const simulate = configuration => JSON.parse(engine.ccall('simulate_world', 'string', ['string'], [configuration]));
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'life-wasm-test-'));
  try {
    for (const configuration of [
      'days=30\ntrace_day=1',
      'days=365\ntrace_day=8\nhourly_income=80\nwork_hours=6\nhealth=55',
      'days=30\nstrategy=money\nstart_date=2028-02-28\nvacation_days=0\ncash=5000',
    ]) {
      const file = path.join(temporary, 'world.conf'); fs.writeFileSync(file, configuration);
      const native = spawnSync(path.join(__dirname, '../build/life_tree'), ['--config', file, '--json'], {encoding: 'utf8', maxBuffer: 10000000});
      assert.equal(native.status, 0, native.stderr);
      const wasm = simulate(configuration);
      assert(!wasm.error, wasm.error);
      equivalent(wasm, JSON.parse(native.stdout));
      assert.equal(wasm.engine, 'cpp-behavior-tree');
      assert.equal(wasm.hours.length, 24);
    }
    assert(simulate('health=nan').error);
    assert(simulate('days=0').error);
    assert(simulate('start_date=invalid').error);
    // Exercise the exact worker script with the already-loaded real WASM engine.
    const messages = [];
    const context = {
      self: {location: {href: 'https://example.test/worker.js'}, postMessage: value => messages.push(value)},
      importScripts() {}, createLifeEngine: async () => engine, URL,
    };
    vm.createContext(context);
    vm.runInContext(fs.readFileSync(path.join(__dirname, '../docs/worker.js'), 'utf8'), context);
    await new Promise(resolve => setImmediate(resolve));
    assert.equal(messages[0].type, 'ready');
    context.self.onmessage({data: {id: 1, type: 'run', values: {days: '30', start_date: '2026-10-01', hourly_income: '100'}}});
    const world = messages.at(-1);
    assert.equal(world.type, 'result');
    assert(world.world.days.at(-1).state.debt < world.baseline.days.at(-1).state.debt);
    context.self.onmessage({data: {id: 2, type: 'trace', values: {days: '30', trace_day: '8'}}});
    assert.equal(messages.at(-1).type, 'trace');
    assert.equal(messages.at(-1).hours.length, 24);
    context.self.onmessage({data: {id: 3, type: 'run', values: {days: '0', start_date: '2026-10-01'}}});
    assert.equal(messages.at(-1).type, 'error');
    console.log('Native C++ and browser WASM results and execution traces match; invalid inputs rejected.');
  } finally { fs.rmSync(temporary, {recursive: true, force: true}); }
})().catch(error => {console.error(error); process.exitCode = 1;});
