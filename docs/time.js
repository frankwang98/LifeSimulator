'use strict';
// Calendar math uses Beijing date parts rather than the computer's local timezone.
const LifeTime = (() => {
  const formatter = new Intl.DateTimeFormat('en-CA', {
    timeZone: 'Asia/Shanghai', year: 'numeric', month: '2-digit', day: '2-digit',
    hour: '2-digit', minute: '2-digit', second: '2-digit', hourCycle: 'h23'
  });
  function beijing(now = new Date()) {
    const parts = Object.fromEntries(formatter.formatToParts(now).map(part => [part.type, part.value]));
    const date = `${parts.year}-${parts.month}-${parts.day}`;
    return {date, hour: Number(parts.hour), time: `${parts.hour}:${parts.minute}:${parts.second}`};
  }
  function dayIndex(start, date) {
    const calendar = value => {
      const [year, month, day] = value.split('-').map(Number);
      return Date.UTC(year, month - 1, day);
    };
    return Math.floor((calendar(date) - calendar(start)) / 86400000) + 1;
  }
  return {beijing, dayIndex};
})();
if (typeof module !== 'undefined') module.exports = LifeTime;
