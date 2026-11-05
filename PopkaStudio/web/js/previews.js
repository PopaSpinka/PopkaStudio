// previews.js — the folders' tiles show what they add, drawn by the board's own code: a hidden engine copy draws the
// tile's layers alone on a screen (the real weather, the time, the fonts from studio.py), the picture is cut to what is
// drawn (a whole screen — the round screen) and kept as an image. Made once each (by the tile's key), one at a time,
// while the folder is open; until it is ready the tile shows its icon.
import { Engine } from './engine.js';

const W = 466, PERIOD = 1000 / 60;
const TW = 220, TH = 104;  // the picture, device pixels (the tile shows it at half: 110 × 52)
let eng = null, ready = null, queue = Promise.resolve();
const cache = new Map();
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function engine() {
  if (!ready) {
    const canvas = document.createElement('canvas');
    canvas.width = canvas.height = W;
    eng = new Engine(canvas, 1);  // (screen 1: an empty home before it)
    eng.onAssets = () => {};
    ready = eng.start();
  }
  return ready;
}

// key — the tile's; make() — its layers (made when it is drawn); whole — a whole screen (round, not cut); px — that
// screen's size (device pixels; none — the tile's)
export function preview(key, make, whole = false, px = 0) {
  if (!cache.has(key)) {
    const p = queue.then(() => draw(make(), whole, px)).catch(() => null);
    queue = p;
    cache.set(key, p);
  }
  return cache.get(key);
}

async function draw(layers, whole, px) {
  const e = await engine();
  const screens = [{ id: 'pv-home', name: '', layers: [] }, { id: 'pv', name: '', layers }];
  e.last = '';
  e.sync(screens);
  for (let i = 0; i < 200 && e.loading > 0; i++) await sleep(25);  // its fonts and icons from studio.py
  e.last = '';
  e.sync(screens);  // (now with them)
  let t = performance.now();
  for (let i = 0; i < 30; i++) e.M._web_frame((t += PERIOD), 1);
  e.paint(true);
  // What to show: the whole screen, or the layers' boxes (with a margin)
  let x0 = 0, y0 = 0, x1 = W, y1 = W;
  if (!whole) {
    const boxes = e.boxes(1, layers.length).filter(Boolean);
    if (!boxes.length) return null;
    x0 = Math.max(0, Math.min(...boxes.map((b) => b[0])) - 8), y0 = Math.max(0, Math.min(...boxes.map((b) => b[1])) - 8);
    x1 = Math.min(W, Math.max(...boxes.map((b) => b[2])) + 8), y1 = Math.min(W, Math.max(...boxes.map((b) => b[3])) + 8);
  }
  const w = x1 - x0, h = y1 - y0, k = px ? px / W : Math.min(TW / w, TH / h);
  const out = document.createElement('canvas');
  out.width = Math.round(w * k), out.height = Math.round(h * k);
  const c = out.getContext('2d');
  c.imageSmoothingQuality = 'high';
  if (whole) c.beginPath(), c.arc(out.width / 2, out.height / 2, out.width / 2, 0, Math.PI * 2), c.clip();
  c.drawImage(e.canvas, x0, y0, w, h, 0, 0, out.width, out.height);
  return out.toDataURL('image/png');
}

// A tile's picture: its icon now, the drawing when it is ready
export function thumb(key, icon, make, whole = false) {
  const span = document.createElement('span');
  span.className = 'tool-pic' + (whole ? ' whole' : '');
  span.innerHTML = icon || '';
  preview(key, make, whole).then((url) => {
    if (!url) return;
    const img = new Image();
    img.src = url, img.alt = '', img.draggable = false;
    span.replaceChildren(img), span.classList.add('drawn');
  });
  return span;
}
