// live.js — Live: the small switch with its word inside Edit. On — the project goes to studio.py as it changes (a request at a time,
// the newest project in it) and from there onto the board over USB (studio.py LiveLink → the board's live.h): what is
// moved, turned or recolored here changes on the board at once, no Load. What the board can't show without a Load (a
// new layer, another font or text, a link) — named in the status line; the switch: green — live, amber — some of it
// needs a Load, gray — no board or a firmware without Live. Off when the page opens; studio.py turns it off when the page is
// gone (no word from it for 6 s).
// Locked (6 Nov 2026, docs/board-detect.md step 1): the board runs other firmware, or ours made from another project
// (its hello: board.js) — the switch looks off and dim; a click asks under it: Load this project? Yes — Load, and Live
// turns on by itself once it is on the board.
import { state, on, sync as saved } from './state.js';
import { boardState, ask, loadNow, loadOrSave } from './board.js';

const $ = (id) => document.getElementById(id);
let live = false, busy = false, again = false, shown = '', afterLoad = false, onEditFn = null;
let st = { state: 'off', needs: [] };

const TIPS = {
  off: 'Live: what you change here changes on the board at once, no Load',
  connecting: 'Live: finding the board…',
  live: 'Live: on the board',
  noboard: 'Live: no board — connect it by USB',
  old: 'Live: Load once — the board\'s firmware has no Live yet',
  other: 'Live: the board runs other firmware — Load PopkaStudio first',
  project: 'Live: the board has another project — Load this one first',
};
// Locked: what is on the board can't take this project's Live (other firmware, or ours from another project)
function locked() {
  const b = boardState();
  return b.state === 'other' ? 'other' : b.state === 'project' && b.project ? 'project' : '';
}

// What a list of names reads as: "Clock", "Clock and Date", "Clock, Date and 3 more"
function names(list) {
  if (list.length <= 2) return list.join(' and ');
  return list.length === 3 ? `${list[0]}, ${list[1]} and ${list[2]}` : `${list[0]}, ${list[1]} and ${list.length - 2} more`;
}

function mark() {
  const box = $('live-box');
  const s = !live ? 'off' : st.state === 'live' && st.needs.length ? 'needs' : st.state;
  const lock = !live && locked();
  box.dataset.state = lock ? 'locked' : s;
  box.setAttribute('aria-checked', String(live));
  box.title = lock ? TIPS[lock] : s === 'needs' ? `Live: Load to see ${names(st.needs)}` : TIPS[s] || TIPS.off;
  // A word in the status line when it says something new
  const now = live ? `${s}|${st.needs.join(',')}` : '';
  if (now !== shown && live) {
    if (s === 'live') saved.note = 'Live: on the board — what you change here changes there';
    else if (s !== 'connecting') saved.note = box.title;
  }
  shown = now;
}

// One request at a time: a change while one is on its way — sent after it, with the project as it is then
async function send(withProject) {
  if (busy) { again = again || withProject; return; }
  busy = true;
  try {
    const body = { on: live, port: $('board-port')?.value || '' };
    if (live && withProject) body.project = state.project;
    const r = await fetch('/api/live', { method: 'POST', body: JSON.stringify(body) });
    st = await r.json();
  } catch (e) {
    st = { state: 'noboard', needs: [] };
  }
  busy = false;
  mark();
  if (again && live) { again = false; send(true); }
}

function toggle(onEdit, loaded = false) {
  const lock = !live && !loaded && locked();  // (loaded — this project just went onto it: its hello comes a moment later)
  if (lock) {  // (other firmware, or another project: Load first — asked under the switch)
    const bar = $('live-box').closest('.bar'), r = $('live-box').getBoundingClientRect(), br = bar.getBoundingClientRect();
    const pop = ask(bar, lock === 'other' ? 'The board runs other firmware. Load this project onto it — then Live works.'
      : 'The board has another project on it. Load this one — then Live works.', 'Load',
    (save) => { afterLoad = true; lock === 'other' ? loadOrSave(save) : loadNow(); }, lock === 'other' ? { box: 'Save its firmware first' } : {});
    pop.style.left = `${Math.round(r.left - br.left + r.width / 2 - 140)}px`;
    return;
  }
  live = !live;
  shown = '';
  if (live && state.mode !== 'edit') onEdit();
  if (!live) saved.note = 'Live: off — the board keeps what it shows until it restarts (Load keeps it for good)';
  mark();
  send(true);
}

export function initLive(onEdit) {
  onEditFn = onEdit;
  const box = $('live-box');
  box.addEventListener('click', (e) => { e.stopPropagation(); toggle(onEdit); });
  box.addEventListener('keydown', (e) => {
    if (e.key === ' ' || e.key === 'Enter') e.preventDefault(), e.stopPropagation(), toggle(onEdit);
  });
  on((what) => {
    if (live && what === 'project') send(true);
    if (what === 'board') mark();  // (the board came, went, or was asked who it is: locked or not)
    if (what === 'loaded' && afterLoad && !live) afterLoad = false, toggle(onEditFn, true);  // (Loaded from its question: on)
    if (what === 'load-failed') afterLoad = false;
  });
  setInterval(() => { if (live) send(false); }, 1500);  // (the board came or went, a Load done; "the page is here")
  mark();
}
