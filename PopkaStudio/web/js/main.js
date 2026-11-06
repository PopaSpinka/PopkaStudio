// main.js — PopkaStudio's page: the project, the map of screens drawn by the board's code, the panels around it.
import {
  state, on, load, screens, home, selected, select, change, doUndo, doRedo, canUndo, canRedo, resetToDefault, emit,
  sync as saved, activeScreen, layerById, screenOf, KINDS, LISTED, listed, moveInList, refreshWeather,
} from './state.js';
import { engines, runAll } from './engine.js';
import { Stage } from './stage.js';
import { renderPalette, renderFunctions, initFold, renderScreens, renderLayers, renderProps, remove, duplicate, addScreen, toGallery } from './panels.js';
import { initBoard } from './board.js';
import { initLive } from './live.js';
import { initStart, startOpen, thumbSoon } from './start.js';
import { initAbout } from './about.js';
import { initUpdate } from './update.js';

const $ = (id) => document.getElementById(id);
let stage;

async function start() {
  // The start window first, over the editor being built behind it; a project picked there — open, the view fitted
  let built;
  const ready = new Promise((r) => { built = r; });
  initAbout();
  initUpdate();  // (a newer PopkaStudio: the number on the heart)
  initStart((fresh) => {
    if (state.mode === 'try' || (fresh && state.mode !== 'edit')) setMode('edit');  // (a new project: in Edit)
    stage.mapView = null;
    stage.fit();
    thumbSoon(stage, 2500);
  }, ready);
  await load();
  stage = new Stage();
  stage.onEdit = () => setMode('edit');
  stage.onGallery = (kind) => toGallery(stage, kind);  // (a card dropped on a screen that is no gallery)
  await stage.start();
  stage.live.onAssets = () => stage.live.sync(screens());
  on((what) => {
    if (what === 'project') stage.sync(), renderFunctions(), renderProps(stage), history(), refreshWeather();
    if (what === 'weather') stage.sync();  // (the cities' numbers came)
    if (what === 'structure') renderScreens(stage), renderLayers(), renderProps(stage, true);
    if (what === 'select') renderFunctions(), renderScreens(stage), renderLayers(), renderProps(stage), stage.reveal(), stage.ensureTiles();  // (the page picked: its frame, its row)
    if (what === 'screen') stage.screenChanged(), stage.ensureTiles(), renderScreens(stage), renderLayers(), state.mode === 'edit' && stage.focus(activeScreen());
    if (what === 'view') $('zoom-val').textContent = Math.round(stage.zoom() * 100) + '%';
  });
  stage.sync();
  markMode();
  emit('view');  // the zoom kept from the last time — next to its buttons
  renderPalette(stage);
  renderFunctions();
  initFold();
  renderScreens(stage);
  renderLayers();
  renderProps(stage, true);
  history();
  initBoard();
  initLive(() => setMode('edit'));
  document.fonts.ready.then(markMode);  // (the words' widths with the page's font)
  runAll(() => stage.drawOverlay(), () => stage.showEngines());
  status();
  setInterval(status, 400);
  refreshWeather();
  setInterval(refreshWeather, 60000);  // (asks again only every 10 minutes, or when the cities change)
  built();
}

function history() {
  $('undo').disabled = !canUndo();
  $('redo').disabled = !canRedo();
}

let noteUntil = 0;

// The corner at the bottom right: only a short note after something was done (4 s) — no standing line (the mode, the
// screen, how many layers: the user didn't need it, 5 Nov 2026)
function status() {
  if (saved.note) noteUntil = performance.now() + 4000, ($('status').textContent = saved.note), (saved.note = '');
  if (state.mode === 'try') stage.tryName();
  if (performance.now() >= noteUntil && $('status').textContent) $('status').textContent = '';
}

// ---------- Toolbar ----------
// The mode — one slider with three places: Edit (one screen and its layers), Try (the whole watch as on the board, at
// the same place), Map (all screens and the links)
function markMode() {
  const m = state.mode;
  $('mode-slide').className = 'slide at-' + m;
  for (const b of $('mode-slide').querySelectorAll('.slide-opt')) b.setAttribute('aria-checked', String(b.dataset.mode === m));
  // The plate under the one that is on: the options are as wide as their words (Edit has the Live switch in it)
  const on = $('mode-slide').querySelector(`[data-mode="${m}"]`), plate = $('mode-slide').querySelector('.slide-thumb');
  plate.style.setProperty('--x', on.offsetLeft + 'px'), plate.style.setProperty('--w', on.offsetWidth + 'px');
  $('try').hidden = m !== 'try';
  document.body.dataset.mode = m;  // (Try: the cards only their heads)
}

function setMode(m) {
  if (m === state.mode) return;
  if (m === 'try') {
    stage.leaveMap();  // (from Map: the picked screen fitted; Map's view kept)
    stage.lastMode = state.mode;
    state.mode = 'try';
    stage.live.show(true);  // (seen from now: the project first, then the commands below)
    stage.live.sync(screens());
    // Start awake (it went to Always On by itself behind Edit; a lock stays) on the screen being edited, reached as on
    // the board (nothing opens it — the face)
    stage.live.command('wake'), stage.live.skip(700);
    const s = activeScreen();
    stage.inView(s);
    if (!stage.tryGo(s)) saved.note = `Nothing opens ${s.name} yet — Try starts on ${home().name}`;
  } else {
    if (state.mode === 'try') {
      // Back from Try: on the screen that was open on the board (selected — its layers in Layers, ready to edit)
      const s = stage.tryScreen();
      state.mode = stage.lastMode || 'edit';
      if (s.id !== state.screen) select({ type: 'screen', id: s.id });
    }
    stage.setMode(m);
  }
  stage.save();
  markMode();
  stage.placeTry();  // (once the Try box is shown: it is measured)
  emit('mode');
  emit('view');
  status();
}
for (const b of $('mode-slide').querySelectorAll('.slide-opt')) b.onclick = () => setMode(b.dataset.mode);
$('zoom-in').onclick = () => stage.zoomBy(1.25);
$('zoom-out').onclick = () => stage.zoomBy(1 / 1.25);
$('zoom-fit').onclick = () => stage.fit();
$('zoom-val').onclick = () => stage.fit();  // the number itself: fit again
$('undo').onclick = doUndo;
$('redo').onclick = doRedo;
$('add-screen').onclick = () => { const s = addScreen(stage); select({ type: 'screen', id: s.id }); stage.focus(s); };
$('reset').onclick = () => {
  if (confirm('Start again from an empty watch face? (Undo brings your screens back.)')) resetToDefault();
};

// ---------- Keys ----------
document.addEventListener('keydown', (e) => {
  if (e.target.closest('input, select, textarea') || startOpen()) return;
  if (e.code === 'Space') { stage.spaceHeld = true; return; }
  const cmd = e.metaKey || e.ctrlKey;
  if (cmd && e.key.toLowerCase() === 'z') return e.preventDefault(), e.shiftKey ? doRedo() : doUndo();
  if (cmd && e.key.toLowerCase() === 'y') return e.preventDefault(), doRedo();
  if (state.mode === 'try') return;
  const l = selected();
  if (cmd && e.key.toLowerCase() === 'd' && l) return e.preventDefault(), duplicate(l.id);
  if (e.key === 'Escape') return select(null);
  if ((e.key === 'Backspace' || e.key === 'Delete') && l) return e.preventDefault(), remove(l.id);
  if ((e.key === 'Backspace' || e.key === 'Delete') && state.sel?.type === 'link') {
    const l = layerById(state.sel.id);
    return e.preventDefault(), change(() => { l.tap = null; }, { structure: true }), select(null);
  }
  if ((e.key === 'Backspace' || e.key === 'Delete') && state.sel?.type === 'main') {
    return e.preventDefault(), change((p) => { p.main = null; }, { structure: true }), select(null);
  }
  if (e.code === 'Digit1' && e.shiftKey) return stage.fit();
  // Edit, no layer picked: ← → — as the arrows over the screen: its pages (a screen with pages), then back and forward
  if ((e.key === 'ArrowLeft' || e.key === 'ArrowRight') && !l && state.mode === 'edit') {
    return e.preventDefault(), e.key === 'ArrowRight' ? stage.stepRight() : stage.stepLeft();
  }
  if (!cmd && !e.altKey && e.key === '1') return setMode('edit');
  if (!cmd && !e.altKey && e.key === '2') return setMode('map');
  if (!cmd && !e.altKey && e.key === '3') return setMode('try');
  // A title or a card of a list: ↑ ↓ — a place up or down in it (the list places it: it doesn't move by pixels)
  if ((e.key === 'ArrowUp' || e.key === 'ArrowDown') && l && LISTED.has(l.type)) {
    e.preventDefault();
    const k = listed(screenOf(l.id)).indexOf(l);
    if (k >= 0) moveInList(l.id, k + (e.key === 'ArrowUp' ? -1 : 1));
    return;
  }
  const arrows = { ArrowLeft: [-1, 0], ArrowRight: [1, 0], ArrowUp: [0, -1], ArrowDown: [0, 1] };
  if (arrows[e.key] && l && !KINDS[l.type].fixed) {
    e.preventDefault();
    const [x, y] = arrows[e.key], k = e.shiftKey ? 10 : 1;
    change(() => { l.dx += x * k, l.dy += y * k; }, { key: l.id + 'nudge' });
  }
});
document.addEventListener('keyup', (e) => { if (e.code === 'Space') stage.spaceHeld = false; });

window.popka = { state, get stage() { return stage; }, engines };  // for the console and for tests

start().catch((e) => {
  noteUntil = Infinity;
  $('status').textContent = 'Could not start: ' + e.message + ' — is studio.py running?';
  console.error(e);
});
