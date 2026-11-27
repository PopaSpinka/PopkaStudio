// screen.js — the board this page is drawn for: its screen's size and shape, from its passport (boards/<id>/board.json).
//
// Everything that knows a screen's size reads it here. The page serves one board at a time: the numbers are fixed while it
// lives, and opening a project of another board reloads the page (start.js). So this module waits for the answer before
// any other starts (a top-level await).
const saved = (() => { try { return localStorage.getItem('popka.project'); } catch (e) { return null; } })();
const ask = (url) => fetch(url, { cache: 'no-store' }).then((r) => r.json());
const [{ boards }, { projects }] = await Promise.all([ask('/api/boards'), ask('/api/projects')]);
const project = projects.find((p) => p.id === saved) || projects[0];

// A board whose screen is used either way up (its passport's "orientations"): the project's way laid over the passport —
// its w, h, engine, looks… — so everything below reads one board, as before (studio.py: variant)
export function boardFor(b, way) {
  const over = b?.orientations?.[way];
  if (!over) return b;
  const out = { ...b, ...over, way };
  for (const k of ['looks', 'places', 'moves']) if (b[k] && over[k]) out[k] = { ...b[k], ...over[k] };
  return out;
}
const base = boards.find((b) => b.id === (project?.board || 'amoled175')) || boards[0];
export const BOARD = boardFor(base, project?.orientation);
export const WAY = BOARD.way || null;  // which way up (null — a board with one way)
export const TW = BOARD.w, TH = BOARD.h;  // the screen on the map and in the engine, board pixels
export const CX = TW / 2, CY = TH / 2;
export const ROUND = BOARD.shape === 'round';
// A rectangular watch (the passport's "watch": the 2.06): a rectangle with some of the round's ways — its hourly pill
export const WATCH = !ROUND && !!BOARD.watch;
// A new layer on a screen taller than the round one: HaloWatch's places are for 466 rows, this keeps them in the middle
// What this board has not got (the passport's "without": the motion sensor "raise", the burn-in shift, the PWR button;
// the round one — the Video app, made for the P4's JPEG decoder and microSD card)
export const lacks = (key) => (BOARD.without || []).includes(key);
// What this board offers: a passport's list ("layers", "screens", "functions"); no list — everything it doesn't lack
export const allowed = (list, key) => (!BOARD[list] || BOARD[list].includes(key)) && !lacks(key);

// A whole screen's picture on a tile (previews.js, studio.css .tool-pic.whole), CSS px: in the screen's own shape — as
// much glass as the 7″'s 88 × 52 (a square 68 × 68, a tall one up to 76 high), never wider than 88; the round one — its
// 52 circle. Before 18 Nov 2026 every rectangle was 88 wide and cut to 52 high: the square 4B's and the tall screens'
// pictures came out squeezed into a wide one
const [PIC_W, PIC_H] = (() => {
  if (ROUND) return [52, 52];
  const r = TW / TH;
  let w = Math.sqrt(88 * 52 * r), h = w / r;
  if (w > 88) w = 88, h = 88 / r;
  if (h > 76) h = 76, w = 76 * r;
  return [Math.round(w), Math.round(h)];
})();
export { PIC_W, PIC_H };
// The screen as a small outline in a 16-unit icon (the Board button): its own shape, as much as a 12 × 8 one
export function screenIcon() {
  const r = TW / TH;
  let w = Math.sqrt(96 * r), h = w / r;
  if (w > 12) w = 12, h = 12 / r;
  if (h > 12) h = 12, w = 12 * r;
  const f = (v) => +v.toFixed(2);
  return `<rect x="${f(8 - w / 2)}" y="${f(8 - h / 2)}" width="${f(w)}" height="${f(h)}" rx="1.6"/>`;
}

const root = document.documentElement;
root.dataset.shape = BOARD.shape;
root.style.setProperty('--scr-w', `${TW}px`);
root.style.setProperty('--scr-h', `${TH}px`);
root.style.setProperty('--pic-w', `${PIC_W}px`);
root.style.setProperty('--pic-h', `${PIC_H}px`);
