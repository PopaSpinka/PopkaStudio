// board.js — which board, and Load: studio.py makes the PopkaWatch sketch from the project, compiles it and uploads
// it (tools/flash.sh). The right card's head: the board — its icon, its short name and a dot (a click — which board
// and port), and Load, which shows its own progress: it fills with blue from the left, a spinner and the percent; then
// green "Done" (back to "Load" in 4 s) or red "Failed" (the reason — its tip; a click — again). Its width never changes:
// "⚡ Load" and 12 px at each side (fixWidth); the other states sit in its middle.
//
// Who is on the port (6 Nov 2026, docs/board-detect.md step 1): studio.py asks each board that appears "hello" (hello.h)
// and remembers boards by their MAC. The dot: green — our firmware, made from this project; blue — ours, but another
// project (or made before hello); amber — other firmware (or none); gray — no board, or still asking. The model is
// picked by itself — what the board said, or what it was last time. A board with other firmware, seen for the first
// time, is offered a Load once — and again if it was ours and has other firmware again (under the head; popka.offered:
// MAC → when it was shown, against the board's "since" from studio.py).
//
// The board's check (step 2): the board menu's "Check the board" — esptool reads the chip, its memory, its flash and
// the firmware that boots (the board restarts for a few seconds; nothing is written): rows under the board and the
// port, kept with the board (the next time shown at once). A firmware not ours — "Save its firmware": a copy into
// PopkaStudio/backups (only what it needs: ~11 KB/s over the S3's USB). The first Load onto other firmware can save it
// first (a box in its question). A Load that couldn't reach the board's loader — the BOOT help: hold BOOT, plug in.
import { BOARD } from './screen.js';
import { state, emit } from './state.js';

const $ = (id) => document.getElementById(id);
let ports = [], boards = [];
let phase = 'idle', idleAt = 0;  // 'idle' | 'running' | 'done' | 'error'
const picked = new Set();  // the boards whose model was picked by itself once (the user may change it after)
// The boards offered a Load: MAC → when (seconds) — again once it was ours and has other firmware again (its "since")
const offered = (() => {
  try {
    const v = JSON.parse(localStorage.getItem('popka.offered')) || {};
    return Array.isArray(v) ? Object.fromEntries(v.map((m) => [m, 0])) : v;  // (before: a list of MACs)
  } catch { return {}; }
})();
let shown = '';

async function refresh() {
  try {
    const r = await (await fetch('/api/boards', { cache: 'no-store' })).json();
    const model = $('board-model');
    if (!model.options.length) boards = r.boards.filter((b) => b.id === BOARD.id), model.replaceChildren(...boards.map((b) => new Option(b.short || b.name, b.id)));
    const sel = $('board-port'), was = sel.value;
    const names = r.ports.map((p) => p.port).join();
    if (names !== ports.map((p) => p.port).join()) {
      sel.replaceChildren(...(r.ports.length ? r.ports.map((p) => new Option(p.name, p.port)) : [new Option('not connected', '')]));
      if (r.ports.some((p) => p.port === was)) sel.value = was;
    }
    ports = r.ports;
  } catch (e) {
    ports = [];
  }
  const p = current();
  // the model by itself — once for each board (what it said, or what it was last time)
  const key = p && (p.mac || p.port);
  if (checked && p && checked.mac !== (p.mac || '').toUpperCase()) checked = null, saved = '';  // (another board)
  if (p?.model && !picked.has(key) && boards.some((b) => b.id === p.model)) picked.add(key), ($('board-model').value = p.model);
  mark();
}

const current = () => ports.find((p) => p.port === $('board-port').value) || ports[0] || null;

// What is on the board, for Live and the dot: none | wait | ours (this project) | project (ours, another one or made
// before hello) | other
export function boardState() {
  const p = current();
  if (!p) return { state: 'none' };
  if (p.state === undefined) return { state: 'legacy', port: p.port };  // (an older studio.py: it doesn't ask — as before)
  const s = p.state === 'ours' ? (p.project && p.project === state.pid ? 'ours' : 'project')
    : p.state === 'old' ? 'project' : p.state === 'other' ? 'other' : 'wait';
  return { state: s, port: p.port, mac: p.mac, project: p.project || '', fw: p.fw || '', loaded: p.loaded || '' };
}

// "2026-11-06T12:03" → "6 Nov 12:03"
function when(t) {
  const d = t && new Date(t);
  return d && !isNaN(d) ? `${d.getDate()} ${d.toLocaleString('en', { month: 'short' })} ${d.toTimeString().slice(0, 5)}` : '';
}

// The tips, the dot; Load — enabled when there is a board and nothing runs
function mark() {
  const b = boards.find((q) => q.id === $('board-model').value), port = $('board-port').selectedOptions[0]?.text;
  const st = boardState();
  const what = {
    none: 'not connected', wait: 'checking what is on it…',
    ours: `PopkaStudio ${st.fw} · this project${st.loaded ? `, loaded ${when(st.loaded)}` : ''}`,
    project: st.project ? `PopkaStudio ${st.fw} · ${st.project}${st.loaded ? `, loaded ${when(st.loaded)}` : ''} — Load this one for Live`
      : 'PopkaStudio, an older firmware',
    other: 'other firmware — Load PopkaStudio onto it to use Live',
    legacy: 'connected',
  }[st.state];
  $('board-btn').title = (b ? b.name : 'Board') + (ports.length ? ` · ${port} · ${what}` : ' · not connected');
  $('board-name').textContent = b ? b.short || b.name : 'Board';
  const btn = $('board-btn');
  btn.classList.toggle('on', st.state !== 'none');
  for (const k of ['ours', 'project', 'other', 'wait']) btn.classList.toggle('st-' + k, st.state === k);
  const now = `${st.state}|${st.project}|${st.port}`;
  if (now !== shown) shown = now, emit('board');  // (Live: locked or not)
  if (st.state === 'other' && st.mac && (offered[st.mac] ?? -1) < (current()?.since || 0) && phase !== 'running') offer(st.mac);
  if (phase === 'running') return;
  $('flash').disabled = !ports.length || BOARD.load === false;
  if (phase === 'idle') $('flash').title = ports.length ? 'Load the watch onto the board' : 'Connect the board by USB';
}

// A small question under its button: a line and two buttons (Live's too: live.js)
export function ask(where, text, yes, onYes, { box = '', pic = '' } = {}) {
  closeAsk();
  const pop = document.createElement('div');
  pop.className = 'ask-pop';
  pop.innerHTML = `${pic}<p></p>${box ? '<label class="ask-box"><input type="checkbox"><span></span></label>' : ''}
    <div class="ask-btns"><button class="ghost">Not now</button><button class="primary"></button></div>`;
  pop.querySelector('p').textContent = text;
  if (box) pop.querySelector('.ask-box span').textContent = box;
  pop.querySelector('.primary').textContent = yes;
  pop.querySelector('.ghost').onclick = closeAsk;
  pop.querySelector('.primary').onclick = () => {
    const checked = !!pop.querySelector('.ask-box input')?.checked;
    closeAsk();
    onYes(checked);
  };
  where.append(pop);
  return pop;
}
export function closeAsk() {
  for (const p of document.querySelectorAll('.ask-pop')) p.remove();
}

// A new board with other firmware: once — Load PopkaStudio onto it?
function offer(mac) {
  // (not now — Try, the start window, another question open: asked at the next look, kept only once it was shown)
  if (document.body.dataset.mode === 'try' || document.querySelector('.start.on, .ask-pop')) return;
  offered[mac] = Math.floor(Date.now() / 1000);
  localStorage.setItem('popka.offered', JSON.stringify(offered));
  ask($('board-btn').closest('.card-head'), 'This board runs other firmware. Load PopkaStudio onto it — then Live works too.', 'Load',
    loadOrSave, { box: 'Save its firmware first' });
}

// The board's two buttons, its BOOT lit: the help when a Load or the check couldn't reach the board's loader
const BOOT_PIC = `<svg class="boot-pic" viewBox="0 0 240 92" aria-hidden="true">
  <circle cx="58" cy="46" r="38" fill="#000" stroke="#3b3c42" stroke-width="5"/>
  <rect x="94" y="24" width="9" height="15" rx="3" fill="#3b3c42"/><text x="112" y="36" class="dim">PWR</text>
  <rect x="94" y="52" width="9" height="15" rx="3" fill="#4c8dff"/><text x="112" y="64" class="on">BOOT — hold</text>
  <path d="M58 84v8" stroke="#6c6d74" stroke-width="4" stroke-linecap="round"/></svg>`;
function bootHelp() {
  ask($('board-btn').closest('.card-head'), 'The board didn\'t switch to flashing. Hold BOOT, unplug the cable and plug it back in, ' +
    'let go of BOOT — then Load again.', 'Load again', () => loadNow(), { pic: BOOT_PIC });
}

// One width for good: idle's "⚡ Load" and 12 px at each side (measured once the font is in)
function fixWidth() {
  if (phase === 'idle') $('flash').style.width = Math.round($('load-in').getBoundingClientRect().width) + 24 + 'px';
}

// idle — the bolt, "Load"; running — the fill, the spinner, the percent; done — "Done"; error — "Failed"
function show(kind, progress = 0, tip = '') {
  phase = kind;
  const btn = $('flash');
  btn.className = 'sq-btn ' + kind;
  $('load-txt').textContent = { idle: 'Load', running: `${Math.round(progress)}%`, done: 'Done', error: 'Failed' }[kind];
  $('load-fill').style.width = progress + '%';
  btn.title = tip;
  mark();
}

// Load: the project, and who it is (its id and revision: the board's hello says them after)
export async function loadNow() {
  if (phase === 'running' || !ports.length) return;
  clearTimeout(idleAt);
  show('running', 0, 'Starting');
  const r = await fetch('/api/flash', {
    method: 'POST', body: JSON.stringify({ project: state.project, port: $('board-port').value, pid: state.pid, rev: state.rev }),
  }).catch(() => null);
  if (!r || !r.ok) {
    const e = r ? await r.json().catch(() => ({})) : {};
    return finish('error', e.error || 'Could not start');
  }
  follow((j) => finish(j.state, j.step.replace(/^Failed: /, ''), j.hint));
}

// The job on the board (a Load or a copy) followed: its progress in the Load button, then done(job)
function follow(done) {
  let from = 0;
  const poll = async () => {
    let j;
    try {
      j = await (await fetch(`/api/job?from=${from}`, { cache: 'no-store' })).json();
    } catch (e) {
      return finish('error', 'PopkaStudio is not running');
    }
    from = j.lines;
    if (j.state === 'running') return show('running', j.progress, j.step), setTimeout(poll, 400);
    done(j);
  };
  poll();
}

function finish(kind, tip, hint = '') {
  show(kind, 100, tip);
  if (kind === 'done') idleAt = setTimeout(() => show('idle'), 4000);
  emit(kind === 'done' ? 'loaded' : hint === 'boot' ? 'load-stuck' : 'load-failed');  // (stuck: Load again comes — Live waits for it)
  if (hint === 'boot') bootHelp();
}

// A copy of the firmware on the board (backups/): its progress in the Load button; then (Load) — a Load
async function saveFirmware(thenLoad = false) {
  if (phase === 'running' || !ports.length) return;
  clearTimeout(idleAt);
  show('running', 0, 'Saving the firmware on the board');
  const r = await fetch('/api/backup', { method: 'POST', body: JSON.stringify({ port: $('board-port').value }) }).catch(() => null);
  if (!r || !r.ok) return finish('error', 'Could not save its firmware');
  follow((j) => {
    if (j.state !== 'done') return finish('error', j.step.replace(/^Failed: /, ''), j.hint);
    saved = j.result;
    showCheck();
    if (thenLoad) phase = 'idle', loadNow();
    else finish('done', `Saved in ${j.result}`);
  });
}
// The Load the first one asks for: its firmware saved first, or not
export function loadOrSave(save) {
  return save ? saveFirmware(true) : loadNow();
}

// ---------- The board's check: its rows in the board menu ----------
let checking = false, checked = null, saved = '';
const MB = (n) => `${n} MB`;
function showCheck() {
  const box = $('check-rows'), p = current(), c = checked || p?.check;
  $('check-btn').disabled = checking || !p || phase === 'running';
  $('check-btn').textContent = checking ? 'Checking… the board restarts' : 'Check the board';
  if (!c) return box.replaceChildren();
  const st = boardState().state, fw = c.firmware;
  const what = st === 'ours' || st === 'project' ? 'PopkaStudio' + (p?.fw ? ` ${p.fw}` : '')
    : c.empty ? 'none' : fw ? [fw.family === 'Arduino' ? 'An Arduino sketch' : fw.project || 'Unnamed', fw.version && fw.family !== 'Arduino' ? fw.version : '',
      fw.idf ? `(IDF ${fw.idf.replace(/^v/, '').split('-')[0]})` : ''].filter(Boolean).join(' ') : 'unknown';
  const fits = (c.fits || []).map((id) => boards.find((b) => b.id === id)?.short).filter(Boolean);
  const rows = [['Chip', `${c.chip}${c.rev ? ` · ${c.rev}` : ''}`], ['Memory', `PSRAM ${c.psram ? MB(c.psram) : '—'} · flash ${MB(c.flash)}`],
    ['Firmware', what], ['Fits', fits.length ? fits.join(', ') : 'none of these boards']];
  if (saved) rows.push(['Saved', saved]);
  box.replaceChildren(...rows.map(([k, v]) => {
    const r = document.createElement('div');
    r.className = 'check-row';
    r.innerHTML = '<span></span><span></span>';
    r.firstChild.textContent = k, r.lastChild.textContent = v;
    return r;
  }));
  if (st === 'other' && !c.empty && !saved) {  // (not ours: a copy of it can be kept)
    const b = document.createElement('button');
    b.className = 'ghost small save-fw';
    const est = c.copy ? ` · ${(c.copy.bytes / 1048576).toFixed(1)} MB, ~${c.copy.seconds < 90 ? `${c.copy.seconds} s` : `${Math.round(c.copy.seconds / 60)} min`}` : '';
    b.textContent = 'Save its firmware' + est;  // (how much and how long: ~40 KB/s)
    b.disabled = phase === 'running';
    b.onclick = () => { $('board-pop').hidden = true; saveFirmware(false); };
    box.append(b);
  }
}
async function check() {
  if (checking || phase === 'running') return;
  checking = true, showCheck();
  try {
    const r = await fetch('/api/check', { method: 'POST', body: JSON.stringify({ port: $('board-port').value }) });
    const j = await r.json();
    if (r.ok) checked = j;
    else if (j.hint === 'boot') $('board-pop').hidden = true, bootHelp();
    else $('check-btn').title = j.error || 'Could not check it';
  } catch (e) { /* studio.py is gone */ }
  checking = false, showCheck();
}

// A popover under its icon: one open at a time; a click anywhere else or Esc closes it
function toggle(pop) {
  const open = pop.hidden;
  for (const p of document.querySelectorAll('.hdr-pop')) p.hidden = true;
  pop.hidden = !open;
}

export function initBoard() {
  if (BOARD.shape !== 'round') $('board-btn').querySelector('svg').innerHTML = '<rect x="2" y="4" width="12" height="8" rx="1.6"/>';
  $('flash').onclick = () => (phase !== 'running' ? loadNow() : null);
  $('board-btn').onclick = () => { toggle($('board-pop')); showCheck(); };
  $('check-btn').onclick = check;
  $('board-model').onchange = $('board-port').onchange = mark;
  document.addEventListener('pointerdown', (e) => {
    if (!e.target.closest('.hdr-pick')) $('board-pop').hidden = true;
    if (!e.target.closest('.ask-pop, #live-box')) closeAsk();
  });
  document.addEventListener('keydown', (e) => { if (e.key === 'Escape') $('board-pop').hidden = true, closeAsk(); });
  show('idle');
  document.fonts.ready.then(fixWidth);
  refresh();
  setInterval(refresh, 2500);
}
