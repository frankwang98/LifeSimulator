'use strict';
let engine;
function configText(values) {
  return Object.entries(values).map(([key, value]) => `${key}=${value}`).join('\n');
}
function simulate(values) {
  const result = JSON.parse(engine.ccall('simulate_world', 'string', ['string'], [configText(values)]));
  if (result.error) throw new Error(result.error);
  return result;
}
(async () => {
  try {
    importScripts('engine.js');
    engine = await createLifeEngine({locateFile: path => new URL(path, self.location.href).href});
    self.postMessage({type: 'ready'});
  } catch (error) {
    self.postMessage({type: 'error', message: `C++ 引擎加载失败：${error.message}`});
  }
})();
self.onmessage = event => {
  const {id, type, values} = event.data;
  try {
    if (!engine) throw new Error('C++ 引擎尚未就绪');
    if (type === 'run') {
      // Baseline shares the user's start date and horizon; other assumptions stay at C++ defaults.
      const baseline = simulate({days: values.days, start_date: values.start_date, trace_day: 1});
      const world = simulate({...values, trace_day: 1});
      self.postMessage({id, type: 'result', baseline, world});
    } else if (type === 'trace') {
      const world = simulate(values);
      self.postMessage({id, type: 'trace', day: Number(values.trace_day), hours: world.hours, branches: world.branches});
    }
  } catch (error) {
    self.postMessage({id, type: 'error', message: error.message});
  }
};
