// update.js — updates (6 Nov 2026, the user's pick: variant 3 of web/mockups/updates.html). An installed PopkaStudio
// asks studio.py once at the start (/api/update — GitHub at most once an hour); newer versions → a blue number on the
// heart: how many. The heart then opens "What's new" instead of About: two columns as a folder's content — the versions
// newer than this one on the left (the newest first, LATEST; this one at the bottom, grey — yours), the picked one's
// notes on the right (its GitHub release notes: the English part, without the install command — Update does that).
// Update — as Load: a fill and the percent (studio.py update_job: git, then install.sh), then Restart PopkaStudio:
// studio.py starts again in its place, the page reloads itself and says to Load (the board's dot is blue then: ours,
// older). About → Check for updates asks GitHub now (about.js). Later, Esc, × or a click beside — closes; an update
// that runs goes on, and the window shows it again when opened.
import { sync as saved } from './state.js';
import { openAbout } from './about.js';

const $ = (id) => document.getElementById(id);
const X = '<svg viewBox="0 0 14 14" aria-hidden="true"><path d="M3 3l8 8M11 3l-8 8"/></svg>';
const COPY = '<svg viewBox="0 0 16 16" aria-hidden="true"><rect x="5.5" y="5.5" width="8" height="8" rx="1.6"/><path d="M10.5 3.5v-.3A1.2 1.2 0 0 0 9.3 2H3.2A1.2 1.2 0 0 0 2 3.2v6.1a1.2 1.2 0 0 0 1.2 1.2h.3"/></svg>';
const MON = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
const day = (d, year = false) => {  // (GitHub's time, UTC → this computer's day: "6 Nov", "6 Nov 2026")
  const t = new Date(d || '');
  return isNaN(t) ? '' : `${t.getDate()} ${MON[t.getMonth()]}${year ? ' ' + t.getFullYear() : ''}`;
};
let info = null, open = null;
// Just restarted by an update (restart() below left a mark): the start window is skipped — the same project opens
// (start.js), and a note says to Load. Read once, as the page loads.
const WAS = (() => {
  try {
    const was = JSON.parse(localStorage.getItem('popka.updated') || 'null');
    localStorage.removeItem('popka.updated');
    return was && Date.now() - (was.at || 0) < 10 * 60000 ? was : null;
  } catch (e) { return null; }
})();
export const justUpdated = () => !!WAS;

// ---------- The release notes (markdown, as written for GitHub) → the window's HTML ----------
const esc = (s) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
const inline = (s) => esc(s)
  .replace(/`([^`]+)`/g, '<code>$1</code>')
  .replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>')
  .replace(/\[([^\]]+)\]\((https?:\/\/[^)\s]+)\)/g, '<a href="$2" target="_blank" rel="noopener">$1</a>');

export function notesHtml(md) {
  const lines = (md || '').replace(/\r/g, '').split('\n');
  const end = lines.findIndex((l) => /^-{3,}\s*$/.test(l));  // (after "---": the same in Russian)
  let out = '', para = [], items = null, code = false, skip = false, first = true;
  const flushPara = () => {
    const t = para.join(' ').trim();
    para = [];
    if (!t || skip) return;
    if (/^\*\*Update/.test(t)) return void (skip = true);  // (how to update by hand: Update does it — up to the next heading)
    out += first ? `<div class="upd-sum">${inline(t)}</div>` : `<p>${inline(t)}</p>`;
    first = false;
  };
  const flushList = () => {
    if (!items) return;
    const li = (it) => `<li>${inline(it.text.join(' '))}${it.kids.length ? `<ul>${it.kids.map(li).join('')}</ul>` : ''}${
      it.after.length ? `<span class="upd-more">${inline(it.after.join(' '))}</span>` : ''}</li>`;
    if (!skip) out += `<ul class="upd-items">${items.map(li).join('')}</ul>`;
    items = null;
  };
  for (const raw of end >= 0 ? lines.slice(0, end) : lines) {
    if (/^\s*```/.test(raw)) { code = !code; continue; }
    if (code) continue;
    const h = raw.match(/^#{1,6}\s+(.*)$/);
    if (h) {
      flushPara(), flushList(), skip = false, first = false;
      out += `<div class="upd-grp">${inline(h[1])}</div>`;
      continue;
    }
    const m = raw.match(/^(\s*)[-*]\s+(.*)$/);
    if (m) {
      flushPara();
      items = items || [];
      const it = { text: [m[2]], kids: [], after: [] };
      if (m[1].length >= 2 && items.length) items[items.length - 1].kids.push(it);
      else items.push(it);
      continue;
    }
    if (!raw.trim()) { flushPara(); continue; }
    if (items && /^\s{2,}\S/.test(raw)) {  // (a list item goes on: its own line, or a paragraph after its sub-list)
      const top = items[items.length - 1], deep = raw.match(/^\s*/)[0].length >= 4 && top.kids.length;
      if (deep) top.kids[top.kids.length - 1].text.push(raw.trim());
      else (top.kids.length ? top.after : top.text).push(raw.trim());
      continue;
    }
    flushList();
    para.push(raw.trim());
  }
  flushPara(), flushList();
  return out;
}

// ---------- The badge on the heart ----------
export const updatesPending = () => !!info?.newer?.length;
function badge() {
  const btn = $('about-btn');
  if (!btn) return;
  let b = btn.querySelector('.upd-badge');
  const n = info?.newer?.length || 0;
  if (!n) return void b?.remove();
  if (!b) b = Object.assign(document.createElement('span'), { className: 'upd-badge' }), btn.append(b);
  b.textContent = n > 9 ? '9+' : String(n);
  btn.title = `PopkaStudio ${info.newer[0].v} is out — what's new`;
}
const listeners = [];
export const onUpdates = (fn) => listeners.push(fn);  // (About's version row)
export const updateInfo = () => info;
export function setUpdates(i) {
  info = i;
  badge();
  for (const fn of listeners) fn(i);
}
// About → Check: GitHub now (studio.py: /api/update?now=1); its answer — the badge, About's row
export async function checkNow() {
  const i = await (await fetch('/api/update?now=1')).json();
  if (!i.error) setUpdates(i);
  return i;
}

// ---------- The window ----------
export function openUpdates(i = info) {
  if (open || !i?.newer?.length) return;
  info = i;
  const newer = i.newer, n = newer.length;
  const ov = document.createElement('div');
  ov.className = 'upd-ov';
  ov.innerHTML = `<section class="upd-win" role="dialog" aria-label="What's new" tabindex="-1">
      <button class="upd-x" aria-label="Close">${X}</button>
      <div class="upd-h"><b>What's new</b><span>${n} update${n > 1 ? 's' : ''} since your ${esc(i.version)}</span></div>
      <div class="upd-two"><div class="upd-vers"></div><div class="upd-notes"></div></div>
      <div class="upd-err" hidden></div>
      <div class="upd-foot"><button class="ghost upd-about">About PopkaStudio</button><span class="upd-step"></span>
        <button class="ghost upd-later">Later</button>
        <button class="primary upd-go"><span class="upd-fill"></span><span class="upd-t"></span></button></div>
    </section>`;
  document.body.append(ov);
  requestAnimationFrame(() => ov.classList.add('on'));
  const q = (s) => ov.querySelector(s);
  const vers = q('.upd-vers'), notes = q('.upd-notes'), go = q('.upd-go'), fill = q('.upd-fill'), t = q('.upd-t'), step = q('.upd-step');
  let timer = 0;

  const pick = (k) => {
    vers.querySelectorAll('.upd-v').forEach((b, j) => b.classList.toggle('on', j === k));
    const r = newer[k];
    notes.innerHTML = `<div class="upd-vh"><b>${esc(r.v)}</b>${k ? '' : '<span class="upd-tag">LATEST</span>'}<span>${day(r.date, true)}</span></div>${notesHtml(r.notes)}`;
    notes.scrollTop = 0;
  };
  vers.innerHTML = newer.map((r, k) => `<button class="upd-v"><b>${esc(r.v)}</b>${k ? '' : '<span class="upd-tag">LATEST</span>'}<span>${day(r.date)}</span></button>`).join('')
    + `<div class="upd-v yours"><b>${esc(i.version)}</b><span>yours</span></div>`;
  vers.querySelectorAll('button.upd-v').forEach((b, k) => b.addEventListener('click', () => pick(k)));
  pick(0);

  const close = () => {
    clearInterval(timer);
    document.removeEventListener('keydown', keys, true);
    ov.classList.remove('on');
    setTimeout(() => ov.remove(), 200);
    open = null;
  };
  const keys = (e) => {
    if (e.key === 'Escape') e.stopPropagation(), e.preventDefault(), close();
    else if (ov.contains(e.target)) e.stopPropagation();  // (the editor's own keys wait)
  };
  document.addEventListener('keydown', keys, true);
  ov.addEventListener('pointerdown', (e) => { if (e.target === ov) close(); });
  q('.upd-x').onclick = q('.upd-later').onclick = close;
  q('.upd-about').onclick = () => { close(), openAbout(); };

  // the button: Update to X → Updating N % → Restart PopkaStudio (or Try again, and the command to do it by hand)
  const show = (s) => {
    go.className = 'primary upd-go' + (s.state === 'running' ? ' running' : s.state === 'done' ? ' ok' : '');
    fill.style.width = s.state === 'running' ? s.pct + '%' : '0';
    t.textContent = s.state === 'running' ? `Updating ${s.pct}%` : s.state === 'done' ? 'Restart PopkaStudio'
      : s.state === 'error' ? 'Try again' : s.state === 'restarting' ? 'Restarting…' : `Update to ${newer[0].v}`;
    go.disabled = s.state === 'running' || s.state === 'restarting';
    step.textContent = s.state === 'running' ? s.step || '' : '';
    const err = q('.upd-err');  // (a row of its own: what went wrong, and the command — a click copies it)
    err.hidden = s.state !== 'error';
    err.replaceChildren();
    if (s.state === 'error') {
      err.append(Object.assign(document.createElement('span'), { textContent: s.error || 'The update stopped' }));
      if (s.command) {
        const c = Object.assign(document.createElement('button'), { className: 'upd-cmd', title: 'Copy — then paste it into Terminal' });
        c.innerHTML = `<code></code>${COPY}`;
        c.querySelector('code').textContent = s.command;
        c.onclick = () => navigator.clipboard?.writeText(s.command).then(() => c.classList.add('copied'));
        err.append(c);
      }
    }
    q('.upd-about').hidden = s.state === 'running' || s.state === 'error';
    q('.upd-later').hidden = s.state === 'restarting';
  };
  const follow = () => {
    clearInterval(timer);
    timer = setInterval(async () => {
      try {
        const s = await (await fetch('/api/update?status=1')).json();
        show(s);
        if (s.state !== 'running') clearInterval(timer);
        if (s.state === 'done') go.dataset.to = s.to || newer[0].v;
      } catch (e) { /* studio.py busy: next time */ }
    }, 400);
  };
  go.onclick = async () => {
    if (go.classList.contains('ok')) return restart(go.dataset.to);
    show({ state: 'running', pct: 0, step: 'Starting' });
    const r = await fetch('/api/update', { method: 'POST', body: '{"go":true}' }).catch(() => null);
    const j = r ? await r.json().catch(() => ({})) : { error: 'PopkaStudio is not running' };
    if (!r || (!r.ok && r.status !== 409)) return show({ state: 'error', error: j.error || 'The update stopped', command: j.command });
    follow();
  };
  const restart = async (to) => {
    show({ state: 'restarting' });
    localStorage.setItem('popka.updated', JSON.stringify({ to, at: Date.now() }));
    await fetch('/api/restart', { method: 'POST', body: '{}' }).catch(() => null);
    const t0 = Date.now();
    await new Promise((r) => setTimeout(r, 900));
    while (Date.now() - t0 < 60000) {  // (studio.py starts again: the new version answers — the page with it)
      try {
        const a = await (await fetch('/api/about', { cache: 'no-store' })).json();
        if (!to || a.version === to) return location.reload();
      } catch (e) { /* not up yet */ }
      await new Promise((r) => setTimeout(r, 500));
    }
    location.reload();
  };

  // not installed (the developer's folder): no Update, only what's new
  if (!i.installed) go.hidden = true, q('.upd-later').textContent = 'Close';
  show({ state: 'idle' });
  // an update already going (the window was closed while it ran) or done: show it
  fetch('/api/update?status=1').then((r) => r.json()).then((s) => {
    if (s.state === 'running') show(s), follow();
    else if (s.state === 'done') show(s), go.dataset.to = s.to || newer[0].v;
  }).catch(() => {});
  open = { close };
  q('.upd-win').focus({ preventScroll: true });
}

// ---------- At the start: ask once (studio.py asks GitHub at most once an hour, and only from an installed copy) ----------
export function initUpdate() {
  fetch('/api/update').then((r) => r.json()).then((i) => {
    setUpdates(i);
    if (WAS && (!WAS.to || WAS.to === i.version))  // (once the editor shows the project again)
      setTimeout(() => { saved.note = `Updated to ${i.version} — Load to bring it to the watch`; }, 2500);
  }).catch(() => {});
}
