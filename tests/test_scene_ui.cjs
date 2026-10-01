// UI integration uses the real WASM engine; the lightweight DOM only hosts rendering/control logic.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
(async () => {
  const root = path.join(__dirname, '..');
  const html = fs.readFileSync(path.join(root, 'docs/index.html'), 'utf8');
  const fields = {};
  for (const match of html.matchAll(/<input\s+name="([^"]+)"[^>]*value="([^"]+)"/g)) fields[match[1]] = {value: match[2]};
  for (const match of html.matchAll(/<select\s+name="([^"]+)"[^>]*>([\s\S]*?)<\/select>/g)) {
    const options = [...match[2].matchAll(/<option value="([^"]+)"([^>]*)>/g)];
    fields[match[1]] = {value: (options.find(option => option[2].includes('selected')) || options[0])[1]};
  }
  const elements = {};
  const element = id => elements[id] ||= {value: '', innerHTML: '', textContent: '', handlers: {}, classList: {toggle() {}}, setAttribute() {}, addEventListener(event, callback) {this.handlers[event] = callback;}};
  element('#world-form').elements = {namedItem: name => fields[name]};
  element('#world-form').reportValidity = () => true;
  element('#day').value = '1'; element('#hour').value = '12'; element('#metric').value = 'health';
  const sent = [], storage = {}, intervals = [];
  let worker, timeout, instant = '2026-10-01T12:34:56Z';
  class Clock extends Date {constructor(...args) {super(...(args.length ? args : [instant]));}}
  class Worker {constructor() {worker = this;} postMessage(message) {sent.push(message);}}
  class FormData {constructor() {this.entries = Object.entries(fields).map(([key, field]) => [key, field.value]);} [Symbol.iterator]() {return this.entries[Symbol.iterator]();}}
  const context = {document: {querySelector: element, addEventListener() {}}, Date: Clock, Intl, Worker, FormData,
    localStorage: {getItem: key => storage[key] || null, setItem: (key, value) => storage[key] = value},
    setTimeout: callback => {timeout = callback; return 1;}, clearTimeout() {}, setInterval: callback => {intervals.push(callback); return intervals.length;}, clearInterval() {}, URL, Blob};
  vm.createContext(context);
  for (const file of ['time.js', 'scene.js', 'app.js']) vm.runInContext(fs.readFileSync(path.join(root, 'docs', file), 'utf8'), context);
  const engine = await require(path.join(root, 'docs/engine.js'))({wasmBinary: fs.readFileSync(path.join(root, 'docs/engine.wasm'))});
  const simulate = values => JSON.parse(engine.ccall('simulate_world', 'string', ['string'], [Object.entries(values).map(([key, value]) => `${key}=${value}`).join('\n')]));
  function respond(message) {
    const world = simulate(message.values); assert(!world.error, world.error);
    let response;
    if (message.type === 'run') response = {id: message.id, type: 'result', world, baseline: simulate({days: message.values.days, start_date: message.values.start_date})};
    else if (message.type === 'live') {const row = world.days.at(-1); response = {id: message.id, type: 'live', date: row.date, day: row.day, weekday: row.weekday, workday: row.workday, hours: world.hours, branches: world.branches};}
    else response = {id: message.id, type: 'trace', day: Number(message.values.trace_day), hours: world.hours, branches: world.branches};
    worker.onmessage({data: response});
  }
  worker.onmessage({data: {type: 'ready'}});
  while (sent.length) respond(sent.shift());
  assert.equal(element('#world-clock').textContent, '2026-10-01 20:34:56');
  assert(element('#scene-status').textContent.includes('陪伴'));
  assert(element('#scene').innerHTML.includes('avatar family'));
  assert.equal((element('#scene').innerHTML.match(/class="avatar/g) || []).length, 2);
  assert(element('#agent-status').innerHTML.includes('伴侣 Agent'));
  assert(element('#utility').innerHTML.includes('Utility 分数'));
  assert(element('#stats').innerHTML.includes('本小时开始'));
  const count = sent.length;
  element('#gender').value = 'female'; element('#gender').handlers.change();
  assert(element('#scene').innerHTML.includes('#b98d87'));
  assert.equal(sent.length, count, 'gender must not change C++ parameters');
  element('#future-mode').handlers.click();
  element('#day').value = '8'; element('#day').handlers.input(); timeout(); respond(sent.shift());
  assert(element('#scene-status').textContent.includes('工作'));
  assert(element('#scene').innerHTML.includes('avatar work'));
  assert(element('#world-clock').textContent.startsWith('2026-10-08'));
  fields.cash.value = '9000'; element('#world-form').handlers.submit({preventDefault() {}}); respond(sent.shift());
  assert.equal(JSON.parse(storage['life-live-v1']).cash, '0', 'future changes must not overwrite live settings');
  element('#live-mode').handlers.click(); respond(sent.shift());
  assert.equal(element('#world-clock').textContent, '2026-10-01 20:34:56');
  assert(element('#scene-status').textContent.includes('陪伴'));
  instant = '2026-10-01T16:00:00Z'; intervals[0](); respond(sent.shift());
  assert.equal(element('#world-clock').textContent, '2026-10-02 00:00:00');
  assert(element('#scene').innerHTML.includes('avatar sleep'));
  fields.start_date.value = '2026-10-03'; element('#apply-live').handlers.click();
  assert(element('#scene-status').textContent.includes('尚未开始'));
  assert.equal(element('#scene').innerHTML, '');
  console.log('Live/future scenes, gender, real Beijing clock, midnight and separate worlds passed with real C++ data.');
})().catch(error => {console.error(error); process.exitCode = 1;});
