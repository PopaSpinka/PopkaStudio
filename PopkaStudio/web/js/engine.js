// engine.js — the board's own code in the page: firmware/PopkaWatch built for the browser (engine/build.sh).
//
// What a screen shows is what the board would show, pixel for pixel: the same face.h, the same lib/Halo drawing,
// frames made only where something changed, into a copy of the panel's memory, 60 times a second. The page hands it
// the screens (studio_set) and the fonts and icons their layers need — made by studio.py with tools/halo_assets.py,
// exactly as for the board, and loaded once per size (studio_font, studio_icons).
//
// The map has one copy of the engine per screen, pinned to it (studio_pin: the screen at rest, as the navigation shows
// it when its transition is over) — and a screen with pages (the Weather screen's cities) one per page (studio_page). Try has one more, not pinned: it runs the sketch as the board does — the face, a tap
// on a layer with a link, HaloWatch's transition, back with a swipe up from the bottom edge; the gallery (a swipe up on
// the face), its cards, the Android 16 transition, back with a swipe from the left edge.
import { BOARD, TW, TH, ROUND, lacks } from './screen.js';
import { state, KIND_INDEX, FX, KINDS, ICONS, TAPS, fnOf, timeOf, zoneShift, citiesOf, weatherOf, CLOCK_LOOKS, clockLook } from './state.js';

const PERIOD = 1000 / 60;
// Pictures put on the page since it opened (all copies): the overlay draws again only after one (a layer's box may have
// moved) or a change of its own (stage.js: drawOverlay)
export let paints = 0;
const SPARKLES = { off: 0, around: 1, screen: 2 };  // what is around a pixel art picture (pixelart.h: p2)

// ---------- Fonts and icons from studio.py, shared by all copies ----------
const blobs = new Map();  // key -> Promise<Uint8Array>
function fetchBlob(kind, key, body) {
  if (!blobs.has(key)) {
    blobs.set(key, fetch(`/api/${kind}`, { method: 'POST', body: JSON.stringify(body) })
      .then((r) => (r.ok ? r.arrayBuffer() : Promise.reject(Object.assign(new Error(`${kind}: ${r.status}`), { status: r.status }))))
      .then((b) => new Uint8Array(b))
      .catch((e) => { blobs.delete(key); throw e; }));
  }
  return blobs.get(key);
}
// A layer's font (null — it has none: the icon, the dots, the Steps chart — its letters are the board's own) and icons
// (the face's pill, the big icon, the hourly pill's)
const PILLS = new Set(['weather', 'hourly', 'steps']), WX_ICONS = new Set(['weather', 'wxicon', 'hourly']);
const NO_FONT = new Set(['wxicon', 'dots', 'parrot', 'stchart', 'stepring', 'batring', 'dotclock']);
// A clock of sclock.h: font n of its look (studio.py SC_LOOKS), at its size
const scKey = (l, n) => (clockLook(l)[4] > n ? `font${n + 1}|sclock|${l.look}|${l.scale}` : null);
export const fontKey = (l) => (l.type === 'sclock' ? scKey(l, 0) : NO_FONT.has(l.type) || KINDS[l.type].fixed ? null
  : PILLS.has(l.type) ? `font|${l.type}|${l.scale}` : `font|${l.type}|${l.size}|${l.weight}|${l.width}`);
const font2Key = (l) => (l.type === 'sclock' ? scKey(l, 1) : l.type === 'steps' ? `font2|steps|${l.scale}`  // the Steps pill's shoe, the info line's
  : l.type === 'infoline' && l.format === 'icons' ? `font2|infoline|${l.size}`
  : !ROUND && (l.type === 'weekarc' || l.type === 'forecast') ? `font2|${l.type}|${l.size}` : null);
const iconsKey = (l) => ((WX_ICONS.has(l.type) && (ROUND || l.type !== 'hourly')) || l.type === 'forecast' ? `icons|${l.type}|${l.scale}`
  : l.type === 'infoline' && l.format === 'icons' ? `icons|infoline|${l.size}` : null);
// What a layer's p2 and p3 are (studio.py writes the same into layers.h)
const CURVE_I = ['weekday', 'shortdate', 'daymonth', 'weather', 'steps', 'datesteps', 'battery', 'city'], INFO_I = ['words', 'short', 'icons'];
const DEV_I = ['cpu', 'mem', 'bat', 'temp', 'snd', 'mot'];  // About (device.h: DV_CPU …)
const P2 = { curve: (l) => CURVE_I.indexOf(l.what), stepring: (l) => l.radius, infoline: (l) => INFO_I.indexOf(l.format), card: (l) => (l.live ? 1 : 0),
  devcard: (l) => DEV_I.indexOf(l.what), device: (l) => DEV_I.indexOf(l.what),
  shade: (l) => Math.max(0, ['full', 'dual', 'corner', 'strip', 'wear'].indexOf(l.look)),  // (shade_rect.h: its look; the round board's one — shade_round.h)
  sclock: (l) => Math.max(0, CLOCK_LOOKS.findIndex(([k]) => k === l.look)) };  // (sclock.h: its look)
const P3 = { curve: (l) => l.angle, weekarc: (l) => l.angle ?? 0, forecast: (l) => l.angle ?? 0 };

// The network the board's Wi-Fi joins (studio.py: the one picked in the Wi-Fi window) — for the quick settings panel's
// Wi-Fi tile; asked once, and again when Wi-Fi is switched on
let wifiNet = '', wifiAsked = false;
function askWifiNet() {
  wifiAsked = true;
  fetch('/api/wifi').then((r) => r.json()).then((j) => {
    wifiNet = String(j.current || '').replace(/[^ -~]/g, '').slice(0, 32);
    for (const e of engines) e.wf = null;  // (sent again with the next sync)
  }).catch(() => {});
}

// The engine's code, compiled once for all copies
let factory = null, wasmModule = null;
async function loadCode() {
  if (!factory) {
    const v = Date.now();
    factory = (await import(`../${BOARD.engine}/PopkaWatch.js?v=${v}`)).default;
    wasmModule = await WebAssembly.compileStreaming(fetch(`${BOARD.engine}/PopkaWatch.wasm?v=${v}`));
  }
}

export class Engine {
  // pin: the screen index this copy shows at rest (the map), -1 — the whole sketch (Try); page — its page
  constructor(canvas, pin, page = 0) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.pin = pin;
    this.page = page;
    this.slots = new Map();    // font / icons key -> slot in this copy
    this.applied = new Map();  // layer id -> {font, icons}: what it shows now (kept while a new size loads)
    this.loading = 0;
    this.last = '';
    this.onAssets = null;
    this.log = [];
    this.shown = true;   // its screen can be seen (stage.showEngines): off — no frames, the project kept for later
    this.stale = false;  // the project changed while it was off
  }

  async start() {
    await loadCode();
    const print = (s) => { this.log.push(s); if (this.log.length > 300) this.log.shift(); };
    this.M = await factory({
      print, printErr: print,
      instantiateWasm: (imports, done) => {
        WebAssembly.instantiate(wasmModule, imports).then((inst) => done(inst, wasmModule));
        return {};
      },
    });
    const M = this.M;
    this.rgba = M._web_rgba();
    this.dirty = M._web_dirty();
    if (this.pin >= 0) M._studio_pin(this.pin), M._studio_page(this.page);
    const n = Object.keys(KIND_INDEX).length, b = new Float32Array(M.HEAPF32.buffer, M._studio_bases(), n * 2);
    this.bases = Array.from({ length: n }, (_, k) => [b[k * 2], b[k * 2 + 1]]);  // HaloWatch's place of each kind
    this.next = performance.now();
    return this;
  }

  // One engine frame per 60 Hz period, whatever the monitor's rate. A display frame up to 2 ms early is taken as on
  // time: one tick, and the next one a period later (counted as 0 ticks it ran a tick anyway without moving the next
  // one: at 120 Hz every display frame then ran a tick — the animations twice as fast)
  frame(t) {
    if (!this.M || !this.shown || t < this.next - 2) return;
    let ticks = Math.max(1, 1 + Math.floor((t - this.next) / PERIOD));
    if (ticks > 4) this.next = t, ticks = 1;  // the tab was asleep: carry on from now
    this.next += ticks * PERIOD;
    if (this.M._web_frame(t, ticks)) this.paint();
  }

  // Only what changed goes to the canvas (the engine's last changed area: web_dirty); the picture over the engine's
  // memory is made once (again if the memory grew)
  paint(all = false) {
    paints++;
    const buf = this.M.HEAPU8.buffer;
    if (this.imgOf !== buf) this.imgOf = buf, this.img = new ImageData(new Uint8ClampedArray(buf, this.rgba, TW * TH * 4), TW, TH);
    const d = new Int32Array(buf, this.dirty, 4);
    if (all) return this.ctx.putImageData(this.img, 0, 0);
    if (d[2] >= d[0]) this.ctx.putImageData(this.img, 0, 0, d[0], d[1], d[2] - d[0] + 1, d[3] - d[1] + 1);
  }

  // Seen or not (stage.showEngines: Edit — the screen being edited, Try — Try's copy, Map — the screens in the window).
  // Off: no frames, its last picture stays; seen again — the project as it is now, and it carries on (its animations
  // from where they were: frame() starts from now)
  show(on) {
    if (on === this.shown) return;
    this.shown = on;
    if (on && this.stale) this.stale = false, this.sync(state.project?.screens || []);
  }

  // Its picture now, seen or not (the project's picture for the start window): the project, a frame, all of it
  drawNow() {
    if (!this.M) return;
    if (this.stale) this.stale = false, this.sync(state.project?.screens || []);
    this.M._web_frame(performance.now(), 1);
    this.paint(true);
  }

  base(layer) {
    return this.bases[KIND_INDEX[layer.type]];
  }

  // HaloWatch's place of each layer of screen s (what its dx, dy are counted from): [x, y] or null — not drawn yet
  places(s, n) {
    if (!this.M) return [];
    const f = new Float32Array(this.M.HEAPF32.buffer, this.M._studio_places(s), n * 2);
    const out = [];
    for (let i = 0; i < n; i++) out.push(Number.isFinite(f[i * 2]) && Number.isFinite(f[i * 2 + 1]) ? [f[i * 2], f[i * 2 + 1]] : null);
    return out;
  }

  // How far from the screen's middle each radial layer of screen s reaches (curved text, rings, the week, the forecast;
  // its dx, dy left out): [inner, outer] or null
  radii(s, n) {
    if (!this.M) return [];
    const f = new Float32Array(this.M.HEAPF32.buffer, this.M._studio_radii(s), n * 2);
    const out = [];
    for (let i = 0; i < n; i++) out.push(Number.isFinite(f[i * 2 + 1]) ? [f[i * 2], f[i * 2 + 1]] : null);
    return out;
  }

  // Where each layer of screen s was drawn in the last frame: [x0, y0, x1, y1] or null
  boxes(s, n) {
    if (!this.M) return [];
    const f = new Float32Array(this.M.HEAPF32.buffer, this.M._studio_boxes(s), n * 4);
    const out = [];
    for (let i = 0; i < n; i++) out.push(f[i * 4 + 2] < f[i * 4] ? null : [f[i * 4], f[i * 4 + 1], f[i * 4 + 2], f[i * 4 + 3]]);
    return out;
  }

  // The finger: t — when it was there (an event's timeStamp; anything odd — now)
  touch(down, x, y, t) {
    const now = performance.now();
    if (!(t > now - 500 && t <= now + 1)) t = now;
    if (this.M) this.M._web_touch(down ? 1 : 0, x, y, t);
  }

  command(line) {
    const p = this.M.stringToNewUTF8(line);
    this.M._web_command(p);
    this.M._free(p);
  }

  // The city of page p (weather.h)
  city(p) {
    return this.M ? this.M.UTF8ToString(this.M._studio_city(p)) : '';
  }

  // The screen on top in Try: its index, -1 — the face, -2 — a stub (a card without a link: stubName — its title)
  openScreen() {
    return this.M ? this.M._studio_open() : -1;
  }
  stubName() {
    return this.M ? this.M.UTF8ToString(this.M._studio_stub()) : '';
  }

  // Try: as a tap on layer i of screen s (on top now), and the frames after it at once — the transition played out of
  // sight; ms of animation time (the page's clock stays: the finger is not touched)
  tap(s, i) {
    if (this.M) this.M._studio_tap(s, i);
  }
  skip(ms) {
    if (!this.M) return;
    const now = performance.now();
    for (let t = 0; t < ms; t += 2 * PERIOD) this.M._web_frame(now, 2);
    this.paint(true);  // (many frames: all of it)
    this.next = performance.now();
  }

  // A pinned copy: its screen's list scrolled to show layer i (-1 — the top)
  scroll(s, i) {
    if (this.M) this.M._studio_scroll(s, i);
  }

  get busy() {
    return this.loading > 0;
  }

  // A font or icons into this copy: from the shared blob, once per key
  slot(kind, key, body) {
    if (this.slots.has(key)) return this.slots.get(key);
    this.slots.set(key, undefined);
    this.loading++;
    fetchBlob(kind, key, body).then((bytes) => {
      const ptr = this.M._malloc(bytes.length);
      this.M.HEAPU8.set(bytes, ptr);
      this.slots.set(key, kind === 'font' ? this.M._studio_font(ptr, bytes.length) : this.M._studio_icons(ptr, bytes.length));
      this.M._free(ptr);
    }).catch((e) => {  // refused (4xx: no such font) — not asked again; studio.py not there — asked with the next change
      console.error(e);
      if (e.status >= 400 && e.status < 500) this.slots.set(key, -1);
      else this.slots.delete(key);
    })
      .finally(() => { this.loading--; if (this.onAssets) this.onAssets(); });
    return undefined;
  }

  // The project to this copy. A pinned copy loads fonts only for its own screen (the others are not drawn). A font
  // that is not here yet is asked for; until it comes the layer keeps showing the one it had (no blinking while a size
  // is dragged)
  sync(screens) {
    if (!this.M) return;
    if (!this.shown) return void (this.stale = true);  // (off: when it is seen again)
    const index = new Map(screens.map((s, i) => [s.id, i]));
    const main = index.get(state.project?.main) ?? -1;  // the gallery
    const all = screens.map((s, si) => ({ n: s.name, l: s.layers.map((l) => {
      const mine = this.pin < 0 || this.pin === si;
      const was = this.applied.get(l.id) || { font: -1, icons: -1, font2: -1, font3: -1, font4: -1 };
      let font = -1, icons = -1, font2 = -1, font3 = -1, font4 = -1;
      if (mine) {
        if (fontKey(l)) {
          font = this.slot('font', fontKey(l), { type: l.type, size: l.size, weight: l.weight, width: l.width, scale: l.scale, look: l.look, board: BOARD.id });
          if (font === undefined) font = was.font;
        }
        if (l.type === 'sclock') {  // (a clock's third and fourth fonts)
          if (scKey(l, 2)) font3 = this.slot('font', scKey(l, 2), { type: l.type, scale: l.scale, look: l.look, part: 'font3', board: BOARD.id });
          if (font3 === undefined) font3 = was.font3;
          if (scKey(l, 3)) font4 = this.slot('font', scKey(l, 3), { type: l.type, scale: l.scale, look: l.look, part: 'font4', board: BOARD.id });
          if (font4 === undefined) font4 = was.font4;
        }
        if (iconsKey(l)) {
          icons = this.slot('icons', iconsKey(l), { type: l.type, scale: l.scale, size: l.size, format: l.format, board: BOARD.id });
          if (icons === undefined) icons = was.icons;
        }
        if (font2Key(l)) {
          font2 = this.slot('font', font2Key(l), { type: l.type, scale: l.scale, size: l.size, format: l.format, look: l.look, part: 'icon', board: BOARD.id });
          if (font2 === undefined) font2 = was.font2;
        }
        this.applied.set(l.id, { font, icons, font2, font3, font4 });
      }
      // Links (as studio.py writes them): from the face's layers and the gallery's cards and button; not to the face,
      // the gallery or the layer's own screen
      let to = (si === 0 && !KINDS[l.type].fixed) || TAPS.has(l.type) || l.type === 'display' ? (l.tap ? index.get(l.tap.to) ?? -1 : -1) : -1;
      if (to === 0 || to === main || to === si) to = -1;
      return {
        kind: KIND_INDEX[l.type], dx: Math.fround(l.dx), dy: Math.fround(l.dy), k: Math.fround(l.scale ?? 1),
        c: parseInt((l.color || '#000000').slice(1), 16), c2: parseInt((l.color2 || '#000000').slice(1), 16), sp: l.spacing ?? 0,
        font, icons, font2, font3, font4, c3: parseInt((l.color3 || '#000000').slice(1), 16),
        tap: to > 0 ? to : -1, fx: FX[l.tap?.fx]?.index ?? 1, p: Math.fround(l.goal ?? l.count ?? l.radius ?? l.angle ?? l.gap ?? 0), hidden: !!l.hidden,
        p2: Math.fround(P2[l.type] ? P2[l.type](l) : l.twinkle ?? SPARKLES[l.sparkles] ?? ((l.type === 'steps' || l.type === 'weather') && l.pill === false ? 1 : 0)),
        p3: Math.fround(P3[l.type] ? P3[l.type](l) : l.meteors ?? l.rainbow ?? 0),
        t: l.text ?? '', u: l.subtitle ?? '', i: l.icon && ICONS[l.icon] ? String.fromCodePoint(ICONS[l.icon]) : '',
      };
    }) }));
    const json = JSON.stringify(all);
    if (json !== this.last) {
      this.last = json;
      const p = this.M.stringToNewUTF8(json);
      this.M._studio_set(p);
      this.M._free(p);
    }
    if (main !== this.main) this.main = main, this.M._studio_main(main);  // after the screens: it must be one of them
    // The weather (every copy): the cities — its pages — and their numbers from studio.py ("--" until they come)
    const wx = JSON.stringify({ c: citiesOf().map((c, k) => ({ n: c.name, ...weatherOf(k) })) });
    if (wx !== this.wx) {
      this.wx = wx;
      const p = this.M.stringToNewUTF8(wx);
      this.M._studio_weather(p);
      this.M._free(p);
    }
    // The quick settings panel's Wi-Fi (shade_rect.h, shade_round.h): this project's Wi-Fi and its network, as its board shows it
    const wifiOn = !!fnOf('wifi').on;
    if (wifiOn && !wifiAsked) askWifiNet();
    if (!wifiOn) wifiAsked = false;  // (switched on again: the network asked anew)
    const wf = wifiOn ? `on ${wifiNet}` : 'off';
    if (wf !== this.wf) this.wf = wf, this.command('shade wifi ' + wf);
    // The time (every copy: the clocks on the map too): 12 or 24 hours, the picked zone's wall clock
    const tm = timeOf(), clk = [tm.h24 ? 1 : 0, zoneShift(tm.zone)];
    if (clk.join() !== this.clk) this.clk = clk.join(), this.M._studio_clock(...clk);
    // The Functions (power.h): off — Always On never, a button nothing. Every copy: a pinned one stands still (no
    // power_tick there), but the quick settings panel shows Always On's state (shade_rect.h)
    {
      const a = fnOf('aod'), b = fnOf('boot'), p = fnOf('pwr'), act = (f) => (!f.on ? 0 : { off: 1, lock: 2, aod: 3 }[f.act] ?? 1);
      const steps = state.project.screens.some((s) => s.layers.some((l) => l.type === 'steps'));  // (the sensor is the pedometer's)
      const raise = a.raise && !steps && !lacks('raise');
      const fn = [a.on ? 1 : 0, a.idle, a.bright, act(b), lacks('pwr') ? 0 : act(p), a.tap ? 1 : 0, raise ? 1 : 0, a.btn || !(a.tap || raise) ? 1 : 0, a.home ? 1 : 0,
        a.shift === false || lacks('shift') ? 0 : 1];
      if (fn.join() !== this.fn) this.fn = fn.join(), this.M._studio_fn(...fn);
    }
  }
}

// All copies on one display clock
export const engines = new Set();
let running = false;
export function runAll(after, before) {
  if (running) return;
  running = true;
  const loop = (t) => {
    requestAnimationFrame(loop);
    if (before) before(t);  // (which copies are seen)
    for (const e of engines) e.frame(t);
    if (after) after(t);
  };
  requestAnimationFrame(loop);
}
