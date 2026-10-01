'use strict';
const data = window.LIFE_DATA;
const labels = {money:'赚钱优先',health:'健康优先',balanced:'均衡生活'};
const colors = {money:'#b78349',health:'#648ca2',balanced:'#588471'};
const descriptions = {money:'工作日最多工作 10 小时，优先积累收入和还贷，其他活动保障最低频率。',health:'工作日最多工作 6 小时，每日运动，留出更多恢复与陪伴时间。',balanced:'工作日最多工作 8 小时，每日运动、学习与陪伴，在收入和生活之间找平衡。'};
const activityLabels = {sleep:'睡眠',recover:'休养',exercise:'运动',family:'陪伴',study:'学习',work:'工作',leisure:'休闲'};
let mode = 'balanced', timer = null;
const day = document.querySelector('#day');
const duration = document.querySelector('#duration');
const metric = document.querySelector('#metric');
const number = value => Math.round(value).toLocaleString('zh-CN');
function stop() { clearInterval(timer); timer = null; document.querySelector('#play').textContent = '▶ 播放'; }
function chart() {
  const key = metric.value, total = Number(duration.value), width = 620, height = 210;
  const values = Object.values(data).flatMap(rows => rows.slice(0,total).map(row=>row[key]));
  let low = Math.min(0,...values), high = Math.max(1,...values);
  const x = index => 66 + index / Math.max(1,total-1)*width;
  const y = value => 235-(value-low)/(high-low)*height;
  let content = '';
  for(let i=0;i<=4;i++) {
    const value = low+(high-low)*i/4, yy=y(value);
    content += `<line x1="66" y1="${yy}" x2="686" y2="${yy}" stroke="#ecefe8"/><text x="56" y="${yy+4}" text-anchor="end" fill="#859087" font-size="11">${number(value)}</text>`;
  }
  for(const strategy of Object.keys(labels)) {
    const points=data[strategy].slice(0,total).map((row,index)=>`${x(index)},${y(row[key])}`).join(' ');
    content+=`<polyline points="${points}" fill="none" stroke="${colors[strategy]}" stroke-width="${strategy===mode?3:1.5}" opacity="${strategy===mode?1:0.6}"/>`;
    const row=data[strategy][Number(day.value)-1];
    content+=`<circle cx="${x(Number(day.value)-1)}" cy="${y(row[key])}" r="4" fill="${colors[strategy]}"/>`;
  }
  content+=`<line x1="${x(Number(day.value)-1)}" y1="25" x2="${x(Number(day.value)-1)}" y2="235" stroke="#9aa69c" stroke-dasharray="4 4"/><text x="66" y="265" fill="#859087" font-size="11">第 1 天</text><text x="686" y="265" text-anchor="end" fill="#859087" font-size="11">第 ${total} 天</text>`;
  const svg=document.querySelector('#chart');svg.innerHTML=content;svg.setAttribute('aria-label',`${metric.selectedOptions[0].textContent}：三种策略前 ${total} 天趋势，当前第 ${day.value} 天`);
}
function render() {
  const row=data[mode][Number(day.value)-1];
  document.querySelector('#day-label').textContent=day.value;
  document.querySelector('#description').textContent=descriptions[mode];
  const cards=[['健康',number(row.health),'/ 100'],['剩余负债','¥ '+number(row.debt),'初始负债 ¥ 300,000'],['知识',row.knowledge.toFixed(1),'初始知识 20'],['现金','¥ '+number(row.cash),`年龄 ${row.age.toFixed(2)} 岁`]];
  document.querySelector('#stats').innerHTML=cards.map(([title,value,note])=>`<article class="stat"><span>${title}</span><strong>${value}</strong><small>${note}</small></article>`).join('');
  document.querySelector('#activities').innerHTML=Object.entries(activityLabels).map(([key,label])=>`<div class="activity"><div class="activity-label"><span>${label}</span><span>${row[key+'_hours']} 小时</span></div><div class="track"><div class="fill" style="width:${row[key+'_hours']/24*100}%"></div></div></div>`).join('');
  document.querySelector('#comparison').innerHTML=Object.entries(labels).map(([key,label])=>{const r=data[key][Number(day.value)-1];return `<tr><td class="${key}">${label}${mode===key?' · 当前':''}</td><td>${number(r.health)}</td><td>¥ ${number(r.debt)}</td><td>${r.knowledge.toFixed(1)}</td><td>${number(r.happiness)}</td></tr>`;}).join('');
  document.querySelectorAll('[data-mode]').forEach(button=>{button.classList.toggle('active',button.dataset.mode===mode);button.setAttribute('aria-pressed',button.dataset.mode===mode);});
  chart();
}
document.querySelectorAll('[data-mode]').forEach(button=>button.addEventListener('click',()=>{mode=button.dataset.mode;render();}));
day.addEventListener('input',()=>{stop();render();});
metric.addEventListener('change',chart);
duration.addEventListener('change',()=>{stop();day.max=duration.value;day.value=Math.min(Number(day.value),Number(duration.value));render();});
document.querySelector('#play').addEventListener('click',()=>{
  if(timer){stop();return;}
  if(Number(day.value)>=Number(day.max))day.value=1;
  document.querySelector('#play').textContent='Ⅱ 暂停';
  timer=setInterval(()=>{day.value=Math.min(Number(day.value)+3,Number(day.max));render();if(Number(day.value)>=Number(day.max))stop();},100);
});
if(!data){document.querySelector('#description').textContent='模拟数据未加载，请刷新页面。';document.querySelectorAll('button,input,select').forEach(el=>el.disabled=true);}else render();
