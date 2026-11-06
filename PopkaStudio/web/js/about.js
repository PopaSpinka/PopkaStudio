// about.js — the About window (variant 4 of web/mockups/about2.html, 6 Nov 2026): the heart at the top right, left of
// the board. No × and no Close: Esc or a click beside the window closes it. Tabs About · Credits (the track of the
// start window slides). About: a small watch with the name, who made PopkaStudio and with whom, the links, the version's
// row — the update's state (update.js) with its button: Check / What's new / Try again, a bar while an update runs —
// the contact, and a support card: subscribe on YouTube (?sub_confirmation=1 — YouTube asks "Subscribe?" at once).
// A newer version out: the heart opens "What's new" instead (About is a link there).
import { updatesPending, openUpdates, updateInfo, checkNow, onUpdates } from './update.js';

const $ = (id) => document.getElementById(id);
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
let openIt = () => {};
export const openAbout = () => openIt();

export function initAbout() {
  const box = $('about'), win = $('about-win'), st = $('about-st'), act = $('about-act');
  const shown = () => box.classList.contains('on');
  let version = '', phase = '', run = null, timer = 0;  // (phase: '' — as updateInfo says; 'checking', 'error')

  // the version's row: what update.js knows (checked: up to date / newer out), or what goes on now
  const row = () => {
    const i = updateInfo(), v = esc(i?.version || version);
    let cls = '', html = `<b>${v}</b>`, btn = 'Check', blue = false;
    if (run?.state === 'running') {
      cls = 'run', html = `<span class="ab-bar"><i style="width:${run.pct}%"></i></span><span class="dim">Updating ${run.pct}%</span>`, btn = '';
    } else if (phase === 'checking') {
      html = `<i class="ab-spin"></i><b>${v}</b><span class="dim">· Checking…</span>`, btn = '';
    } else if (i?.newer?.length) {
      cls = 'new', html = `<i class="ab-dot"></i><b>${esc(i.newer[0].v)} is out</b><span class="dim">· you have ${v}</span>`;
      btn = run?.state === 'done' ? 'Restart' : "What's new", blue = true;
    } else if (phase === 'error') {
      cls = 'error', html = `<i class="ab-dot"></i>Can't reach GitHub`, btn = 'Try again';
    } else if (i?.latest) {
      cls = 'ok', html = `<i class="ab-dot"></i><b>${v}</b><span class="dim">· Up to date</span>`;
    }
    st.className = 'ab-st ' + cls, st.innerHTML = html;
    act.hidden = !btn, act.textContent = btn, act.classList.toggle('blue', blue);
  };
  const follow = async () => {  // (an update going on: its percent here while About is open)
    clearTimeout(timer);
    try { run = await (await fetch('/api/update?status=1')).json(); } catch (e) { run = null; }
    row();
    if (shown() && run?.state === 'running') timer = setTimeout(follow, 500);
  };
  act.addEventListener('click', async () => {
    if (updateInfo()?.newer?.length) return close(), openUpdates();  // (What's new / Restart: that window does it)
    phase = 'checking', row();
    try {
      const i = await checkNow();
      phase = '';
      if (i.error) phase = 'error';
    } catch (e) { phase = 'error'; }
    row();
  });
  onUpdates(row);

  const tab = (n) => {
    win.dataset.step = String(n);
    $('about-tab1').classList.toggle('on', n === 1), $('about-tab2').classList.toggle('on', n === 2);
  };
  const open = () => {
    win.classList.add('instant'), tab(1);
    requestAnimationFrame(() => win.classList.remove('instant'));
    box.classList.add('on'), box.setAttribute('aria-hidden', 'false');
    phase = phase === 'checking' ? phase : '', row(), follow();
    win.focus({ preventScroll: true });
  };
  const close = () => { clearTimeout(timer), box.classList.remove('on'), box.setAttribute('aria-hidden', 'true'); };
  openIt = open;
  win.tabIndex = -1;
  $('about-btn').addEventListener('click', () => (updatesPending() ? openUpdates() : open()));
  $('about-tab1').addEventListener('click', () => tab(1));
  $('about-tab2').addEventListener('click', () => tab(2));
  box.addEventListener('pointerdown', (e) => { if (e.target === box) close(); });  // (a click beside the window)
  addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && shown()) e.stopPropagation(), close();
  }, true);
  fetch('/api/about').then((r) => r.json()).then((a) => { version = a.version, $('about-ver').textContent = 'v' + a.version, row(); }).catch(() => {});
}
