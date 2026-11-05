// live.js — Live: the small switch with its word inside Edit. On — the project goes to studio.py as it changes (a request at a time,
// the newest project in it) and from there onto the board over USB (studio.py LiveLink → the board's live.h): what is
// moved, turned or recolored here changes on the board at once, no Load. What the board can't show without a Load (a
// new layer, another font or text, a link) — named in the status line; the switch: green — live, amber — some of it
// needs a Load, gray — no board or a firmware without Live. Off when the page opens; studio.py turns it off when the page is
// gone (no word from it for 6 s).
import { state, on, sync as saved } from './state.js';

const $ = (id) => document.getElementById(id);
let live = false, busy = false, again = false, shown = '';
let st = { state: 'off', needs: [] };

const TIPS = {
  off: 'Live: what you change here changes on the board at once, no Load',
  connecting: 'Live: finding the board…',
  live: 'Live: on the board',
  noboard: 'Live: no board — connect it by USB',
  old: 'Live: Load once — the board\'s firmware has no Live yet',
};

// What a list of names reads as: "Clock", "Clock and Date", "Clock, Date and 3 more"
function names(list) {
  if (list.length <= 2) return list.join(' and ');
  return list.length === 3 ? `${list[0]}, ${list[1]} and ${list[2]}` : `${list[0]}, ${list[1]} and ${list.length - 2} more`;
}

function mark() {
  const box = $('live-box');
  const s = !live ? 'off' : st.state === 'live' && st.needs.length ? 'needs' : st.state;
  box.dataset.state = s;
  box.setAttribute('aria-checked', String(live));
  box.title = s === 'needs' ? `Live: Load to see ${names(st.needs)}` : TIPS[s] || TIPS.off;
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

function toggle(onEdit) {
  live = !live;
  shown = '';
  if (live && state.mode !== 'edit') onEdit();
  if (!live) saved.note = 'Live: off — the board keeps what it shows until it restarts (Load keeps it for good)';
  mark();
  send(true);
}

export function initLive(onEdit) {
  const box = $('live-box');
  box.addEventListener('click', (e) => { e.stopPropagation(); toggle(onEdit); });
  box.addEventListener('keydown', (e) => {
    if (e.key === ' ' || e.key === 'Enter') e.preventDefault(), e.stopPropagation(), toggle(onEdit);
  });
  on((what) => { if (live && what === 'project') send(true); });
  setInterval(() => { if (live) send(false); }, 1500);  // (the board came or went, a Load done; "the page is here")
  mark();
}
