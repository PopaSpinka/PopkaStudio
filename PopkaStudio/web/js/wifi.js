// wifi.js — the Wi-Fi window (6 Nov 2026, the user's flow). Wi-Fi is a part like any other: a new project has it off,
// and the watch's time comes from the computer (studio.py sends it after a Load). Only a choice that needs Wi-Fi opens
// it, never by itself (adding a face or the Weather app with Wi-Fi off asks nothing — the user's word): "Wi-Fi" for a
// clock's time, a weather city (its pen or + — dim while Wi-Fi is off; panels.js needWifi). Over the editor: the networks the board sees, the
// strongest first (studio.py asks it over USB — "wifi scan"; every firmware of ours can, even one without the Wi-Fi
// function: wifiscan.h; macOS hides the names from programs, so the Mac can't), ↻ — look again; a click on one — its
// password right under it (a saved one, or an open one — none needed); Other network… — its name and password typed.
// Save — the network is kept on this Mac only (studio.py: secrets/wifi.json; into the firmware at a Load) and the
// Wi-Fi function goes on: the next Load brings the time from the internet, and the weather. Cancel, Esc or a click
// beside it — nothing changes.
import { setFn, fnOf, sync as saved } from './state.js';

const SIG = (rssi) => {
  const n = rssi == null ? 3 : rssi >= -60 ? 3 : rssi >= -75 ? 2 : 1;
  return `<svg class="sig" viewBox="0 0 16 16"><path${n < 3 ? ' class="off"' : ''} d="M1.6 6.2a9.2 9.2 0 0 1 12.8 0"/><path${n < 2 ? ' class="off"' : ''} d="M3.7 8.4a6.2 6.2 0 0 1 8.6 0"/><path d="M5.8 10.6a3.2 3.2 0 0 1 4.4 0"/><circle cx="8" cy="12.9" r=".6"/></svg>`;
};
const LOCK = '<svg viewBox="0 0 16 16"><rect x="3.5" y="7" width="9" height="6.5" rx="1.5"/><path d="M5.5 7V5a2.5 2.5 0 0 1 5 0v2"/></svg>';
const AGAIN = '<svg viewBox="0 0 16 16"><path d="M13 8a5 5 0 1 1-1.6-3.7"/><path d="M13 2.5v3h-3"/></svg>';
const OTHER = '\u0000other';  // (the picked row: Other network…)
let open = null;

// The window: resolves true — saved (Wi-Fi on), false — nothing changed
export function openWifi() {
  if (open) return open.promise;
  let resolve;
  const promise = new Promise((r) => { resolve = r; });
  const ov = document.createElement('div');
  ov.className = 'wifi-ov';
  ov.innerHTML = `<section class="wifi-win" role="dialog" aria-label="Wi-Fi">
      <div class="wifi-head"><b>Wi-Fi</b><button class="icon-btn wifi-again" title="Look again" aria-label="Look again">${AGAIN}</button></div>
      <div class="wifi-list"></div>
      <div class="wifi-foot"><button class="ghost wifi-cancel">Cancel</button><button class="primary wifi-save" disabled>Save</button></div>
    </section>`;
  document.body.append(ov);
  requestAnimationFrame(() => ov.classList.add('on'));
  const win = ov.querySelector('.wifi-win'), list = ov.querySelector('.wifi-list'), save = ov.querySelector('.wifi-save');
  const w = { nets: [], saved: [], current: '', pick: '', pass: '', name: '', note: '', busy: false };
  open = { promise };

  const close = (ok) => {
    document.removeEventListener('keydown', keys, true);
    ov.classList.remove('on');
    setTimeout(() => ov.remove(), 200);
    open = null;
    resolve(ok);
  };
  const keys = (e) => {
    if (e.key === 'Escape') e.stopPropagation(), e.preventDefault(), close(false);
    else if (e.key === 'Enter' && !save.disabled) e.preventDefault(), save.click();
    if (ov.contains(e.target)) e.stopPropagation();  // (the editor's own keys wait)
  };
  document.addEventListener('keydown', keys, true);
  ov.addEventListener('pointerdown', (e) => { if (e.target === ov) close(false); });
  ov.querySelector('.wifi-cancel').onclick = () => close(false);

  const known = (ssid) => w.saved.includes(ssid) || ssid === w.current;
  const netOf = (ssid) => w.nets.find((n) => n.ssid === ssid);
  const ready = () => {
    if (w.pick === OTHER) return w.name.trim() !== '';
    const n = netOf(w.pick) || (w.pick && { lock: true });
    return !!w.pick && (known(w.pick) || !n.lock || w.pass.length >= 8);
  };
  const field = (cls, type, value, ph, set) => {
    const i = document.createElement('input');
    Object.assign(i, { className: 'wifi-in ' + cls, type, value, placeholder: ph, spellcheck: false, autocomplete: 'off' });
    i.oninput = () => { set(i.value); save.disabled = !ready(); };
    return i;
  };
  const render = () => {
    const rows = [];
    const shown = [...w.nets];
    if (w.current && !netOf(w.current)) shown.unshift({ ssid: w.current, rssi: null, lock: true, away: true });  // (saved, not seen now)
    for (const n of shown) {
      const r = document.createElement('button');
      r.className = 'wifi-row' + (w.pick === n.ssid ? ' on' : '');
      r.innerHTML = `${SIG(n.rssi)}<span class="wifi-name"></span>${known(n.ssid) ? '<span class="wifi-tag">saved</span>' : ''}${n.lock ? `<span class="wifi-lock">${LOCK}</span>` : ''}`;
      r.querySelector('.wifi-name').textContent = n.ssid;
      r.onclick = () => { if (w.pick !== n.ssid) w.pick = n.ssid, w.pass = ''; render(); };
      rows.push(r);
      if (w.pick === n.ssid && n.lock && !known(n.ssid)) {
        const i = field('', 'password', w.pass, 'Password', (v) => { w.pass = v; });
        rows.push(i);
        requestAnimationFrame(() => i.focus());
      }
    }
    if (w.busy) rows.push(Object.assign(document.createElement('div'), { className: 'wifi-note', textContent: 'Looking for networks…' }));
    else if (w.note) rows.push(Object.assign(document.createElement('div'), { className: 'wifi-note', textContent: w.note }));
    const o = document.createElement('button');
    o.className = 'wifi-row other' + (w.pick === OTHER ? ' on' : '');
    o.textContent = 'Other network…';
    o.onclick = () => { if (w.pick !== OTHER) w.pick = OTHER, w.pass = ''; render(); };
    rows.push(o);
    if (w.pick === OTHER) {
      const nm = field('', 'text', w.name, 'Network name', (v) => { w.name = v; });
      rows.push(nm, field('', 'password', w.pass, 'Password', (v) => { w.pass = v; }));
      requestAnimationFrame(() => (w.name ? null : nm.focus()));
    }
    list.replaceChildren(...rows);
    save.disabled = !ready();
  };

  const scan = async () => {
    w.busy = true, w.note = '';
    render();
    try {
      const r = await fetch('/api/wifi', { method: 'POST', body: JSON.stringify({ scan: true }) });
      const j = await r.json();
      w.saved = j.saved || [], w.current = j.current || '';
      if (r.ok) w.nets = j.networks || [];
      else w.note = j.error === 'No board' ? 'Connect the board to see the networks it finds' : 'The board can\'t look for networks yet — Load once, or type the name';
      if (r.ok && !w.nets.length) w.note = 'No networks found';
    } catch (e) {
      w.note = 'PopkaStudio is not running';
    }
    w.busy = false;
    if (!w.pick && w.current) w.pick = w.current;  // (the saved one, ready to Save)
    render();
  };
  ov.querySelector('.wifi-again').onclick = scan;

  save.onclick = async () => {
    const ssid = w.pick === OTHER ? w.name.trim() : w.pick;
    const body = { ssid };
    if (w.pick === OTHER || !known(ssid)) body.password = w.pass;  // (a saved one keeps its password)
    save.disabled = true;
    const r = await fetch('/api/wifi', { method: 'POST', body: JSON.stringify(body) }).catch(() => null);
    if (!r || !r.ok) {
      w.note = 'Could not save it';
      return render();
    }
    setFn('wifi', { on: true }, { structure: true });
    saved.note = `Wi-Fi: ${ssid} — Load to bring the time and the weather from the internet`;
    close(true);
  };

  // what is known at once (no wait for the scan): the saved networks
  fetch('/api/wifi').then((r) => r.json()).then((j) => {
    w.saved = j.saved || [], w.current = j.current || '';
    if (!w.pick && w.current) w.pick = w.current;
    render();
  }).catch(() => {});
  render();
  scan();
  win.focus();
  return promise;
}

// A choice that needs Wi-Fi: Wi-Fi on — go(); off — the window first, go() once it is saved
export function needWifi(go) {
  if (fnOf('wifi').on) return go();
  openWifi().then((ok) => ok && go());
}
