// board.js — which board, and Load: studio.py makes the PopkaWatch sketch from the project, compiles it and uploads
// it (tools/flash.sh). The top bar ends with the board's icon (a click — which board and port) and Load, which shows its
// own progress: it fills with blue from the left, a spinner and the percent; then green "Done" (back to "Load" in 4 s)
// or red "Failed" (the reason — its tip; a click — again). Its width never changes: "⚡ Load" and 12 px at each side
// (fixWidth); the other states sit in its middle.
import { state } from './state.js';

const $ = (id) => document.getElementById(id);
let ports = [], boards = [];
let phase = 'idle', idleAt = 0;  // 'idle' | 'running' | 'done' | 'error'

async function refresh() {
  try {
    const r = await (await fetch('/api/boards', { cache: 'no-store' })).json();
    const model = $('board-model');
    if (!model.options.length) boards = r.boards.filter((b) => !b.soon), model.replaceChildren(...boards.map((b) => new Option(b.short || b.name, b.id)));
    const sel = $('board-port'), was = sel.value;
    const names = r.ports.map((p) => p.port).join();
    if (names !== ports.map((p) => p.port).join()) {
      ports = r.ports;
      sel.replaceChildren(...(ports.length ? ports.map((p) => new Option(p.name, p.port)) : [new Option('not connected', '')]));
      if (ports.some((p) => p.port === was)) sel.value = was;
    }
  } catch (e) {
    ports = [];
  }
  mark();
}

// The tips; Load — enabled when there is a board and nothing runs
function mark() {
  const b = boards.find((q) => q.id === $('board-model').value), port = $('board-port').selectedOptions[0]?.text;
  $('board-btn').title = (b ? b.short || b.name : 'Board') + (ports.length ? ` · ${port}` : ' · not connected');
  if (phase === 'running') return;
  $('flash').disabled = !ports.length;
  if (phase === 'idle') $('flash').title = ports.length ? 'Load the watch onto the board' : 'Connect the board by USB';
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

async function load() {
  clearTimeout(idleAt);
  show('running', 0, 'Starting');
  const r = await fetch('/api/flash', {
    method: 'POST', body: JSON.stringify({ project: state.project, port: $('board-port').value }),
  }).catch(() => null);
  if (!r || !r.ok) {
    const e = r ? await r.json().catch(() => ({})) : {};
    return finish('error', e.error || 'Could not start');
  }
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
    finish(j.state, j.step.replace(/^Failed: /, ''));
  };
  poll();
}

function finish(kind, tip) {
  show(kind, 100, tip);
  if (kind === 'done') idleAt = setTimeout(() => show('idle'), 4000);
}

// A popover under its icon: one open at a time; a click anywhere else or Esc closes it
function toggle(pop) {
  const open = pop.hidden;
  for (const p of document.querySelectorAll('.hdr-pop')) p.hidden = true;
  pop.hidden = !open;
}

export function initBoard() {
  $('flash').onclick = () => (phase !== 'running' ? load() : null);
  $('board-btn').onclick = () => toggle($('board-pop'));
  $('board-model').onchange = $('board-port').onchange = mark;
  document.addEventListener('pointerdown', (e) => {
    if (!e.target.closest('.hdr-pick')) $('board-pop').hidden = true;
  });
  document.addEventListener('keydown', (e) => { if (e.key === 'Escape') $('board-pop').hidden = true; });
  show('idle');
  document.fonts.ready.then(fixWidth);
  refresh();
  setInterval(refresh, 2500);
}
