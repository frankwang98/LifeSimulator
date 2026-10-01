'use strict';
const $ = selector => document.querySelector(selector);
const form = $('#world-form');
const day = $('#day');
const hour = $('#hour');
const actionKeys = ['sleep', 'recover', 'exercise', 'family', 'study', 'work', 'leisure'];
const actionLabels = {sleep: '睡眠', recover: '休养', exercise: '运动', family: '陪伴', study: '学习', work: '工作', leisure: '休闲'};
const scoreLabels = {health: '健康', energy: '精力', cash: '现金', debt: '负债', knowledge: '知识', happiness: '幸福', relationship: '关系'};
const weekdays = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];
const fmt = value => Number(value).toLocaleString('zh-CN', {maximumFractionDigits: 1});
const defaults = Object.fromEntries(new FormData(form));
let result = null, activeValues = null, pendingValues = null;
let timer = null, traceTimer = null, request = 0, runId = 0, traceId = 0, pendingTraceDay = 0;
let traceDay = 1, hours = [], branches = [];
let timeMode = 'live', engineReady = false, liveValues = null, liveData = null, liveId = 0, liveSlot = '';
try { liveValues = JSON.parse(localStorage.getItem('life-live-v1')); } catch (_) {}
let gender = 'male';
try { gender = localStorage.getItem('life-gender') === 'female' ? 'female' : 'male'; } catch (_) {}
$('#gender').value = gender;
const worker = new Worker('worker.js');
function clearScene(message) {
  $('#scene').innerHTML = ''; $('#scene-title').textContent = '我的平行世界';
  $('#scene-message').textContent = message; $('#scene-reason').textContent = '';
  $('#tree').innerHTML = ''; $('#events').innerHTML = ''; $('#deltas').textContent = '';
  if (timeMode === 'live') { $('#stats').innerHTML = ''; $('#activities').innerHTML = ''; }
  $('#action-label').textContent = message; $('#root-status').textContent = 'Selector · 等待有效记录';
}
function currentTick() {
  if (timeMode === 'live') {
    const now = LifeTime.beijing();
    return liveData && liveData.date === now.date ? liveData.hours[now.hour] : null;
  }
  return traceDay === Number(day.value) ? hours[Number(hour.value)] : null;
}
function currentClock() {
  if (timeMode === 'live') {
    const now = LifeTime.beijing();
    $('#world-clock').textContent = `${now.date} ${now.time}`;
    $('#clock-caption').textContent = '北京时间 · 与现实同步';
    if (!engineReady || !liveValues) return;
    const index = LifeTime.dayIndex(liveValues.start_date, now.date);
    if (index < 1 || index > 3650) {
      liveData = null; $('#scene-status').textContent = index < 1 ? '当前世界尚未开始' : '已超过当前模型支持的十年范围';
      clearScene(index < 1 ? `世界将于 ${liveValues.start_date} 开始` : '请调整当前世界起点');
      $('#stats').innerHTML = ''; $('#activities').innerHTML = ''; return;
    }
    const slot = `${now.date}/${now.hour}`;
    if (liveSlot === slot) return;
    liveSlot = slot; liveId = ++request; liveData = null;
    clearScene('正在读取此刻的 C++ 决策…');
    $('#scene-status').textContent = '正在同步平行世界';
    worker.postMessage({id: liveId, type: 'live', values: {...liveValues, days: index, trace_day: index}});
  } else if (result) {
    const row = result.world.days[Number(day.value) - 1];
    $('#world-clock').textContent = `${row.date} ${String(hour.value).padStart(2, '0')}:00:00`;
    $('#clock-caption').textContent = '模拟时间 · 未来推演';
  }
}
function renderLive() {
  if (!liveData) return;
  const tick = currentTick(); if (!tick) return;
  $('#stats').innerHTML = ['health', 'debt', 'cash', 'knowledge'].map(key => `<article class="stat"><span>${scoreLabels[key]}</span><strong>${key === 'debt' || key === 'cash' ? '¥ ' : ''}${fmt(tick.before[key])}</strong><small>与现实同步 · 本小时开始的状态</small></article>`).join('');
  const counts = actionKeys.map(key => liveData.hours.slice(0, tick.hour).filter(value => value.action === key).length);
  $('#activities').innerHTML = actionKeys.map((key, index) => `<div class="activity"><div class="activity-label"><span>${actionLabels[key]}</span><span>已完成 ${counts[index]} 小时</span></div><div class="track"><div class="fill" style="width:${counts[index] / 24 * 100}%"></div></div></div>`).join('');
  renderTrace();
}
function switchMode(next) {
  stop(); clearTimeout(traceTimer); traceId = ++request; timeMode = next;
  $('#live-mode').classList.toggle('active', next === 'live');
  $('#future-mode').classList.toggle('active', next === 'future');
  $('#live-mode').setAttribute('aria-pressed', next === 'live');
  $('#future-mode').setAttribute('aria-pressed', next === 'future');
  $('#future-timeline').hidden = next === 'live';
  day.disabled = next === 'live' || !result; hour.disabled = next === 'live' || !result;
  $('#play').disabled = next === 'live' || !result;
  if (next === 'live') {
    liveSlot = ''; currentClock();
  } else if (result) {
    renderDay(); currentClock();
    if (traceDay === Number(day.value)) renderTrace(); else requestTrace();
  } else clearScene('请先生成平行世界');
}

function status(message) { $('#engine-status').textContent = message; }
function stop() { clearInterval(timer); timer = null; $('#play').textContent = '▶ 回放一天'; }
function settings() { return Object.fromEntries(new FormData(form)); }
function restore(values) {
  for (const [key, value] of Object.entries(values)) {
    const field = form.elements.namedItem(key);
    if (field && key in defaults) field.value = value;
  }
}
function run() {
  if (!form.reportValidity()) return;
  stop(); clearTimeout(traceTimer); pendingTraceDay = 0; traceId = ++request;
  pendingValues = settings(); runId = ++request;
  $('#run').disabled = true; $('#day').disabled = true; $('#hour').disabled = true; $('#play').disabled = true;
  status('正在用 C++ 推演两个世界…');
  worker.postMessage({id: runId, type: 'run', values: pendingValues});
}
function requestTrace() {
  if (!result) return;
  stop(); traceId = ++request;
  const requestedDay = Number(day.value); pendingTraceDay = requestedDay;
  $('#hour').disabled = true; $('#play').disabled = true;
  $('#action-label').textContent = '正在读取这一天的 C++ 执行轨迹…';
  $('#root-status').textContent = 'Selector · 正在读取执行记录';
  clearScene('正在读取这一天的 C++ 执行轨迹…');
  clearTimeout(traceTimer);
  traceTimer = setTimeout(() => worker.postMessage({id: traceId, type: 'trace', values: {...activeValues, trace_day: requestedDay}}), 120);
}
function plot() {
  if (!result) return;
  const key = $('#metric').value;
  const worlds = [{value: result.baseline, color: '#648ca2'}, {value: result.world, color: '#588471'}];
  const count = result.world.days.length;
  const values = worlds.flatMap(world => [world.value.initial[key], ...world.value.days.map(row => row.state[key])]);
  const low = Math.min(0, ...values), high = Math.max(1, ...values);
  const x = index => 66 + index / count * 620;
  const y = value => 235 - (value - low) / (high - low) * 210;
  let content = '';
  for (let i = 0; i <= 4; i++) {
    const value = low + (high - low) * i / 4, yy = y(value);
    content += `<line x1="66" y1="${yy}" x2="686" y2="${yy}" stroke="#ecefe8"/><text x="56" y="${yy + 4}" text-anchor="end" fill="#859087" font-size="11">${fmt(value)}</text>`;
  }
  for (const world of worlds) {
    const points = [world.value.initial[key], ...world.value.days.map(row => row.state[key])].map((value, index) => `${x(index)},${y(value)}`).join(' ');
    content += `<polyline points="${points}" fill="none" stroke="${world.color}" stroke-width="2.5"/>`;
  }
  content += `<line x1="${x(Number(day.value))}" y1="25" x2="${x(Number(day.value))}" y2="235" stroke="#9aa69c" stroke-dasharray="4 4"/><text x="66" y="265" fill="#859087" font-size="11">起点</text><text x="686" y="265" text-anchor="end" fill="#859087" font-size="11">${result.world.days[count - 1].date}</text>`;
  $('#chart').innerHTML = content;
  $('#chart').setAttribute('aria-label', `${scoreLabels[key]}：初始世界和平行世界的 ${count} 天曲线`);
}
function renderDay() {
  if (!result || timeMode === 'live') return;
  const row = result.world.days[Number(day.value) - 1];
  const base = result.baseline.days[Number(day.value) - 1];
  $('#day-label').textContent = day.value;
  $('#date-label').textContent = `${row.date} · ${weekdays[row.weekday]} · ${row.workday ? '工作日' : '休息日'}`;
  $('#stats').innerHTML = ['health', 'debt', 'cash', 'knowledge'].map(key => `<article class="stat"><span>${scoreLabels[key]}</span><strong>${key === 'debt' || key === 'cash' ? '¥ ' : ''}${fmt(row.state[key])}</strong><small>初始世界 ${fmt(base.state[key])} · 差值 ${fmt(row.state[key] - base.state[key])}</small></article>`).join('');
  $('#activities').innerHTML = actionKeys.map((key, index) => `<div class="activity"><div class="activity-label"><span>${actionLabels[key]}</span><span>${row.counts[index]} 小时</span></div><div class="track"><div class="fill" style="width:${row.counts[index] / 24 * 100}%"></div></div></div>`).join('');
  const last = result.world.days.at(-1), original = result.baseline.days.at(-1);
  $('#future-summary').textContent = `推演终点 ${last.date}：平行世界健康 ${fmt(last.state.health)}（初始世界 ${fmt(original.state.health)}），负债 ¥ ${fmt(last.state.debt)}（初始世界 ¥ ${fmt(original.state.debt)}）。`;
  plot();
}
function renderTrace() {
  const tick = currentTick(); if (!tick) return;
  const activeBranches = timeMode === 'live' ? liveData.branches : branches;
  LifeScene.render(tick.action, gender);
  $('#scene-status').textContent = `${timeMode === 'live' ? '现在' : '模拟'} · ${actionLabels[tick.action]} · ${timeMode === 'live' ? (liveData.workday ? '工作日' : '休息日') : ''}`;
  currentClock();
  $('#hour-label').textContent = `${String(tick.hour).padStart(2, '0')}:00`;
  $('#action-label').textContent = `这一小时：${actionLabels[tick.action]} · C++ Action SUCCESS`;
  $('#deltas').textContent = Object.keys(scoreLabels).filter(key => Math.abs(tick.after[key] - tick.before[key]) > 0.000001).map(key => `${scoreLabels[key]} ${fmt(tick.before[key])} → ${fmt(tick.after[key])}`).join('；') || '状态没有变化';
  const eventMap = Object.fromEntries(tick.events.map(event => [event.node, event.status]));
  $('#root-status').textContent = `Selector · ${eventMap.root} · 选择第一个成功分支`;
  const selected = activeBranches.find(branch => eventMap[branch.id] === 'SUCCESS');
  $('#scene-reason').textContent = selected ? `选择原因：${selected.condition} → ${actionLabels[tick.action]}。` : '';
  $('#tree').innerHTML = activeBranches.map(branch => {
    const outcome = eventMap[branch.id], condition = eventMap[`${branch.id}.condition`], action = eventMap[`${branch.id}.action`];
    return `<div class="branch ${outcome === 'SUCCESS' ? 'selected' : outcome === 'FAILURE' ? 'failed' : 'skipped'}"><div class="sequence-label">Sequence · ${outcome || '未访问'}</div><div class="node ${condition === 'SUCCESS' ? 'passed' : condition === 'FAILURE' ? 'failed' : ''}">Condition · ${branch.condition}<b>${condition || '未访问'}</b></div><div class="node ${action ? 'passed' : ''}">Action · ${actionLabels[branch.action]}<b>${action || '未访问'}</b></div></div>`;
  }).join('');
  $('#events').innerHTML = tick.events.map(event => `<li>${event.node} [${event.kind}] → ${event.status}</li>`).join('');
}
worker.onmessage = ({data}) => {
  if (data.type === 'ready') { engineReady = true; if (!liveValues) { liveValues = settings(); try { localStorage.setItem('life-live-v1', JSON.stringify(liveValues)); } catch (_) {} } $('#apply-live').disabled = false; status('C++ / WebAssembly 引擎就绪 · 计算在本地进行'); run(); currentClock(); return; }
  if (data.type === 'error') {
    if (data.id && data.id !== runId && data.id !== traceId && data.id !== liveId) return;
    status(data.message); $('#run').disabled = false; stop();
    if (data.id === liveId) { liveData = null; clearScene('当前世界计算失败，请检查设定'); }
    return;
  }
  if (data.type === 'result' && data.id === runId) {
    result = data; activeValues = {...pendingValues}; hours = data.world.hours; branches = data.world.branches; traceDay = 1;
    day.max = data.world.days.length; day.value = 1; hour.value = 12;
    $('#run').disabled = false; day.disabled = timeMode === 'live'; hour.disabled = timeMode === 'live'; $('#play').disabled = timeMode === 'live';
    $('#dirty').textContent = ''; status('运行完成 · 日末数据和节点轨迹均来自 C++ 行为树');
    try { localStorage.setItem('life-world-v2', JSON.stringify(activeValues)); } catch (_) {}
    if (timeMode === 'future') { renderDay(); renderTrace(); } else { plot(); renderLive(); }
  } else if (data.type === 'live' && data.id === liveId) {
    liveData = data; if (timeMode === 'live') renderLive();
  } else if (data.type === 'trace' && data.id === traceId && data.day === pendingTraceDay && timeMode === 'future') {
    hours = data.hours; branches = data.branches; traceDay = data.day;
    hour.disabled = false; $('#play').disabled = false; renderTrace();
  }
};
worker.onerror = () => { status('C++ 引擎加载失败，请刷新或检查 Pages 发布状态。'); stop(); };
form.addEventListener('submit', event => { event.preventDefault(); run(); });
form.addEventListener('input', () => $('#dirty').textContent = '设定已修改，生成后生效');
$('#reset').addEventListener('click', () => { restore(defaults); $('#dirty').textContent = '已恢复初始设定，生成后生效'; });
day.addEventListener('input', () => { renderDay(); requestTrace(); });
hour.addEventListener('input', () => { stop(); renderTrace(); });
$('#metric').addEventListener('change', plot);
$('#play').addEventListener('click', () => {
  if (timer) { stop(); return; }
  if (timeMode !== 'future') return;
  hour.value = 0; renderTrace(); $('#play').textContent = 'Ⅱ 暂停';
  timer = setInterval(() => { if (Number(hour.value) === 23) { stop(); return; } hour.value = Number(hour.value) + 1; renderTrace(); }, 650);
});
$('#export').addEventListener('click', () => {
  const url = URL.createObjectURL(new Blob([JSON.stringify(settings(), null, 2)], {type: 'application/json'}));
  const link = document.createElement('a'); link.href = url; link.download = 'my-parallel-world.json'; link.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
});
$('#import').addEventListener('change', async event => {
  try { const values = JSON.parse(await event.target.files[0].text()); if (!values || typeof values !== 'object' || Array.isArray(values)) throw new Error('格式不正确'); restore(values); $('#dirty').textContent = '已导入设定，生成后生效'; }
  catch (_) { status('导入失败：请使用导出的 JSON 设定文件。'); }
  event.target.value = '';
});
try { const saved = JSON.parse(localStorage.getItem('life-world-v2')); if (saved && typeof saved === 'object') restore(saved); } catch (_) {}

$('#live-mode').addEventListener('click', () => switchMode('live'));
$('#future-mode').addEventListener('click', () => switchMode('future'));
$('#gender').addEventListener('change', () => {
  gender = $('#gender').value === 'female' ? 'female' : 'male';
  try { localStorage.setItem('life-gender', gender); } catch (_) {}
  renderTrace();
});
$('#apply-live').addEventListener('click', () => {
  if (!engineReady || !form.reportValidity()) return;
  liveValues = settings();
  try { localStorage.setItem('life-live-v1', JSON.stringify(liveValues)); } catch (_) {}
  liveSlot = ''; liveData = null; switchMode('live');
  status('当前世界设定已更新；未来推演使用独立结果');
});
currentClock();
setInterval(currentClock, 1000);
document.addEventListener('visibilitychange', () => { if (!document.hidden) currentClock(); });
