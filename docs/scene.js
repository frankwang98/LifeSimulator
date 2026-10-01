'use strict';
const LifeScene = (() => {
  const scenes = {
    sleep: {name: '卧室', message: '好好睡一觉，让精力慢慢回来。', color: '#7386a5'},
    recover: {name: '卧室', message: '今天先照顾身体，其他事情可以缓一缓。', color: '#8c9ba6'},
    work: {name: '办公室', message: '专注处理工作，积累收入。', color: '#7d9ca9'},
    study: {name: '书房', message: '给未来的自己，多学一点东西。', color: '#b5a171'},
    exercise: {name: '户外运动区', message: '走出去，让身体活动起来。', color: '#86a685'},
    family: {name: '客厅', message: '留一点时间，给在意的人。', color: '#be9a85'},
    leisure: {name: '客厅', message: '放慢节奏，享受自己的时间。', color: '#90a696'}
  };
  function furniture(action) {
    if (action === 'sleep' || action === 'recover') return '<rect x="155" y="233" width="275" height="76" rx="14" fill="#d4dcdd"/><rect x="165" y="220" width="70" height="35" rx="12" fill="#f9f7ee"/><path d="M230 242h190v56H230z" fill="#a1b6bf"/><path d="M168 309v20m250-20v20" stroke="#8a9694" stroke-width="9"/>';
    if (action === 'work' || action === 'study') return '<rect x="240" y="240" width="185" height="15" rx="5" fill="#c6ab87"/><path d="M255 255v73m153-73v73" stroke="#aa9070" stroke-width="9"/><rect x="304" y="183" width="85" height="53" rx="5" fill="#697e86"/><rect x="310" y="189" width="73" height="38" rx="2" fill="#d6e8e2"/><path d="M300 237h97" stroke="#5f737b" stroke-width="5"/><rect x="365" y="236" width="37" height="5" fill="#e9d9ad"/><path d="M192 275h55v49" stroke="#99aaa4" stroke-width="13" fill="none"/>';
    if (action === 'exercise') return '<path d="M0 278q110-35 220 0t240 0t210 0v100H0z" fill="#b7c8a6"/><path d="M0 326h640" stroke="#e6d6b4" stroke-width="32"/><path d="M460 265v-73" stroke="#a18d6c" stroke-width="9"/><circle cx="460" cy="164" r="42" fill="#8aa982"/>';
    return '<rect x="193" y="249" width="225" height="63" rx="15" fill="#b7c2ae"/><rect x="206" y="215" width="198" height="70" rx="15" fill="#c9d1bf"/><rect x="183" y="246" width="32" height="72" rx="10" fill="#a5b39d"/><rect x="397" y="246" width="32" height="72" rx="10" fill="#a5b39d"/><path d="M203 317v12m202-12v12" stroke="#8c957e" stroke-width="7"/><ellipse cx="484" cy="294" rx="42" ry="12" fill="#c8ad85"/><path d="M484 303v28" stroke="#ae946e" stroke-width="7"/>';
  }
  function person(gender, action) {
    const female = gender === 'female';
    const hair = female ? '<path d="M-23-71q-9-51 24-49 34 0 24 49l-9 15H-15z" fill="#51463e"/>' : '<path d="M-20-93q-4-28 23-26 26 0 21 26l-11-12-12 8-13-4z" fill="#51463e"/>';
    const legs = action === 'sleep' ? '' : '<g class="legs"><path d="M-9-25l-8 49m27-49 9 49" stroke="#667a87" stroke-width="13" stroke-linecap="round"/><path d="M-24 26h17m22 0h16" stroke="#4c6268" stroke-width="8" stroke-linecap="round"/></g>';
    return `<g class="avatar ${action}" transform="translate(${action === 'sleep' ? '256 253' : action === 'exercise' ? '300 280' : '250 279'})"><g class="body">${hair}<circle cx="1" cy="-94" r="20" fill="#efc6a4"/><path d="M-6-91h1m14 0h1" stroke="#665347" stroke-width="3" stroke-linecap="round"/><path d="M0-82q4 3 8 0" fill="none" stroke="#b0806b" stroke-width="2"/><path d="M-17-70q18-12 36 0l5 47h-46z" fill="${female ? '#b98d87' : '#73988d'}"/>${legs}<g class="arms"><path d="M-16-65l-17 30m50-30 19 29" stroke="#efc6a4" stroke-width="10" stroke-linecap="round"/></g></g></g>`;
  }
  function render(action, gender) {
    const scene = scenes[action];
    if (!scene) throw new Error('Unknown C++ action');
    document.querySelector('#scene-title').textContent = scene.name;
    document.querySelector('#scene-message').textContent = scene.message;
    const svg = document.querySelector('#scene');
    svg.setAttribute('aria-label', `${gender === 'female' ? '女性' : '男性'}角色在${scene.name}，行为：${action}`);
    svg.innerHTML = `<defs><linearGradient id="wall" x2="0" y2="1"><stop stop-color="#f0eee3"/><stop offset="1" stop-color="#e4e7dc"/></linearGradient></defs><rect width="640" height="360" fill="url(#wall)"/><rect y="289" width="640" height="71" fill="#ded9c6"/><rect x="74" y="68" width="105" height="110" rx="7" fill="#fffaf0"/><rect x="81" y="75" width="91" height="96" rx="4" fill="${scene.color}"/><path d="M126 76v94m-44-47h89" stroke="#fffaf0" stroke-width="5"/><circle cx="515" cy="107" r="25" fill="#f9f7ee"/><path d="M515 90v18l12 8" fill="none" stroke="#9ba69b" stroke-width="3"/><rect x="553" y="257" width="33" height="45" rx="6" fill="#bca889"/><path d="M570 258v-46m0 22q-39-18-22-33 17-9 22 27m0-4q28-27 34-9 1 15-34 21" fill="#8fa180"/>${furniture(action)}${person(gender, action)}${action === 'sleep' ? '<text class="zzz" x="275" y="152" fill="#7e94aa" font-size="23">z Z</text>' : ''}${action === 'family' ? '<text class="chat" x="319" y="167" fill="#a38374" font-size="28">♡</text>' : ''}`;
  }
  return {render};
})();
