const assert = require('node:assert/strict');
const {beijing, dayIndex} = require('../docs/time.js');
assert.deepEqual(beijing(new Date('2026-10-01T12:34:56Z')), {date: '2026-10-01', hour: 20, time: '20:34:56'});
assert.deepEqual(beijing(new Date('2026-10-01T16:00:00Z')), {date: '2026-10-02', hour: 0, time: '00:00:00'});
assert.equal(dayIndex('2026-10-01', '2026-10-01'), 1);
assert.equal(dayIndex('2026-10-01', '2026-10-08'), 8);
assert.equal(dayIndex('2026-10-01', '2026-09-30'), 0);
assert.equal(dayIndex('2028-02-28', '2028-03-01'), 3);
console.log('Beijing clock, midnight, future starts and leap-day calculations passed.');
