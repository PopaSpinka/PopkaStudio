// screen.js — the board this page is drawn for: its screen's size and shape, from its passport (boards/<id>/board.json).
//
// Everything that knows a screen's size reads it here. The page serves one board at a time: the numbers are fixed while it
// lives, and opening a project of another board reloads the page (start.js). So this module waits for the answer before
// any other starts (a top-level await).
const saved = (() => { try { return localStorage.getItem('popka.project'); } catch (e) { return null; } })();
const ask = (url) => fetch(url, { cache: 'no-store' }).then((r) => r.json());
const [{ boards }, { projects }] = await Promise.all([ask('/api/boards'), ask('/api/projects')]);
const project = projects.find((p) => p.id === saved) || projects[0];

export const BOARD = boards.find((b) => b.id === (project?.board || 'amoled175')) || boards[0];
export const TW = BOARD.w, TH = BOARD.h;  // the screen on the map and in the engine, board pixels
export const CX = TW / 2, CY = TH / 2;
export const ROUND = BOARD.shape === 'round';
// A new layer on a screen taller than the round one: HaloWatch's places are for 466 rows, this keeps them in the middle
// What this board has not got (the passport's "without": the motion sensor "raise", the burn-in shift, the PWR button;
// the round one — the Video app, made for the P4's JPEG decoder and microSD card)
export const lacks = (key) => (BOARD.without || []).includes(key);
// What this board offers: a passport's list ("layers", "screens", "functions"); no list — everything it doesn't lack
export const allowed = (list, key) => (!BOARD[list] || BOARD[list].includes(key)) && !lacks(key);

const root = document.documentElement;
root.dataset.shape = BOARD.shape;
root.style.setProperty('--scr-w', `${TW}px`);
root.style.setProperty('--scr-h', `${TH}px`);
