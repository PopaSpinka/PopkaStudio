// start.js — the start window, over the editor (blurred behind it), in steps that slide: 1 the board (Choose a board —
// its photo, name and screen; a board not ready yet is shown as Soon), 2 its projects — a new one, or one of them to
// open, rename or delete — and 3 a new project's name. It shows when the page starts; again from the top left:
// PopkaStudio — at the boards, the project's name — at the projects (opened right there, no slide). A project's circle
// is its watch face: the picture the page makes a while after each save (thumbSoon), kept next to the project by
// studio.py.
import { state, on, openProject, closeProject, change, projectName, home, screens, newLayer, FACE_SCREEN } from './state.js';
import { engines } from './engine.js';
import { preview } from './previews.js';
import { justUpdated } from './update.js';

const $ = (id) => document.getElementById(id);
let boards = [], projects = [], board = 'amoled175', opened = false, onOpen = null, ready = null;

const ICON = {
  check: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="m5 12.5 4.5 4.5L19 7.5"/></svg>',
  plus: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 5v14M5 12h14"/></svg>',
  trash: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M5 7h14M10 7V5h4v2M7 7l1 12h8l1-12"/></svg>',
  pen: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M5 19h3.5L18.5 9 15 5.5 5 15.5zM13 7.5 16.5 11"/></svg>',
};

// The board's picture: its photo; without one, drawn — the round one: a metal ring with its two buttons (PWR above the
// middle, BOOT below) round the screen, and on the screen HaloWatch's face drawn by the board's own code (previews.js);
// one not ready — a screen
function picture(b, cls = '') {
  if (b.photo) return Object.assign(new Image(), { src: b.photo, alt: '', className: 'bphoto ' + cls, draggable: false });
  if (b.drawn !== 'round') return Object.assign(document.createElement('div'), { className: 'drawn ' + cls });
  const el = document.createElement('div');
  el.className = 'rboard ' + cls;
  el.innerHTML = '<i class="rb-btn pwr"></i><i class="rb-btn boot"></i><span class="rb-scr"></span>';
  preview('board:face', () => {
    const s = { id: 'pv', layers: [] };
    for (const kind of FACE_SCREEN) s.layers.push(newLayer(kind, null, s));
    return s.layers;
  }, true, 320).then((url) => {
    if (url) el.querySelector('.rb-scr').append(Object.assign(new Image(), { src: url, alt: '', draggable: false }));
  });
  return el;
}

const thumbUrl = (p) => `/api/thumb?p=${p.id}&v=${p.thumb}`;
const mine = () => projects.filter((p) => p.board === board);
const current = () => boards.find((b) => b.id === board);

// When a project was edited: Now, 5 min ago, 3 h ago, Yesterday, 2 Nov
function when(t) {
  const s = Date.now() / 1000 - t, d = new Date(t * 1000), today = new Date();
  if (s < 60) return 'Now';
  if (s < 3600) return `${Math.floor(s / 60)} min ago`;
  if (d.toDateString() === today.toDateString()) return `${Math.floor(s / 3600)} h ago`;
  if (new Date(today - 864e5).toDateString() === d.toDateString()) return 'Yesterday';
  return d.toLocaleDateString('en-GB', { day: 'numeric', month: 'short', ...(d.getFullYear() !== today.getFullYear() && { year: 'numeric' }) });
}

function names(b) {
  const t = document.createElement('span');
  t.className = 'names';
  t.innerHTML = '<span class="maker"></span><span class="bname"></span><span class="spec"></span>';
  t.children[0].textContent = (b.maker || '').toUpperCase();
  t.children[1].textContent = b.title || b.name;
  t.children[2].textContent = b.spec || b.screen || '';
  return t;
}

// ---------- Step 1: the board ----------
function renderBoards() {
  $('start-boards').replaceChildren(...boards.map((b) => {
    const el = document.createElement('button');
    el.className = 'bcard' + (b.id === board ? ' picked' : '') + (b.soon ? ' off' : '');
    el.disabled = !!b.soon;
    if (b.soon) el.insertAdjacentHTML('beforeend', '<span class="soon">SOON</span>');
    else if (b.id === board) el.insertAdjacentHTML('beforeend', `<span class="check">${ICON.check}</span>`);
    el.append(picture(b), names(b));
    el.onclick = () => { if (board !== b.id) board = b.id, renderBoards(); };
    el.ondblclick = () => go(2);
    return el;
  }));
  $('start-next').disabled = !!current()?.soon;
}

// ---------- Step 2: its projects ----------
function renderProjects() {
  const b = current();
  if (b) $('start-board').replaceChildren(picture(b, 'mini'), names(b));
  const add = document.createElement('button');
  add.className = 'ptile new';
  add.innerHTML = `<span class="th plus">${ICON.plus}</span><span class="nm">New</span><span class="when">&nbsp;</span>`;
  add.onclick = () => go(3);
  $('start-grid').replaceChildren(add, ...mine().map(tile));
}

function tile(p) {
  const el = document.createElement('div');
  el.className = 'ptile' + (opened && p.id === state.pid ? ' cur' : '');
  el.tabIndex = 0;
  el.innerHTML = `<span class="ring">${p.thumb ? `<img class="th" alt="" draggable="false" src="${thumbUrl(p)}">` : '<span class="th ini"></span>'}
    <button class="pen" title="Rename" aria-label="Rename">${ICON.pen}</button>
    <button class="trash" title="Delete" aria-label="Delete">${ICON.trash}</button></span>
    <span class="nm"></span><span class="when">${when(p.edited)}</span>`;
  const nm = el.querySelector('.nm');
  nm.textContent = p.name;
  const ini = el.querySelector('.ini');  // (no picture yet — made when it is opened: its first letter)
  if (ini) ini.textContent = p.name.slice(0, 1).toUpperCase();
  el.onclick = (e) => { if (!e.target.closest('.trash, .pen, input')) pick(p.id); };
  el.onkeydown = (e) => { if (e.key === 'Enter' && e.target === el) pick(p.id); };
  // A new name: the pen, or a double click on the name (Enter or leaving it keeps it, Esc — the old one)
  nm.onclick = (e) => e.stopPropagation();
  nm.ondblclick = el.querySelector('.pen').onclick = (e) => {
    e.stopPropagation();
    if (el.querySelector('.nm-edit')) return;
    const input = Object.assign(document.createElement('input'), { value: p.name, maxLength: 40, className: 'nm-edit' });
    nm.replaceWith(input);
    input.focus(), input.select();
    let done = false;
    const finish = (keep) => {
      if (done) return;
      done = true;
      const name = input.value.trim().replace(/\s+/g, ' ');
      if (keep && name && name !== p.name) rename(p, name);
      else input.replaceWith(nm);
    };
    input.onkeydown = (k) => { k.stopPropagation(); if (k.key === 'Enter') finish(true); if (k.key === 'Escape') finish(false); };
    input.onblur = () => finish(true);
  };
  // Delete: a click makes the bin red, a second one deletes (to projects/.trash); away from the circle — nothing
  const bin = el.querySelector('.trash');
  bin.onclick = (e) => {
    e.stopPropagation();
    if (!bin.classList.contains('armed')) return bin.classList.add('armed'), (bin.title = 'Delete — click again');
    remove(p);
  };
  el.onmouseleave = () => { bin.classList.remove('armed'), (bin.title = 'Delete'); };
  return el;
}

// The step shown: slid to (instant — put there at once: the window opening)
function go(n, instant = false) {
  if (n > 1 && current()?.soon) return;
  if (n === 2) renderProjects();
  const win = $('start-win');
  if (instant) win.classList.add('instant');
  win.dataset.step = n;
  if (instant) win.getBoundingClientRect(), win.classList.remove('instant');
  if (n === 3) {  // the name: the first free Project N, picked so typing replaces it
    const taken = new Set(projects.map((p) => p.name));
    let k = 1;
    while (taken.has(`Project ${k}`)) k++;
    const input = $('start-name');
    input.value = `Project ${k}`;
    input.focus({ preventScroll: true }), input.select();
  }
}
const step = () => Number($('start-win').dataset.step);

// ---------- What the buttons do (studio.py: /api/projects) ----------
async function api(body) {
  const r = await fetch('/api/projects', { method: 'POST', body: JSON.stringify(body) });
  const j = await r.json();
  if (!r.ok) throw new Error(j.error || r.status);
  projects = j.projects;
  return j;
}

async function refresh() {
  try {
    if (!boards.length) boards = (await (await fetch('/api/boards', { cache: 'no-store' })).json()).boards;
    projects = (await (await fetch('/api/projects', { cache: 'no-store' })).json()).projects;
  } catch (e) { /* studio.py is not running: what was there stays */ }
  const cur = projects.find((p) => p.id === state.pid);
  if (cur && boards.some((b) => b.id === cur.board && !b.soon)) board = cur.board;
  renderBoards();
  renderProjects();
}

async function create() {
  const name = $('start-name').value.trim().replace(/\s+/g, ' ');
  if (!name) return $('start-name').focus();
  const { id } = await api({ board, name });
  await pick(id, true);
}

async function rename(p, name) {
  if (opened && p.id === state.pid) {  // the open one: through the editor, so its next save keeps the name
    change((q) => { q.name = name; });
    p.name = name;
  } else {
    await api({ rename: p.id, name });
  }
  renderProjects();
  showName();
}

// The open one deleted: the newest other one opens; none left — no project is open, the list stays with New alone
// (before 6 Nov 2026 a new one was made at once: it took the free name "Project 1" and the same id — the project looked
// as if it had not gone)
async function remove(p) {
  await api({ delete: p.id });
  if (p.id === state.pid) {
    if (projects[0]) {
      await openProject(projects[0].id);
      if (opened) onOpen?.();
    } else closeProject(), (opened = false);
  }
  renderProjects();
}

// fresh — a project just made: the editor opens it in Edit (its watch face in the middle), whatever mode was on
async function pick(pid, fresh = false) {
  await ready;  // (the editor still being built behind: the project it loads first, then this one)
  const other = pid !== state.pid;  // (asked before opening it: then state.pid is this one)
  if (other && !(await openProject(pid))) return refresh();
  if (other || !opened || fresh) onOpen?.(fresh);
  opened = true;
  showName();
  hideStart();
}

// n — the step it opens at (1 the boards, 2 the projects)
export function showStart(n = 1) {
  go(n, true);
  $('start').classList.add('on');
  $('start').setAttribute('aria-hidden', 'false');
  refresh();
}

export const startOpen = () => $('start').classList.contains('on');

export function hideStart() {
  if (!opened) return;  // (at the start: a project has to be picked)
  $('start').classList.remove('on');
  $('start').setAttribute('aria-hidden', 'true');
}

// The open project's name at the top left, after PopkaStudio
function showName() {
  if (state.project) $('proj-name').textContent = projectName();
}

// open — what the editor does once a project is open (the view fitted to it); built — the editor is ready
export function initStart(open, built) {
  onOpen = open, ready = built;
  $('brand').onclick = () => showStart(1);
  $('proj-name').onclick = () => showStart(2);
  $('start-next').onclick = () => go(2);
  $('start-back').onclick = () => go(1);
  $('start-back2').onclick = () => go(2);
  $('start-create').onclick = create;
  $('start-name').onkeydown = (e) => {
    if (e.key === 'Enter') e.preventDefault(), create();
    if (e.key === 'Escape') e.preventDefault(), e.stopPropagation(), go(2);
  };
  $('start').addEventListener('pointerdown', (e) => { if (e.target === $('start')) hideStart(); });
  document.addEventListener('keydown', (e) => {  // (the editor's own keys wait while it is open: startOpen)
    if (!startOpen() || e.target.closest?.('input')) return;
    if (e.key === 'Escape') step() === 3 ? go(2) : hideStart();
    if (e.key === 'Enter' && step() === 1 && !e.target.closest?.('button, .ptile')) go(2);
  });
  if (justUpdated()) ready.then(() => (state.pid ? pick(state.pid) : showStart()));  // (back from an update: the same project)
  else showStart();
}

// ---------- The project's picture: its watch face, a while after a save (once its fonts are all there) ----------
let thumbTimer = 0;
export function thumbSoon(stage, ms = 2000, tries = 10) {
  clearTimeout(thumbTimer);
  thumbTimer = setTimeout(() => {
    const t = stage.tiles.get(home().id), pid = state.pid;
    if (t && t.engine.M && !t.engine.shown) t.engine.drawNow();  // (the face out of sight: its picture made now)
    if (!t || !t.engine.M || [...engines].some((e) => e.busy)) return tries > 0 && thumbSoon(stage, 1000, tries - 1);
    const c = document.createElement('canvas');
    c.width = c.height = 280;
    const g = c.getContext('2d');
    g.imageSmoothingQuality = 'high';
    g.drawImage(t.canvas, 0, 0, 280, 280);
    c.toBlob((png) => {
      if (png && pid === state.pid && screens().length) fetch(`/api/thumb?p=${pid}`, { method: 'PUT', body: png }).catch(() => {});
    }, 'image/png');
  }, ms);
}
on((what) => {
  if (what === 'gone') opened = false, showStart(2);  // (the open project deleted in another window: the projects)
  if (what === 'saved' && window.popka?.stage) thumbSoon(window.popka.stage);
  if (what === 'project') showName();  // (opened, renamed here or in another window)
});
