// video.js — the Video app's clips (Properties of its screen): what has been converted on this computer and what is on the
// card. + — a video from the computer: studio.py turns it into a .p4v (ffmpeg) and writes it to the card, right into a card
// in the Mac's reader or else through the board over USB (slow: 0.2 MB/s). The arrow writes one again, the bin forgets it.
// Stream — the clips of videos/stream go to the board over Wi-Fi while its switch is on (stream.py, boards/p4lcd7/p4-stream.md):
// the board finds the studio by itself and shows them in Video under Stream. Its + makes a lighter clip (~0.7 MB/s).
const UP = '<svg viewBox="0 0 16 16"><path d="M8 11V3M4.8 6.2 8 3l3.2 3.2M3 13h10"/></svg>';

let list = null, cardNames = null, macCard = null, ffmpeg = true, busy = null, target = null, h0 = null, icons0 = null, loading = false;
const fmt = (s) => `${Math.floor(s / 60)}:${String(Math.round(s) % 60).padStart(2, '0')}`;
const port = () => document.getElementById('board-port')?.value || '';
const post = (url, body) => fetch(url, { method: 'POST', body: JSON.stringify(body || {}) });
// Somewhere to write a clip: the board's card in this Mac's reader (no board needed), or the board on its port
const canWrite = () => !!macCard || !!port();

async function refresh() {
  if (loading) return;
  loading = true;
  try {
    const r = await (await fetch('/api/videos')).json();
    const was = JSON.stringify([list, macCard, ffmpeg]);
    list = r.clips, ffmpeg = r.ffmpeg, macCard = r.card;
    if (JSON.stringify([list, macCard, ffmpeg]) !== was) draw();  // (looked at every 3 s: the pictures don't blink)
    if (port() && !cardNames) {
      const c = await (await post('/api/video/card', { port: port() })).json().catch(() => ({}));
      if (c.names) cardNames = c.names, draw();
    }
  } finally {
    loading = false;
  }
}

// Follows the studio's one job (a conversion or a write) in its row; done(job) after
function follow(name, label, done) {
  busy = { name, label, pct: 0 };
  draw();
  const poll = async () => {
    const j = await (await fetch('/api/job?from=99999')).json().catch(() => null);
    if (!j || j.state === 'running') {
      if (j) busy.pct = Math.round(j.progress || 0), draw();
      return setTimeout(poll, 500);
    }
    busy = null;
    done(j);
  };
  setTimeout(poll, 400);
}

function send(name) {
  if (!canWrite()) return;
  post('/api/video/send', { name, port: port() }).then(async (r) => {
    if (!r.ok) return fail(name, await r.json().catch(() => ({})));
    follow(name, 'Writing', (j) => {
      if (j.state === 'done' && !macCard) cardNames = [...(cardNames || []), name];
      else fail(name, { error: j.step });
      refresh();
    });
  });
}
const fail = (name, e) => { problem = { name, text: (e.error || 'Failed').replace(/^Failed: /, '') }; draw(); };
let problem = null;

function add(file) {
  problem = null;
  busy = { name: file.name, label: 'Uploading', pct: 0 };
  draw();
  fetch(`/api/video/add?name=${encodeURIComponent(file.name)}`, { method: 'POST', body: file }).then(async (r) => {
    const a = await r.json().catch(() => ({}));
    if (!r.ok) return (busy = null), fail(file.name, a);
    follow(a.name, 'Converting', (j) => {
      if (j.state !== 'done') return fail(file.name, { error: j.step }), refresh();
      refresh().then(() => send(a.name));
    });
  });
}

function draw() {
  if (!target) return;
  const h = h0, rows = [];
  const items = [...(list || [])];
  if (busy && !items.some((c) => c.name === busy.name)) items.unshift({ name: busy.name, title: busy.name, seconds: 0, pending: true });
  for (const c of items) {
    const on = (macCard || cardNames)?.includes(c.name), here = busy?.name === c.name, bad = problem?.name === c.name;
    const tag = here ? `${busy.label} ${busy.pct}%` : bad ? problem.text : on ? `${fmt(c.seconds)} · on the card` : fmt(c.seconds);
    const up = !here && !on && !c.pending && canWrite() ? h('button', { class: 'icon-btn', title: 'Write to the card', 'aria-label': 'Write to the card', html: UP, onclick: () => send(c.name) }) : null;
    const bin = !here && !c.pending ? h('button', { class: 'icon-btn', title: 'Remove', 'aria-label': 'Remove', html: icons0.trash, onclick: () => post('/api/video/delete', { name: c.name, port: port() }).then(() => { cardNames = cardNames?.filter((n) => n !== c.name) ?? null; refresh(); }) }) : null;
    rows.push(h('li', { class: 'layer clip' + (bad ? ' bad' : '') },
      c.pending ? h('span', { class: 'clip-pic' }) : h('img', { class: 'clip-pic', src: `/api/video/poster?name=${encodeURIComponent(c.name)}`, alt: '' }),
      h('span', { class: 'layer-name' }, c.title), h('span', { class: 'tag', title: tag }, tag), h('span', { class: 'acts' }, up, bin)));
  }
  target.replaceChildren(...rows);
}

export function clipsGroup(h, icons) {
  h0 = h, icons0 = icons;
  const input = h('input', { type: 'file', accept: 'video/*', hidden: true, onchange: () => { if (input.files[0]) add(input.files[0]); input.value = ''; } });
  const plus = h('button', { class: 'icon-btn', title: ffmpeg ? 'Add a video' : 'Needs ffmpeg: brew install ffmpeg', 'aria-label': 'Add a video', html: icons.plus, onclick: () => input.click() });
  target = h('ul', { class: 'layers clips' });
  draw();
  refresh();
  // A card put into the Mac's reader or taken out — seen within a few seconds, while the list is on screen
  const list0 = target, tick = () => { if (!list0.isConnected || target !== list0) return; if (!busy) refresh(); setTimeout(tick, 3000); };
  setTimeout(tick, 3000);
  return h('div', { class: 'group' }, h('h3', {}, 'Clips', plus), target, input);
}

// ---------- Stream: the switch, and the clips it gives the board ----------
let sState = null, sTarget = null, sBusy = null, sProblem = null, sLoading = false;

async function sRefresh() {
  if (sLoading) return;
  sLoading = true;
  try {
    const r = await (await fetch('/api/stream')).json();
    const was = JSON.stringify(sState && { on: sState.on, clips: sState.clips });
    sState = r;
    if (JSON.stringify({ on: r.on, clips: r.clips }) !== was) sDraw();
  } finally {
    sLoading = false;
  }
}

function sSwitch(on) {
  sProblem = null;
  post('/api/stream', { on }).then(async (r) => {
    const a = await r.json().catch(() => ({}));
    if (!r.ok) sProblem = a.error || 'Failed';
    else sState = { ...sState, ...a };
    sDraw();
  });
}

function sAdd(file) {
  sProblem = null;
  sBusy = { name: file.name, label: 'Uploading', pct: 0 };
  sDraw();
  fetch(`/api/video/add?stream=1&name=${encodeURIComponent(file.name)}`, { method: 'POST', body: file }).then(async (r) => {
    const a = await r.json().catch(() => ({}));
    if (!r.ok) return (sBusy = null), (sProblem = a.error || 'Failed'), sDraw();
    sBusy = { name: a.name, label: 'Converting', pct: 0 };
    const poll = async () => {
      const j = await (await fetch('/api/job?from=99999')).json().catch(() => null);
      if (!j || j.state === 'running') {
        if (j && sBusy) sBusy.pct = Math.round(j.progress || 0), sDraw();
        return setTimeout(poll, 500);
      }
      if (j.state !== 'done') sProblem = j.step;
      sBusy = null;
      sRefresh().then(sDraw);
    };
    setTimeout(poll, 400);
  });
}

function sDraw() {
  if (!sTarget) return;
  const h = h0, rows = [];
  const items = [...(sState?.clips || [])];
  if (sBusy && !items.some((c) => c.name === sBusy.name)) items.unshift({ name: sBusy.name, title: sBusy.name, seconds: 0, pending: true });
  for (const c of items) {
    const here = sBusy?.name === c.name;
    const tag = here ? `${sBusy.label} ${sBusy.pct}%` : `${fmt(c.seconds)} · ${c.mb_s.toFixed(1)} MB/s`;
    const bin = !here && !c.pending ? h('button', { class: 'icon-btn', title: 'Remove', 'aria-label': 'Remove', html: icons0.trash, onclick: () => post('/api/stream', { delete: c.name }).then(sRefresh) }) : null;
    rows.push(h('li', { class: 'layer clip' },
      c.pending ? h('span', { class: 'clip-pic' }) : h('img', { class: 'clip-pic', src: `/api/video/poster?stream=1&name=${encodeURIComponent(c.name)}`, alt: '' }),
      h('span', { class: 'layer-name' }, c.title), h('span', { class: 'tag', title: tag }, tag), h('span', { class: 'acts' }, bin)));
  }
  if (sProblem) rows.push(h('li', { class: 'layer clip bad' }, h('span', { class: 'layer-name' }, sProblem)));
  sTarget.replaceChildren(...rows);
  const sw = sTarget.parentNode?.querySelector('.switch');
  if (sw) sw.classList.toggle('on', !!sState?.on), sw.setAttribute('aria-checked', String(!!sState?.on));
}

export function streamGroup(h, icons) {
  h0 = h, icons0 = icons;
  const input = h('input', { type: 'file', accept: 'video/*', hidden: true, onchange: () => { if (input.files[0]) sAdd(input.files[0]); input.value = ''; } });
  const plus = h('button', { class: 'icon-btn', title: 'Add a video to stream', 'aria-label': 'Add a video to stream', html: icons.plus, onclick: () => input.click() });
  const sw = h('button', { class: 'switch' + (sState?.on ? ' on' : ''), role: 'switch', 'aria-checked': String(!!sState?.on), 'aria-label': 'Stream', onclick: () => sSwitch(!sState?.on) });
  sTarget = h('ul', { class: 'layers clips' });
  sDraw();
  sRefresh();
  const list0 = sTarget, tick = () => { if (!list0.isConnected || sTarget !== list0) return; if (!sBusy) sRefresh(); setTimeout(tick, 3000); };
  setTimeout(tick, 3000);
  return h('div', { class: 'group' }, h('h3', {}, 'Stream', h('span', { class: 'h3-acts' }, sw, plus)), sTarget, input);
}
