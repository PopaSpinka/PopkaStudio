// stage.js — the middle of the page, in two ways of looking at the watch:
//   Edit — one screen (the active one) in the middle, as on the board: select, move and resize its layers; the screen
//          stays in the middle (no panning there), only the zoom changes;
//   Map  — every screen of the project, each drawn by its own copy of the board's code, and the links between them:
//          one blue curve per link, as Figma's prototype connections (a dot at the layer, an arrow at the screen it
//          opens; the way back — a swipe up from its bottom edge to the face, or from the left edge to the gallery — is
//          the same link), and one from the face to the gallery (a swipe up, a swipe down back); pan and zoom; a screen is grabbed whole (by its name or anywhere on it) — its layers are edited in Edit.
// A part of the gallery is placed by its list: it is selected, not moved (a selected one the list scrolls to); a
// title or a card is dragged up or down to another place in the list (a line shows where it goes), and a new one
// dropped onto a list goes in where it was dropped.
// A screen with pages (the Weather screen's cities) stands as a row of its pages joined by arrows (a swipe to the next
// one), a copy of the engine for each; any page can be picked (state.page: Edit and Try show that one); a gallery's
// button at the bottom edge (More) stands on it always, its link starts at its bottom.
// Both: dropping new elements from the palette onto a screen. And Try: the whole sketch running as on the board, the
// mouse is a finger — its screen exactly where Edit has its own (Edit's zoom, in the middle), so the Edit ⇄ Try switch
// changes nothing in place. The zoom and the place of each way are kept in the browser (they come back after a reload).
//
// Coordinates: the map is in board pixels (a screen is 466 × 466 at its x, y); the view puts the map on the page with
// a shift and a zoom. Inside a screen — the board's pixels. A layer's place is its anchor — what X and Y in
// Properties are: HaloWatch's place of it (the engine says where: studio_places) plus its dx, dy — the date and the
// clock: the center of the line at the middle of the letters (the clock: its colon), the pills — their center, the
// temperature — the middle of its digits, the icon — its center, the high/low — the right end of its row. Moving snaps
// the anchor to the screen's center lines and to HaloWatch's place.
import {
  state, screens, screenById, layerById, screenOf, selected, select, change, beginGesture, emit, newLayer, links,
  activeScreen, mainScreen, KINDS, KIND_INDEX, LISTED, listed, moveInList, listIndex, wayTo, home,
  roomFor, PAGES, PAGE_STEP, paged, span, citiesOf, isGallery, pagesOf, pageName, on, listPlace,
} from './state.js';
import { Engine, engines, paints } from './engine.js';
import { BOARD, TW, TH, CX, CY, ROUND, BEZEL } from './screen.js';

const SNAP = 6, HANDLE = 5, SAVE_KEY = ROUND ? 'popka.view' : `popka.view.${BOARD.id}`;  // (each shape its own zoom)
// Round the screen's middle: curved text, the week, the forecast (they turn: angle) and the rings (radius only). Dragged,
// they go round the middle and to or from it — they stay round it, X and Y untouched (moveRadial)
const RADIAL = new Set(['curve', 'weekarc', 'forecast', 'stepring', 'batring']);

// Try's side button at height y (its middle: PWR 151, BOOT 315) as a piece of the bezel's ring (studio.css .side-btn):
// from radius 240 (under the bezel's edge at 242) out to 251 — 247 pressed — 12° of the circle; a light line along
// its outer edge. Board pixels, clockwise from 12 o'clock
function sideButton(y) {
  const a = 90 + Math.asin((y - CX) / 242) * 180 / Math.PI;
  const at = (r, d) => [CX + r * Math.sin((d * Math.PI) / 180), CX - r * Math.cos((d * Math.PI) / 180)].map((v) => v.toFixed(2)).join(' ');
  const piece = (r0, r1, d0, d1) => `M${at(r1, d0)} A${r1} ${r1} 0 0 1 ${at(r1, d1)} L${at(r0, d1)} A${r0} ${r0} 0 0 0 ${at(r0, d0)}Z`;
  const one = (cls, ro) => `<g class="${cls}"><path class="body" d="${piece(240, ro, a - 6, a + 6)}"/><path class="shine" d="${piece(ro - 1.6, ro - 0.4, a - 5, a + 5)}"/></g>`;
  return `<svg viewBox="0 0 466 466" aria-hidden="true">${one('out', 251)}${one('in', 247)}</svg>`;
}
const TURNS = new Set(['curve', 'weekarc', 'forecast']);
const flipped = (type, angle) => TURNS.has(type) && Math.abs(angle) > 120;  // (widgets.h: the bottom — the other way up)

// The view kept in the browser: the last way of looking, the shift and zoom, what Fit fitted, and Map's own
function loadViews() {
  try {
    const v = JSON.parse(localStorage.getItem(SAVE_KEY) || 'null');
    if (v && typeof v === 'object') return v;
  } catch (e) { /* no storage here: start fresh */ }
  return {};
}

export class Stage {
  get cities() {  // the Weather screen's pages: the cities' names
    return citiesOf().map((c) => c.name);
  }

  constructor() {
    this.viewport = document.getElementById('viewport');
    this.world = document.getElementById('world');
    this.overlay = document.getElementById('overlay');
    this.octx = this.overlay.getContext('2d');
    this.tryBox = document.getElementById('try');
    this.tiles = new Map();  // screen id -> {el, canvas, engine}
    this.hover = null;       // {type: 'layer' | 'link' | 'label', id}
    this.guides = [];
    this.drag = null;
    this.fingerDown = false;
    this.arcs = [];
    this.labels = [];
    // Edit and Try share one view: switching between them moves nothing — the screen stays where it is, at its size.
    // Map has its own: from Map to Edit or Try the screen picked there is fitted; back in Map — the map as it was left
    // (shift and zoom). Fit fits the screen being edited (Edit, Try) or all of them (Map), and that fit holds when the
    // window changes size until the view is moved by hand
    const saved = loadViews();
    this.savedView = saved.view && saved.view.z > 0 ? saved.view : null;
    this.fitTo = this.savedView ? saved.fitTo || null : 'screen';  // 'screen' | 'all' | null
    this.mapView = saved.mapView && saved.mapView.view?.z > 0 ? saved.mapView : null;  // {view, fitTo}: Map as left
    this.startMode = saved.mode === 'map' ? 'map' : 'edit';
    this.mapCam = this.startMode === 'map';  // the view now is Map's
    const liveCanvas = document.getElementById('live');
    liveCanvas.width = TW, liveCanvas.height = TH;
    this.live = new Engine(liveCanvas, -1);
    this.onEdit = null;  // a double click on a screen in Map: main.js opens Edit on it
    this.pager = document.getElementById('pager');
    this.pager.addEventListener('pointerdown', (e) => e.stopPropagation());  // (not the field's: no deselect)
    document.getElementById('page-prev').addEventListener('click', () => this.stepLeft());
    document.getElementById('page-next').addEventListener('click', () => this.stepRight());
    document.getElementById('pager-name').addEventListener('click', () => {  // (the name: the screen picked, as a click on it)
      const s = activeScreen();
      select({ type: 'screen', id: s.id, page: paged(s) ? Math.min(state.page, pagesOf(s) - 1) : 0 });
    });
    this.fwd = [];         // Edit: the screens left by Back, the latest last (Forward goes there)
    this.came = null;      // the screen before this one (Back goes there if it opens this one)
    this.bind();
    on(() => { this.rev++; });  // (anything changed — the project, the selection, the mode: the overlay draws again)
    this.rev = 0;
    document.fonts.ready.then(() => { this.rev++; });  // (the names' font came)
    new ResizeObserver(() => {
      this.vw = this.viewport.clientWidth, this.vh = this.viewport.clientHeight;  // (showEngines: no layout read a frame)
      this.sizeOverlay();
      this.resized();
    }).observe(this.viewport);
  }

  // Before each frame: which copies of the engine run — those whose screen can be seen. Edit: the screen being edited
  // (its pages in the window); Try: Try's own copy (the box is over everything); Map: the screens in the window. The
  // others keep their last picture and do nothing — no frames, no project — till they are seen again (engine.js: show)
  showEngines() {
    const { x, y, z } = state.view, x0 = -x / z, y0 = -y / z, x1 = x0 + (this.vw ?? 0) / z, y1 = y0 + (this.vh ?? 0) / z;
    const sees = (mx, my) => mx < x1 && mx + TW > x0 && my < y1 && my + TH > y0;
    const tr = state.mode === 'try';
    for (const s of screens()) {
      const t = this.tiles.get(s.id);
      if (!t) continue;
      const on = !tr && (state.mode !== 'edit' || s.id === state.screen);
      t.engine.show(on && sees(s.x, s.y));
      t.pages.forEach((q, k) => q.engine.show(on && sees(s.x + (k + 1) * PAGE_STEP, s.y)));
    }
    this.live.show(tr);
  }

  async start() {
    await this.live.start();
    engines.add(this.live);
    this.ensureTiles();
    this.sizeOverlay();
    const r = this.viewport.getBoundingClientRect();
    this.size = [r.width, r.height];
    if (this.savedView) state.view = { ...this.savedView }, this.applyView();
    this.refit();
    this.setMode(this.startMode);
  }

  // The window changed size: the fit made again, or the view's middle kept in the middle
  resized() {
    const r = this.viewport.getBoundingClientRect(), was = this.size;
    if (!was) return;  // (not started)
    this.size = [r.width, r.height];
    if (this.fitTo) this.refit();
    else state.view.x += (r.width - was[0]) / 2, state.view.y += (r.height - was[1]) / 2, this.applyView();
    this.placeTry();
  }

  refit() {
    if (this.fitTo === 'screen') this.editView();
    else if (this.fitTo === 'all') this.fitAll();
  }

  // ---------- Edit or Map ----------
  setMode(m) {
    state.mode = m;
    if (m === 'map') {
      if (!this.mapCam && this.mapView) {  // Map as it was left
        this.fitTo = this.mapView.fitTo;
        if (this.fitTo === 'all') this.fitAll();
        else state.view = { ...this.mapView.view }, this.applyView();
      }
      this.mapCam = true;
    } else if (this.mapCam) {
      this.leaveMap();
    } else {
      if (this.spotId && this.spotId !== state.screen) this.focus(activeScreen());  // (another screen came from Try)
      this.markSpot(activeScreen()), this.inView(activeScreen());
    }
    this.ensureTiles();
    this.save();
  }

  // From Map to Edit or Try: Map's view kept for coming back; the screen picked there — fitted
  leaveMap() {
    if (!this.mapCam) return;
    this.mapView = { view: { ...state.view }, fitTo: this.fitTo === 'all' ? 'all' : null };
    this.mapCam = false;
    this.fitTo = 'screen';
    this.editView();
    this.markSpot(activeScreen());
  }

  // Edit and Try show the active screen: a view that has it out of the window altogether (moved away on the map)
  // brings it to the middle, at the same zoom
  // Where Edit and Try show screen s on the map: its picked page (state.page), the first one of any other
  pageX(s) {
    return s.x + (s.id === state.screen && paged(s) ? Math.min(state.page, pagesOf(s) - 1) : 0) * PAGE_STEP;
  }
  markSpot(s) {
    this.spotId = s.id, this.spotX = this.pageX(s);
  }
  // The cities' order changed (their page numbers with it): Edit and Try keep showing the same city in its place
  keepPage() {
    const s = activeScreen();
    if (state.mode === 'map' || !paged(s) || this.spotId !== s.id) return;
    state.view.x += ((this.spotX ?? this.pageX(s)) - this.pageX(s)) * state.view.z;
    this.applyView();
    this.markSpot(s);
    this.save();
  }

  // Another page of the active screen (Edit: the arrows at its sides, ← →; the list of screens): Edit — it slides into
  // the screen's place; Map — it gets the bright frame
  goPage(k) {
    const s = activeScreen();
    k = Math.max(0, Math.min(pagesOf(s) - 1, k));
    if (!paged(s) || k === state.page) return;
    const was = this.pageX(s);
    state.page = k;
    if (state.sel?.type === 'screen' && state.sel.id === s.id) state.sel = { ...state.sel, page: k };
    emit('select');
    if (state.mode !== 'edit') return;
    this.slideBy((was - this.pageX(s)) * state.view.z);
    this.markSpot(s);
  }
  // ---------- Edit: back and forward between screens, as on the board ----------
  // Back — where the board's back goes from this screen: the screen whose layer opens it (the one just come from, if it
  // is one of those — the swipe from the left edge), the face from the gallery and from what the face opens (the swipe
  // up from the bottom edge: home), the face too from a screen nothing opens; from the face itself — nowhere. Forward —
  // the screen left by Back; going anywhere else forgets it (as a browser's)
  backOf(s) {
    if (!s || s === home()) return null;
    const came = screenById(this.came);
    if (came && came !== s && links().some((k) => k.from === came && k.to === s)) return came;
    const w = wayTo(s), last = w && w[w.length - 1];
    return last && last.from && last.from !== s ? last.from : home();
  }
  // The screen changed (main.js): where it came from; a jump not made by Back or Forward forgets Forward
  screenChanged() {
    if (this.cur && this.cur !== state.screen) this.came = this.cur;
    this.cur = state.screen;
    if (!this.navving) this.fwd = [];
  }
  jump(s, page = 0) {
    this.navving = true;
    select({ type: 'screen', id: s.id, page });
    this.navving = false;
  }
  goBack() {
    const s = activeScreen(), b = this.backOf(s);
    if (!b) return;
    this.fwd.push(s.id);
    this.jump(b);
  }
  goForward() {
    let s = null;
    while (!s && this.fwd.length) s = screenById(this.fwd.pop());
    if (s) this.jump(s);
  }
  // The arrows (and ← →): a screen with pages — its pages first, then back (at the first) or forward (at the last)
  stepLeft() {
    const s = activeScreen();
    if (paged(s) && Math.min(state.page, pagesOf(s) - 1) > 0) return this.goPage(state.page - 1);
    this.goBack();
  }
  stepRight() {
    const s = activeScreen();
    if (paged(s) && state.page < pagesOf(s) - 1) return this.goPage(state.page + 1);
    this.goForward();
  }
  // A double click on a layer that opens a screen (a card, More, the face's pill…): Edit on that screen
  enter(l) {
    const k = links().find((q) => q.layer === l);
    if (k && k.to !== screenOf(l.id)) select({ type: 'screen', id: k.to.id, page: 0 });
  }

  // A page of screen s from the list: picked, and shown (Edit: in the screen's place; Map: only picked — the bright
  // frame moves to it, the map stays still, as with a click on it there)
  showPage(s, k) {
    if (state.mode === 'map') return select({ type: 'screen', id: s.id, page: k });
    if (s.id !== state.screen || state.mode !== 'edit') {
      select({ type: 'screen', id: s.id, page: k });
      return this.focus(s);
    }
    if (!(state.sel?.type === 'screen' && state.sel.id === s.id)) select({ type: 'screen', id: s.id, page: state.page });
    this.goPage(k);
  }
  // The view moved sideways by dx window px, smoothly (a page coming in); slideTo — where it goes
  slideBy(dx) {
    const x0 = state.view.x, t0 = performance.now(), ms = 320;
    this.slideTo = (this.slide ? this.slideTo : x0) + dx;
    this.slide = (t) => {
      const q = Math.min(1, (t - t0) / ms), e = 1 - (1 - q) ** 3;
      state.view.x = x0 + dx * e;
      this.applyView();
      if (q >= 1) this.slide = null, (state.view.x = this.slideTo), this.applyView(), this.save();
    };
  }

  inView(s) {
    const r = this.viewport.getBoundingClientRect(), { x, y, z } = state.view;
    const x0 = x + this.pageX(s) * z, y0 = y + s.y * z, w = TW * z, h = TH * z;
    if (!(x0 + w > 0 && x0 < r.width && y0 + h > 0 && y0 < r.height)) this.center(s);
  }

  center(s) {
    const z = state.view.z, [x, y] = this.editCenter(z, s);
    state.view = { x, y, z };
    this.applyView();
  }

  // Edit: the active screen in the middle — fitted to the window, or at the zoom set by hand
  editFitZoom() {
    const r = this.viewport.getBoundingClientRect();
    const z = Math.min((r.width - 600) / (TW + 2 * BEZEL[0]), (r.height - 230) / (TH + 2 * BEZEL[1]));  // room for the cards at the sides, the top row,
                                                                          // the name above the screen
    return Math.max(0.25, Math.min(2, Math.floor(z * 4) / 4));
  }

  editCenter(z, s = activeScreen()) {
    const r = this.viewport.getBoundingClientRect();
    return [(r.width - TW * z) / 2 - this.pageX(s) * z, (r.height - TH * z) / 2 - s.y * z - 6];
  }

  // Fit the active screen: in the middle, as big as the window lets it be
  editView() {
    const z = this.editFitZoom(), [x, y] = this.editCenter(z);
    state.view = { x, y, z };
    this.applyView();
  }

  // The view moved by hand: no fit any more
  viewMoved() {
    if (this.fitTo) this.fitTo = null;
    this.save();
  }

  save() {
    clearTimeout(this.saveTimer);
    this.saveTimer = setTimeout(() => {
      try {
        const mode = state.mode === 'try' ? this.lastMode : state.mode;
        localStorage.setItem(SAVE_KEY, JSON.stringify({ mode, view: state.view, fitTo: this.fitTo, mapView: this.mapView }));
      } catch (e) { /* no storage: the view is just not kept */ }
    }, 300);
    if (state.mode !== 'try') this.lastMode = state.mode;
  }

  // Show screen s. Edit and Try: it takes the place of the screen shown there before (the same spot, the same size —
  // only the screen is another); Map: it comes to the middle
  focus(s) {
    const p = state.mode === 'map' ? null : screenById(this.spotId);
    if (!p) {
      this.center(s);
      if (this.fitTo === 'all') this.fitTo = null;
    } else if (p !== s) {
      const z = state.view.z;
      state.view.x += ((this.spotX ?? p.x) - this.pageX(s)) * z, state.view.y += (p.y - s.y) * z;
      this.applyView();
    }
    if (state.mode !== 'map') this.markSpot(s), this.inView(s);
    this.save();
  }

  // ---------- Screens: a tile and an engine copy for each ----------
  ensureTiles() {
    const ids = new Set(screens().map((s) => s.id));
    for (const [id, t] of this.tiles) {
      if (ids.has(id)) continue;
      for (const q of [t, ...t.pages]) engines.delete(q.engine);
      t.el.remove(), this.tiles.delete(id);
    }
    screens().forEach((s, i) => {
      let t = this.tiles.get(s.id);
      if (!t) {
        const el = document.createElement('div');
        el.className = 'tile';
        const canvas = document.createElement('canvas');
        canvas.width = TW, canvas.height = TH;
        el.append(canvas);
        this.world.append(el);
        t = { el, canvas, engine: new Engine(canvas, i), pages: [] };
        this.tiles.set(s.id, t);
        t.engine.onAssets = () => t.engine.sync(screens());
        t.engine.start().then(() => { engines.add(t.engine); t.engine.sync(screens()); });
      }
      // Its other pages: a copy each, in a row to its right (inside its tile: they move with it)
      const want = paged(s) ? pagesOf(s) - 1 : 0;
      while (t.pages.length > want) { const q = t.pages.pop(); engines.delete(q.engine), q.el.remove(); }
      while (t.pages.length < want) {
        const k = t.pages.length + 1, el = document.createElement('div'), canvas = document.createElement('canvas');
        el.className = 'page', el.style.left = k * PAGE_STEP + 'px';
        canvas.width = TW, canvas.height = TH;
        el.append(canvas), t.el.append(el);
        const q = { el, canvas, engine: new Engine(canvas, i, k) };
        t.pages.push(q);
        q.engine.onAssets = () => q.engine.sync(screens());
        q.engine.start().then(() => { engines.add(q.engine); q.engine.sync(screens()); });
      }
      for (const q of [t, ...t.pages]) {
        if (q.engine.pin === i) continue;
        q.engine.pin = i, q.engine.last = '';
        if (q.engine.M) q.engine.M._studio_pin(i);
      }
      const active = s.id === state.screen;
      t.el.style.left = s.x + 'px', t.el.style.top = s.y + 'px';
      t.el.classList.toggle('off', state.mode === 'edit' && !active);
      // Map: the picked screen in a bright frame — on its picked page
      const pg = active && paged(s) ? Math.min(state.page, pagesOf(s) - 1) : 0;
      t.el.classList.toggle('active', active && state.mode === 'map' && pg === 0);
      t.pages.forEach((q, j) => q.el.classList.toggle('active', active && state.mode === 'map' && pg === j + 1));
    });
  }

  sync() {
    this.ensureTiles();
    for (const t of this.tiles.values()) for (const q of [t, ...t.pages]) q.engine.sync(screens());
    this.live.sync(screens());
    this.reveal();
  }

  // The selected part of the gallery in view: its screen's list (in its map copy) scrolled to it — a title or a card
  // to the middle, the More button to the end
  reveal() {
    if (this.drag?.kind === 'list') return;  // (grabbed in its list: it stays where it is under the mouse)
    const l = selected(), s = l && screenOf(l.id), e = s && this.engineOf(s);
    if (e && (LISTED.has(l.type) || l.type === 'edge')) e.scroll(screens().indexOf(s), s.layers.indexOf(l));  // (More: the list's end)
    // The quick settings panel: open on the home's copy while its layer is selected (shade_rect.h: the command shade)
    const he = this.engineOf(home()), open = l?.type === 'shade';
    if (he?.M && he.shadeOpen !== open) he.shadeOpen = open, he.command(open ? 'shade open' : 'shade close');
  }

  engineOf(s) {
    return this.tiles.get(s.id)?.engine;
  }

  // HaloWatch's place of a layer: where its screen's copy of the engine has it (the Weather screen's icon and high/low
  // stand by the temperature), or — not drawn yet — the place of its kind
  base(layer) {
    const s = screenOf(layer.id), e = s && this.engineOf(s), i = s ? s.layers.indexOf(layer) : -1;
    const p = e && e.M && i >= 0 ? e.places(screens().indexOf(s), s.layers.length)[i] : null;
    return p || (this.live.bases || [])[KIND_INDEX[layer.type]] || [CX, CY];
  }

  // Where its layers were drawn (page k: in that page's copy)
  boxes(s, k = 0) {
    const t = this.tiles.get(s.id), e = k ? t?.pages[k - 1]?.engine : t?.engine;
    return e && e.M ? e.boxes(screens().indexOf(s), s.layers.length) : [];
  }

  // A map point in screen s: [x, y, page] — on one of its pages, that page's own pixels
  inScreen(s, mx, my) {
    let x = mx - s.x, k = 0;
    if (paged(s) && x > TW) k = Math.min(pagesOf(s) - 1, Math.round(x / PAGE_STEP)), x -= k * PAGE_STEP;
    return [x, my - s.y, k];
  }

  anchor(l) {
    const [bx, by] = this.base(l);
    return [bx + l.dx, by + l.dy];
  }

  // ---------- The view: shift and zoom ----------
  applyView() {
    const { x, y, z } = state.view;
    this.world.style.transform = `translate(${x}px, ${y}px) scale(${z})`;
    this.world.style.setProperty('--z', z);  // (the picked screen's frame on the map: as thick at any zoom)
    const g = Math.max(8, 18 * z);
    this.viewport.style.backgroundSize = `${g}px ${g}px`;
    this.viewport.style.backgroundPosition = `${x}px ${y}px`;
    this.world.classList.toggle('pixels', z >= 1.5);
    emit('view');
  }

  // The zoom shown next to its buttons, and a step of it
  zoom() {
    return state.view.z;
  }
  zoomBy(f) {
    this.setZoom(this.zoom() * f);
  }

  // A zoom about a point of the window: the mouse's, or — the buttons — the screen being edited (Edit, Try: it grows
  // where it is) or the window's middle (Map)
  setZoom(z, cx, cy) {
    const r = this.viewport.getBoundingClientRect(), v = state.view;
    if (cx == null) {
      const s = activeScreen();
      [cx, cy] = state.mode === 'map' ? [r.width / 2, r.height / 2] : this.toView(this.pageX(s) + CX, s.y + CY);
    }
    z = Math.max(0.1, Math.min(4, z));
    v.x = cx - ((cx - v.x) * z) / v.z, v.y = cy - ((cy - v.y) * z) / v.z, v.z = z;
    this.applyView();
    this.placeTry();
    this.viewMoved();
  }

  // Edit, Try: the active screen fitted; Map: all screens in the window (with room for the names and the links)
  fit() {
    this.fitTo = state.mode === 'map' ? 'all' : 'screen';
    this.refit();
    this.placeTry();
    this.save();
  }

  fitAll() {
    const list = screens();
    const r = this.viewport.getBoundingClientRect();
    const x0 = Math.min(...list.map((s) => s.x)) - 60, x1 = Math.max(...list.map((s) => s.x + span(s))) + 60;
    const y0 = Math.min(...list.map((s) => s.y - 110)), y1 = Math.max(...list.map((s) => s.y + TH)) + 150;
    // (in the room the cards leave: the left card, the right one while it shows properties, the top row)
    const props = document.querySelector('.card.right:not(.bare)');
    const L = 280, T = 50, R = props ? 300 : 20, B = 30, w = r.width - L - R, hh = r.height - T - B;
    const z = Math.max(0.1, Math.min(1, w / (x1 - x0), hh / (y1 - y0)));
    state.view = { z, x: L + (w - (x1 - x0) * z) / 2 - x0 * z, y: T + (hh - (y1 - y0) * z) / 2 - y0 * z };
    this.applyView();
  }

  sizeOverlay() {
    const dpr = window.devicePixelRatio || 1;
    if (this.vw === undefined) this.vw = this.viewport.clientWidth, this.vh = this.viewport.clientHeight;
    const w = Math.round(this.vw * dpr), h = Math.round(this.vh * dpr);
    if (this.overlay.width !== w || this.overlay.height !== h) this.overlay.width = w, this.overlay.height = h;
  }

  toMap(e) {
    const r = this.viewport.getBoundingClientRect(), v = state.view;
    return [(e.clientX - r.left - v.x) / v.z, (e.clientY - r.top - v.y) / v.z];
  }

  toView(mx, my) {
    const v = state.view;
    return [mx * v.z + v.x, my * v.z + v.y];
  }

  // The screens one can see now: Map — all of them, Edit — the active one
  shown() {
    return state.mode === 'edit' ? [activeScreen()] : screens();
  }

  tileAt(mx, my) {
    const list = this.shown();
    for (let i = list.length - 1; i >= 0; i--) {
      const s = list[i];
      if (mx >= s.x && mx <= s.x + span(s) && my >= s.y && my <= s.y + TH) return s;
    }
    return null;
  }

  // The topmost visible layer of screen s under the board point (a few pixels of slack around thin letters)
  hitLayer(s, x, y, page = 0) {
    const boxes = this.boxes(s, page), pad = 4 / state.view.z + 2;
    for (let i = s.layers.length - 1; i >= 0; i--) {
      const b = boxes[i];
      if (!b || s.layers[i].hidden) continue;
      if (x >= b[0] - pad && x <= b[2] + pad && y >= b[1] - pad && y <= b[3] + pad) return s.layers[i];
    }
    return null;
  }

  handleAt(mx, my) {
    const l = selected();
    if (state.mode === 'map' || !l || l.hidden || !resizable(l)) return null;
    const s = screenOf(l.id);
    if (!this.shown().includes(s)) return null;
    const b = this.boxes(s)[s.layers.indexOf(l)];
    if (!b) return null;
    const r = (HANDLE + 3) / state.view.z;
    const corners = [[b[0], b[1]], [b[2], b[1]], [b[2], b[3]], [b[0], b[3]]].map(([x, y]) => [x + s.x, y + s.y]);
    return corners.some(([cx, cy]) => Math.abs(mx - cx) <= r && Math.abs(my - cy) <= r) ? l : null;
  }

  // Where in screen s's list a title or a card at board height y goes: {to — its place among the others (skip — the
  // one being dragged, not counted), line — the height of the gap there, or null}. Items scrolled out of the screen
  // have no box: those before the first one shown are above, the rest below
  listSlot(s, y, skip, type = skip?.type || 'card', x = undefined) {
    const boxes = this.boxes(s), items = listed(s).filter((q) => q !== skip);
    const box = (q) => { const b = boxes[s.layers.indexOf(q)]; return b && b[2] >= b[0] ? b : null; };
    const first = items.findIndex(box);
    let to = 0;
    items.forEach((q, k) => {
      const b = box(q), mid = b ? (b[1] + b[3]) / 2 : k < first ? -Infinity : Infinity;
      // (a rectangle's gallery is a grid: in reading order — the rows above, then this row's cards to the left)
      if (x !== undefined && b ? b[3] < y || (b[1] <= y && (b[0] + b[2]) / 2 < x) : mid < y) to = k + 1;
    });
    to = listPlace(s, skip, to, type);  // (never above the gallery's title: its head)
    const a = items[to - 1] && box(items[to - 1]), b = items[to] && box(items[to]);
    return { to, line: x !== undefined ? null : a && b ? (a[3] + b[1]) / 2 : a ? a[3] + 8 : b ? b[1] - 8 : null };
  }

  // A title or a card dragged in its list: the place under its middle (its box when grabbed, moved with the mouse)
  dragInList(d, my, mx) {
    const l = layerById(d.id), s = l && screenOf(l.id);
    if (!s || !d.box) return;
    if (!d.moved) {
      if (Math.abs(my - d.y0) < 3 / state.view.z) return;
      d.moved = true;
    }
    d.dy = my - d.y0;
    d.dx = ROUND ? 0 : mx - d.x0;  // (a grid: sideways too)
    Object.assign(d, this.listSlot(s, (d.box[1] + d.box[3]) / 2 + d.dy, l, l.type, ROUND ? undefined : (d.box[0] + d.box[2]) / 2 + d.dx), { from: listed(s).indexOf(l) });
  }

  // ---------- Links as curves (map coordinates; Map only) ----------
  // As Figma draws prototype connections: one per link, out of the side of the layer's box that faces the screen it
  // opens (level there) into that screen's ring
  // What the overlay draws by from the engines' last pictures: the links' points, the selected and hovered layers' boxes
  // and the selected one's anchor — a key: the same — nothing to draw again
  overlayBoxes(arcs) {
    const out = [arcs.map((a) => a.pts.map((p) => p.map(Math.round).join()).join(';')).join('/')];
    for (const it of [state.sel, this.hover]) {
      const l = it && (it.type === 'layer' || it.type === 'link') && layerById(it.id), s = l && screenOf(l.id);
      if (!s) continue;
      for (let k = 0; k < (paged(s) ? pagesOf(s) : 1); k++) out.push(String(this.boxes(s, k)[s.layers.indexOf(l)]));
      out.push(String(this.anchor(l)));
    }
    return out.join('|');
  }

  computeArcs() {
    if (state.mode !== 'map') return [];
    // The point of a screen's ring (center c) that faces a point — never straight from above, where the screen's
    // name is: 40° to the side of it — and the way out of the ring there
    const arcs = [], ring = (c, toward) => {
      const up = -Math.PI / 2;
      let a = Math.atan2(toward[1] - c[1], toward[0] - c[0]);
      if (Math.abs(a - up) < 0.7) a = up + (a <= up ? -0.7 : 0.7);
      const d = [Math.cos(a), Math.sin(a)];
      if (!ROUND) {  // a rectangle: where the ray leaves the bezel, and out square to that edge
        const hw = CX + BEZEL[0] + 10, hh = CY + BEZEL[1] + 10, k = Math.min(hw / Math.abs(d[0] || 1e-6), hh / Math.abs(d[1] || 1e-6));
        return { p: [c[0] + d[0] * k, c[1] + d[1] * k], d: Math.abs(d[0]) * hh > Math.abs(d[1]) * hw ? [Math.sign(d[0]), 0] : [0, Math.sign(d[1])] };
      }
      return { p: [c[0] + d[0] * (CX + 10), c[1] + d[1] * (CX + 10)], d };
    };
    for (const k of links()) {
      const h = k.from, bb = this.boxes(h)[h.layers.indexOf(k.layer)];
      const b = bb && (bb[1] + bb[3]) / 2 > 0 && (bb[1] + bb[3]) / 2 < TH ? bb : null;  // a card off the list's view: none
      const tc = [k.to.x + CX, k.to.y + CY];
      let start;
      if (k.layer.type === 'edge' && b) {  // More: out of the button's bottom, downwards
        start = { p: [h.x + CX, h.y + Math.min(b[3], TH) + 8], d: [0, 1] };
      } else if (b) {
        const right = tc[0] >= h.x + (b[0] + b[2]) / 2;
        start = { p: [h.x + (right ? b[2] + 8 : b[0] - 8), h.y + (b[1] + b[3]) / 2], d: [right ? 1 : -1, 0] };
      } else {  // a card scrolled out of the screen: from its screen's ring
        start = ring([h.x + CX, h.y + CY], tc);
      }
      arcs.push(this.arc('link', k.layer.id, start, ring(tc, start.p)));
    }
    const m = mainScreen(), f = screens()[0];
    if (m) {  // the face and the gallery: a swipe up, a swipe down from the top edge back
      const fc = [f.x + CX, f.y + CY], mc = [m.x + CX, m.y + CY];
      arcs.push(this.arc('main', m.id, ring(fc, mc), ring(mc, fc)));
    }
    return arcs;
  }

  // A cubic from s.p (going out along s.d) into e.p (coming in against e.d); points sampled for drawing and clicks
  arc(type, id, s, e) {
    const k = Math.max(60, 0.42 * Math.hypot(e.p[0] - s.p[0], e.p[1] - s.p[1]));
    const c1 = [s.p[0] + s.d[0] * k, s.p[1] + s.d[1] * k], c2 = [e.p[0] + e.d[0] * k, e.p[1] + e.d[1] * k];
    const pts = [];
    for (let i = 0; i <= 40; i++) {
      const t = i / 40, u = 1 - t, a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
      pts.push([a * s.p[0] + b * c1[0] + c * c2[0] + d * e.p[0], a * s.p[1] + b * c1[1] + c * c2[1] + d * e.p[1]]);
    }
    return { type, id, pts, p0: s.p, p2: e.p };
  }

  // The link near the pointer (within 9 px of its line)
  arcAt(sx, sy) {
    let best = null, bd = 9;
    for (const a of this.arcs) {
      for (let i = 1; i < a.pts.length; i++) {
        const [ax, ay] = this.toView(...a.pts[i - 1]), [bx, by] = this.toView(...a.pts[i]);
        const d = segDist(sx, sy, ax, ay, bx, by);
        if (d < bd) bd = d, best = a;
      }
    }
    return best;
  }

  labelAt(sx, sy) {
    return this.labels.find((l) => sx >= l.r[0] && sx <= l.r[0] + l.r[2] && sy >= l.r[1] && sy <= l.r[1] + l.r[3]) || null;
  }

  // ---------- Mouse ----------
  bind() {
    const vp = this.viewport;
    vp.addEventListener('pointerdown', (e) => this.down(e));
    vp.addEventListener('pointermove', (e) => this.move(e));
    const up = (e) => this.up(e);
    vp.addEventListener('pointerup', up);
    vp.addEventListener('pointercancel', up);
    vp.addEventListener('pointerleave', () => { if (!this.drag) this.hover = null; });
    // Map: scroll — the map moves, ⌘ + scroll or a pinch — the zoom to the mouse. Edit, as Try: the field doesn't move;
    // ⌘ + scroll or a pinch — the screen grows or shrinks where it is
    vp.addEventListener('wheel', (e) => {
      e.preventDefault();
      const r = vp.getBoundingClientRect(), zoom = e.ctrlKey || e.metaKey;
      if (state.mode !== 'map') return zoom && this.zoomBy(Math.exp(-e.deltaY * 0.01));
      if (zoom) return this.setZoom(state.view.z * Math.exp(-e.deltaY * 0.01), e.clientX - r.left, e.clientY - r.top);
      state.view.x -= e.deltaX, state.view.y -= e.deltaY;
      this.applyView();
      this.viewMoved();
    }, { passive: false });

    // Map: a double click on a screen (or its name) — Edit on it (on that page). Edit: on a layer that opens a screen —
    // Edit on that screen
    vp.addEventListener('dblclick', (e) => {
      if (state.mode === 'edit') {
        const [mx, my] = this.toMap(e), s = this.tileAt(mx, my), l = s && this.hitLayer(s, ...this.inScreen(s, mx, my));
        if (l) this.enter(l);
        return;
      }
      if (state.mode !== 'map') return;
      const r = vp.getBoundingClientRect(), lab = this.labelAt(e.clientX - r.left, e.clientY - r.top);
      const [mx, my] = this.toMap(e), s = lab ? screenById(lab.id) : this.tileAt(mx, my);
      if (!s) return;
      select({ type: 'screen', id: s.id, page: lab ? lab.page || 0 : this.inScreen(s, mx, my)[2] });
      if (this.onEdit) this.onEdit();
    });

    // Elements dropped from the palette onto a screen: a new layer with its anchor where it was dropped
    vp.addEventListener('dragover', (e) => {
      if (!e.dataTransfer.types.includes('text/x-popka-kind')) return;
      e.preventDefault();
      const s = this.tileAt(...this.toMap(e));
      for (const [id, t] of this.tiles) t.el.classList.toggle('drop', !!s && id === s.id);
    });
    vp.addEventListener('dragleave', () => { for (const t of this.tiles.values()) t.el.classList.remove('drop'); });
    vp.addEventListener('drop', (e) => {
      for (const t of this.tiles.values()) t.el.classList.remove('drop');
      const kind = e.dataTransfer.getData('text/x-popka-kind');
      const [mx, my] = this.toMap(e), s = this.tileAt(mx, my);
      if (!KINDS[kind] || !s) return;
      e.preventDefault();
      // a part of a list dropped on a screen that is no gallery: into the gallery (main.js: panels' toGallery)
      if (['title', 'card', 'edge', 'scroll'].includes(kind) && !isGallery(s)) return this.onGallery?.(kind);
      const [x, y] = this.inScreen(s, mx, my);
      this.add(kind, s, x, y);
    });

    // Try: the live copy, the mouse is a finger; ⌘ + scroll or a pinch — the zoom (the screen grows where it is)
    this.tryBox.addEventListener('wheel', (e) => {
      e.preventDefault();
      if (e.ctrlKey || e.metaKey) this.zoomBy(Math.exp(-e.deltaY * 0.01));
    }, { passive: false });
    const live = this.tryBox.querySelector('.device');
    // The board's buttons on the bezel (lib/Halo): BOOT — a press as it goes down, held 1 s — its long press too; PWR —
    // as its chip reports it: a press once let go, held 1.5 s — the long press instead. They are not the finger
    for (const b of live.querySelectorAll('.side-btn')) {
      const name = b.dataset.btn;
      b.innerHTML = sideButton(name === 'pwr' ? 151 : 315);
      let timer = 0, long = false;
      b.addEventListener('pointerdown', (e) => {
        e.stopPropagation(), e.preventDefault();
        b.setPointerCapture(e.pointerId);
        b.classList.add('down'), (long = false);
        if (name === 'boot') this.live.command('boot');
        timer = setTimeout(() => { long = true, this.live.command(name + ' long'); }, name === 'boot' ? 1000 : 1500);
      });
      const up = (e) => {
        e.stopPropagation();
        if (!b.classList.contains('down')) return;
        b.classList.remove('down'), clearTimeout(timer);
        if (name === 'pwr' && !long) this.live.command('pwr');
      };
      b.addEventListener('pointerup', up);
      b.addEventListener('pointercancel', up);
      b.addEventListener('pointermove', (e) => e.stopPropagation());
    }
    const toBoard = (e) => {
      const r = live.getBoundingClientRect();
      return [((e.clientX - r.left) / r.width) * TW, ((e.clientY - r.top) / r.height) * TH];
    };
    live.addEventListener('pointerdown', (e) => {
      live.setPointerCapture(e.pointerId);
      this.fingerDown = true;
      this.live.touch(true, ...toBoard(e), e.timeStamp);
    });
    // Every move the browser gathered since the last frame, each with its own time (the finger's speed comes from them)
    live.addEventListener('pointermove', (e) => {
      if (!this.fingerDown) return;
      const all = e.getCoalescedEvents ? e.getCoalescedEvents() : [];
      for (const ev of all.length ? all : [e]) this.live.touch(true, ...toBoard(ev), ev.timeStamp);
    });
    const lift = () => { if (this.fingerDown) this.fingerDown = false, this.live.touch(false, 0, 0); };
    live.addEventListener('pointerup', lift);
    live.addEventListener('pointercancel', lift);
  }

  // ---------- Try: the live copy exactly where Edit and Map have the active screen, the open screen's name above it ----------
  placeTry() {
    if (state.mode !== 'try') return;
    const { x, y, z } = state.view, s = activeScreen();
    const r = this.viewport.getBoundingClientRect(), t = this.tryBox.getBoundingClientRect();
    const ox = r.left - t.left, oy = r.top - t.top;
    const left = ox + x + this.pageX(s) * z, top = oy + y + s.y * z;
    const dev = this.tryBox.querySelector('.device');
    dev.style.transform = `translate(${left}px, ${top}px) scale(${z})`;
    dev.classList.toggle('pixels', z >= 1.5);
    // The dotted field as the stage's
    const g = Math.max(8, 18 * z);
    this.tryBox.style.backgroundSize = `${g}px ${g}px`;
    this.tryBox.style.backgroundPosition = `${ox + x}px ${oy + y}px`;
    this.tryAt = [left, top], this.markSpot(s);
    this.tryName(true);
  }

  // Into Try on screen s (Edit's): the board goes to the face, then the way the watch gets to s — a swipe up to the
  // gallery, taps on the layers that open the screens on the way — each played out of sight, so Try opens on s at
  // rest, its way back as on the board. Nothing opens s — the face; false
  tryGo(s) {
    const L = this.live, way = wayTo(s);
    if (L.openScreen() !== -1) L.command('back'), L.command('face'), L.skip(1500);
    for (const step of way || []) {
      if (step.main) L.command('face off');
      else L.tap(screens().indexOf(step.from), step.from.layers.indexOf(step.layer));
      L.skip(1500);
    }
    if (way && paged(s)) for (let k = 0; k < Math.min(state.page, pagesOf(s) - 1); k++) L.command('swipe left'), L.skip(900);  // its page
    this.tryFrom = (way ? s : home()).id;
    return !!way;
  }

  // The screen open on the board in Try now (the face — the home; a stub of a card without a link — the screen the
  // card is on): leaving Try, Edit and Map go on with it
  tryScreen() {
    const open = this.live.openScreen();
    if (open === -2) return screenById(this.tryFrom) || screens()[0];
    return (open > 0 && screens()[open]) || screens()[0];
  }

  // The screen open on the board now, named as Edit and Map name the active screen (drawName, the same place and look);
  // the home — a house
  tryName(force = false) {
    if (state.mode !== 'try' || !this.tryAt) return;
    const open = this.live.openScreen(), s = open > 0 && screens()[open] ? screens()[open] : screens()[0];
    const name = open === -2 ? this.live.stubName() : s.name;  // a card without a link: its title in the middle
    if (open !== -2) this.tryFrom = s.id;  // (a stub over it: the screen it was opened from)
    const key = (open === -2 ? '' : s.id) + '|' + name;
    if (key === this.tryNameKey && !force) return;
    this.tryNameKey = key;
    const cv = document.getElementById('try-name'), b = this.tryBox.getBoundingClientRect(), dpr = window.devicePixelRatio || 1;
    const w = Math.round(b.width * dpr), h = Math.round(b.height * dpr);
    if (cv.width !== w || cv.height !== h) cv.width = w, cv.height = h;
    const c = cv.getContext('2d'), z = state.view.z, [left, top] = this.tryAt;
    c.setTransform(1, 0, 0, 1, 0, 0);
    c.clearRect(0, 0, w, h);
    c.setTransform(dpr, 0, 0, dpr, 0, 0);
    drawName(c, name, left + (TW * z) / 2, top - BEZEL[1] * z, z, s === screens()[0] && open !== -2, NAME.active);
  }

  down(e) {
    if (e.button !== 0 && e.button !== 1) return;
    this.viewport.setPointerCapture(e.pointerId);
    const r = this.viewport.getBoundingClientRect(), sx = e.clientX - r.left, sy = e.clientY - r.top;
    const [mx, my] = this.toMap(e);
    const map = state.mode === 'map';
    // The middle button or Space held — the view pans; the background: in Map it pans, in Edit it stays (a click on it
    // just drops the selection)
    const grab = { kind: 'pan', x0: e.clientX, y0: e.clientY, vx: state.view.x, vy: state.view.y, moved: false };
    const pan = map ? grab : { kind: 'still', moved: false };
    if (e.button === 1 || (this.spaceHeld && map)) return void (this.drag = map ? grab : { kind: 'still', moved: false });  // (Edit: no panning)
    const lab = this.labelAt(sx, sy);
    if (lab) {
      const s = screenById(lab.id);
      select({ type: 'screen', id: s.id, page: lab.page || 0 });
      if (map) this.drag = { kind: 'tile', id: s.id, x0: mx, y0: my, sx0: s.x, sy0: s.y, moved: false };
      return;
    }
    const h = this.handleAt(mx, my);
    if (h) return this.startResize(h, mx, my);
    const s = this.tileAt(mx, my);
    if (s) {
      const [px, py, page] = this.inScreen(s, mx, my);
      const l = map ? null : this.hitLayer(s, px, py, page);  // Map: the whole screen, never a layer
      if (l) {
        select({ type: 'layer', id: l.id });
        if (paged(s) && state.page !== page) state.page = page, emit('select');  // (on this page)
        // (a part of the gallery: the list places it — selected, not moved; a title or a card — to another place in it,
        // the list still till it is let go)
        this.drag = LISTED.has(l.type) ? { kind: 'list', id: l.id, x0: mx, y0: my, box: this.boxes(s, page)[s.layers.indexOf(l)], moved: false }
          : KINDS[l.type].fixed ? { kind: 'still', moved: false }
            : { kind: 'move', id: l.id, x0: mx, y0: my, dx0: l.dx, dy0: l.dy, moved: false };
      } else {
        select({ type: 'screen', id: s.id, page });
        this.drag = map ? { kind: 'tile', id: s.id, x0: mx, y0: my, sx0: s.x, sy0: s.y, moved: false } : pan;
      }
      return;
    }
    const a = this.arcAt(sx, sy);
    if (a) return select({ type: a.type, id: a.id });
    this.drag = { ...pan, deselect: true };
  }

  move(e) {
    const r = this.viewport.getBoundingClientRect(), sx = e.clientX - r.left, sy = e.clientY - r.top;
    const [mx, my] = this.toMap(e);
    const d = this.drag;
    if (d) {
      if (d.kind === 'still') return;
      if (d.kind === 'pan') {
        if (Math.hypot(e.clientX - d.x0, e.clientY - d.y0) > 3) d.moved = true;
        state.view.x = d.vx + e.clientX - d.x0, state.view.y = d.vy + e.clientY - d.y0;
        this.applyView();
        return this.viewMoved();
      }
      if (d.kind === 'tile') return this.moveTile(d, mx, my);
      if (d.kind === 'list') return this.dragInList(d, my, mx), (this.viewport.style.cursor = d.moved ? 'grabbing' : 'grab');
      if (d.kind === 'resize') return this.resizeTo(d, mx, my);
      return this.moveLayer(d, mx, my, e.altKey);
    }
    // Hover: a name, a corner, a layer, a link
    let cursor = 'default';
    this.hover = null;
    const lab = this.labelAt(sx, sy);
    if (lab) this.hover = { type: 'label', id: lab.id, page: lab.page || 0 }, (cursor = state.mode === 'map' ? 'grab' : 'pointer');
    else if (this.handleAt(mx, my)) cursor = 'nwse-resize';
    else {
      const s = this.tileAt(mx, my), l = s && state.mode !== 'map' && this.hitLayer(s, ...this.inScreen(s, mx, my));
      if (l) this.hover = { type: 'layer', id: l.id }, (cursor = LISTED.has(l.type) ? 'grab' : KINDS[l.type].fixed ? 'pointer' : 'move');
      else if (s && state.mode === 'map') cursor = 'grab';
      else if (!s) {
        const a = this.arcAt(sx, sy);
        if (a) this.hover = { type: a.type, id: a.id }, (cursor = 'pointer');
        else if (state.mode === 'map') cursor = 'grab';
      }
    }
    this.viewport.style.cursor = this.spaceHeld && state.mode === 'map' ? 'grab' : cursor;
  }

  up() {
    const d = this.drag;
    this.drag = null, this.guides = [];
    if (d && (d.kind === 'pan' || d.kind === 'still') && d.deselect && !d.moved) select(null);
    if (d && d.kind === 'list') {  // let go: at its new place (the list scrolls to it), or — a click — scrolled to it
      if (d.moved && d.to !== d.from) moveInList(d.id, d.to);
      else if (!d.moved) this.reveal();
    }
  }

  add(kind, s, x, y) {
    const probe = newLayer(kind, null, s);
    let at = {};
    if (x !== undefined) {
      const [bx, by] = (this.live.bases || [])[KIND_INDEX[kind]] || [CX, CY];
      at = { dx: Math.round(x - bx), dy: Math.round(y - by) };
    }
    const l = newLayer(kind, at, s);
    const shifts = roomFor(kind, s);
    // a title or a card dropped onto a list: into the list where it was dropped (a click — at its end); the stars —
    // the background: under everything
    const i = kind === 'stars' ? 0 : LISTED.has(kind) && y !== undefined && listed(s).length ? listIndex(s, this.listSlot(s, y, null, kind).to, kind)
      : kind === 'title' && listed(s).length ? listIndex(s, listed(s).length, kind) : s.layers.length;  // (a list's first title: its head, on top)
    change(() => { s.layers.splice(i, 0, l), shifts(); }, { structure: true });
    select({ type: 'layer', id: l.id });
  }

  moveTile(d, mx, my) {
    if (!d.moved) {
      if (Math.hypot(mx - d.x0, my - d.y0) < 3 / state.view.z) return;
      d.moved = true;
      beginGesture();
    }
    let x = Math.round(d.sx0 + mx - d.x0), y = Math.round(d.sy0 + my - d.y0);
    // Snap to the other screens' rows and columns
    const r = 10 / state.view.z;
    for (const o of screens()) {
      if (o.id === d.id) continue;
      if (Math.abs(o.y - y) < r) y = o.y;
      if (Math.abs(o.x - x) < r) x = o.x;
    }
    change((p) => { const s = p.screens.find((q) => q.id === d.id); s.x = x, s.y = y; }, { history: false });
  }

  moveLayer(d, mx, my, free) {
    const l = layerById(d.id);
    if (!l) return;
    if (RADIAL.has(l.type)) return this.moveRadial(d, l, mx, my, free);
    if (!d.moved) {
      if (Math.hypot(mx - d.x0, my - d.y0) < 2 / state.view.z) return;
      d.moved = true;
      beginGesture();
    }
    let dx = d.dx0 + Math.round(mx - d.x0), dy = d.dy0 + Math.round(my - d.y0);
    // Snap the anchor to the screen's center lines and to HaloWatch's place (dx, dy = 0); Alt — no snapping
    const s = screenOf(l.id), [bx, by] = this.base(l);
    this.guides = [];
    if (!free) {
      const r = SNAP / state.view.z + 1;
      const sx = [[CX - bx, CX], [0, bx]].find(([v]) => Math.abs(dx - v) <= r);
      const sy = [[CY - by, CY], [0, by]].find(([v]) => Math.abs(dy - v) <= r);
      if (sx) dx = sx[0], this.guides.push(['v', sx[1], s]);
      if (sy) dy = sy[0], this.guides.push(['h', sy[1], s]);
    }
    if (dx === l.dx && dy === l.dy) return;
    change(() => { l.dx = dx, l.dy = dy; }, { history: false });
  }

  // A curved layer or a ring dragged: round the screen's middle (its angle) and to or from it (its radius). Its outer
  // edge snaps right onto the screen's edge — as far out as it goes, nothing cut (where it reaches: the engine's
  // studio_radii, by its pixels) — or as far from the edge as another such layer on the screen is; its middle snaps to
  // 12, 3, 6 and 9 o'clock. The guides: the circle it snapped to, the line through the middle. Alt — no snapping
  moveRadial(d, l, mx, my, free) {
    const s = screenOf(l.id);
    if (!d.moved) {
      if (Math.hypot(mx - d.x0, my - d.y0) < 2 / state.view.z) return;
      d.moved = true;
      beginGesture();
      const e = this.engineOf(s), i = s.layers.indexOf(l);
      const radii = e && e.M ? e.radii(screens().indexOf(s), s.layers.length) : [];
      d.r0 = l.radius ?? KINDS[l.type].radius ?? 200, d.a0 = l.angle ?? 0;
      // its outer edge past its radius: the same at any radius — but past 120° from 12 o'clock the letters turn over
      // (the bottom reads left to right: widgets.h), so one for each way, the other measured once it is drawn so
      d.outs = {};
      if (radii[i]) d.outs[flipped(l.type, d.a0)] = radii[i][1] - d.r0;
      d.gaps = radii.map((r, j) => (j !== i && r && !s.layers[j].hidden ? CX - r[1] : null)).filter((g) => g != null && g >= 0 && g < 150);
      d.c = [s.x + CX + l.dx, s.y + CX + l.dy];  // (its middle on the map)
      d.p0 = [d.x0 - d.c[0], d.y0 - d.c[1]];
    }
    const px = mx - d.c[0], py = my - d.c[1];
    let r = d.r0 + Math.hypot(px, py) - Math.hypot(...d.p0);
    let a = d.a0 + (Math.atan2(px, -py) - Math.atan2(d.p0[0], -d.p0[1])) * 180 / Math.PI;
    a = ((a % 360) + 540) % 360 - 180;  // (-180 … 180, as Rotation keeps it)
    this.guides = [];
    const tol = SNAP / state.view.z + 1;
    if (TURNS.has(l.type)) {
      const tolA = Math.max(2.5, (tol / Math.max(40, r)) * 180 / Math.PI);
      const q = free ? null : [0, 90, 180, -90].find((v) => Math.abs(((a - v + 540) % 360) - 180) <= tolA);
      if (q != null) a = q, this.guides.push([q % 180 ? 'h' : 'v', CX, s]);
      else a = Math.round(a * 10) / 10;
    }
    const way = flipped(l.type, a);
    d.out = d.outs[way] ?? d.outs[!way] ?? null;
    if (!free && d.out != null) {
      let best = null;
      for (const g of [0, ...d.gaps]) {
        const off = r + d.out - (CX - g);
        if (Math.abs(off) <= tol && (!best || Math.abs(off) < Math.abs(best.off))) best = { g, off };
      }
      if (best) r = Math.floor((CX - best.g - d.out) * 10) / 10, this.guides.push(['c', CX - best.g, s]);  // (never past it)
    }
    d.snap = this.guides.some((g) => g[0] === 'c') ? CX - this.guides.find((g) => g[0] === 'c')[1] : null;
    r = Math.max(20, Math.min(260, d.snap != null ? r : Math.round(r * 10) / 10));
    if (r === l.radius && (!TURNS.has(l.type) || a === l.angle)) return;
    change(() => { l.radius = r; if (TURNS.has(l.type)) l.angle = a; }, { history: false });
    // Turned over for the first time: once drawn so, its edge measured that way and the snap made again
    if (!(way in d.outs)) {
      d.last = [mx, my, free];
      requestAnimationFrame(() => requestAnimationFrame(() => {
        const e = this.engineOf(s), i = s.layers.indexOf(l), rr = e && e.M && i >= 0 ? e.radii(screens().indexOf(s), s.layers.length)[i] : null;
        if (!rr || way in d.outs || flipped(l.type, l.angle ?? 0) !== way) return;
        d.outs[way] = rr[1] - l.radius;
        if (this.drag === d) return this.moveRadial(d, l, ...d.last);
        if (d.snap != null) change(() => { l.radius = Math.floor((CX - d.snap - d.outs[way]) * 10) / 10; }, { history: false });  // (let go already)
      }));
    }
  }

  // Resizing by a corner: the size follows the distance from the anchor (the layer grows around it)
  startResize(l, mx, my) {
    const s = screenOf(l.id), [ax, ay] = this.anchor(l);
    beginGesture();
    this.drag = {
      kind: 'resize', id: l.id, ax: ax + s.x, ay: ay + s.y, size0: l.size, scale0: l.scale,
      r0: Math.max(4, Math.hypot(mx - ax - s.x, my - ay - s.y)),
    };
  }

  resizeTo(d, mx, my) {
    const l = layerById(d.id), f = Math.hypot(mx - d.ax, my - d.ay) / d.r0;
    change(() => {
      if ('scale' in l) l.scale = Math.max(0.3, Math.min(3, Math.round(d.scale0 * f * 100) / 100));
      else l.size = Math.max(8, Math.min(300, Math.round(d.size0 * f)));
    }, { history: false });
  }

  // Edit, a screen with pages: its name above it is the switch (‹ none at the first page, › none at the last). It
  // stands over the screen's place — where the page sliding in will be — so it doesn't move with the pages
  placePager() {
    const s = state.mode === 'edit' && activeScreen(), on = !!s;
    this.pager.hidden = !on;
    if (!on) return;
    const pg = paged(s), n = pg ? pagesOf(s) : 1, k = Math.min(state.page, n - 1);
    const z = state.view.z, vx = this.slide ? this.slideTo : state.view.x;
    const left = Math.round(vx + (this.pageX(s) + CX) * z) + 'px', top = Math.round(state.view.y + s.y * z - (10 + BEZEL[1]) * z - 38) + 'px';
    if (this.pager.style.left !== left) this.pager.style.left = left;
    if (this.pager.style.top !== top) this.pager.style.top = top;
    const key = [s.id, k, n, this.rev, this.fwd.length, this.came].join('|');
    if (key === this.pagerKey) return;  // (its words and arrows: only when something changed)
    this.pagerKey = key;
    const $ = (id) => document.getElementById(id), back = this.backOf(s), fwd = screenById(this.fwd[this.fwd.length - 1]);
    $('pager-text').textContent = pg ? pageName(s, k) : s.name;
    $('pager-home').toggleAttribute('hidden', s !== home());  // (an svg: no .hidden of its own)
    $('pager-count').textContent = pg ? `${k + 1} / ${n}` : '';
    const prev = $('page-prev'), next = $('page-next');
    prev.disabled = !(k > 0 || back), next.disabled = !(k < n - 1 || fwd);
    prev.title = k > 0 ? 'Previous page (←)' : back ? `${back === home() ? 'Home' : 'Back'}: ${back.name} (←)` : '';
    next.title = k < n - 1 ? 'Next page (→)' : fwd ? `Forward: ${fwd.name} (→)` : '';
  }

  // ---------- The overlay: names, links, frames, handles, the anchor, guides ----------
  // Drawn again only when something it shows changed: the view, the mode, the selection, what the mouse is over, the
  // guides, a list's drag, anything in the project — or, after a copy of the engine put a new picture up, the boxes it
  // draws by (the selected and hovered layers', the links'). Otherwise the last drawing stays (each display frame — 120 a
  // second on this Mac — it was the whole window's canvas again)
  drawOverlay() {
    this.sizeOverlay();  // (the window moved to a screen of another pixel density: no resize, the canvas is resized here)
    if (this.slide) this.slide(performance.now());
    this.placePager();
    const c = this.octx, dpr = window.devicePixelRatio || 1;
    const sel = state.sel, hov = this.hover, d0 = this.drag, v = state.view;
    const key = [state.mode, v.x, v.y, v.z, dpr, this.overlay.width, this.overlay.height, state.screen, state.page, this.rev,
      sel ? sel.type + sel.id : '', hov ? hov.type + hov.id + (hov.page || 0) : '', JSON.stringify(this.guides),
      d0 && d0.kind === 'list' ? [d0.moved, d0.dy, d0.to, d0.line].join() : '', d0 ? d0.kind : ''].join('|');
    if (key === this.oKey && paints === this.oPaints) return;
    const arcs = state.mode === 'try' ? [] : this.computeArcs();
    const boxes = key === this.oKey ? this.overlayBoxes(arcs) : '';
    this.oPaints = paints;
    if (key === this.oKey && boxes === this.oBoxes) return;  // (a new picture, but nothing the overlay shows moved)
    this.oKey = key, this.oBoxes = boxes || this.overlayBoxes(arcs);
    c.setTransform(1, 0, 0, 1, 0, 0);
    c.clearRect(0, 0, this.overlay.width, this.overlay.height);
    if (state.mode === 'try') return;
    c.setTransform(dpr, 0, 0, dpr, 0, 0);
    const V = (x, y) => this.toView(x, y);

    // Links under everything else on the overlay, as Figma's: blue, a dot at the layer, an arrow at the screen
    this.arcs = arcs;
    for (const a of this.arcs) {
      const on = sel && sel.type === a.type && sel.id === a.id, hot = hov && hov.type === a.type && hov.id === a.id;
      c.globalAlpha = on || hot ? 1 : 0.8;
      c.strokeStyle = c.fillStyle = '#4c8dff';
      c.lineWidth = on ? 2.5 : 1.75;
      c.beginPath();
      a.pts.forEach(([x, y], i) => (i ? c.lineTo(...V(x, y)) : c.moveTo(...V(x, y))));
      c.stroke();
      const [ex, ey] = V(...a.p2), [qx, qy] = V(...a.pts[a.pts.length - 4]), ang = Math.atan2(ey - qy, ex - qx), al = on ? 12 : 10;
      c.beginPath();
      c.moveTo(ex, ey);
      c.lineTo(ex - al * Math.cos(ang - 0.5), ey - al * Math.sin(ang - 0.5));
      c.lineTo(ex - al * Math.cos(ang + 0.5), ey - al * Math.sin(ang + 0.5));
      c.closePath(), c.fill();
      const [sx, sy] = V(...a.p0);
      c.beginPath();
      c.arc(sx, sy, on ? 4.5 : 3.5, 0, Math.PI * 2);
      c.fillStyle = '#fff', c.fill();
      c.lineWidth = 1.5, c.stroke();
      c.globalAlpha = 1;
    }

    // Screen names, centered above the screens; the home — a house and its name (Map: drag the name to move a screen).
    // The same in every mode: the active screen's lighter than the others (Map), under the mouse — lighter still
    // A screen with pages: an arrow from each page to the next (a swipe), as the links; a name over each page — the
    // screen's own (and its city) over the first, the city over the others; the picked page's lighter
    this.labels = [];
    const z = state.view.z;
    for (const s of this.shown()) {
      const [cx, top] = V(s.x + CX, s.y - BEZEL[1]), house = s === screens()[0];
      const color = (k) => (hov && hov.type === 'label' && hov.id === s.id && (hov.page || 0) === k ? NAME.hover
        : s.id === state.screen && (!paged(s) || Math.min(state.page, pagesOf(s) - 1) === k) ? NAME.active : NAME.other);
      if (!paged(s)) {
        if (state.mode === 'edit' && s.id === state.screen) continue;  // (its name is the pager: placePager)
        this.labels.push({ id: s.id, page: 0, r: drawName(c, s.name, cx, top, z, house, color(0)) });
      } else {
        const n = pagesOf(s);
        for (let k = 0; k < n; k++) {
          const [px, pt] = V(s.x + k * PAGE_STEP + CX, s.y - BEZEL[1]), city = pageName(s, k);
          const asPager = state.mode === 'edit' && s.id === state.screen && Math.min(state.page, n - 1) === k;  // (placePager)
          if (!asPager) this.labels.push({ id: s.id, page: k, r: k ? drawName(c, city, px, pt, z, false, color(k)) : drawName(c, s.name, px, pt, z, house, color(0), city) });
          if (k + 1 === n) continue;
          const [ax, ay] = V(s.x + k * PAGE_STEP + TW + 10, s.y + CY), [bx] = V(s.x + (k + 1) * PAGE_STEP - 10, 0);
          c.globalAlpha = 0.8, c.strokeStyle = c.fillStyle = '#4c8dff', c.lineWidth = 1.75;
          c.beginPath(), c.moveTo(ax, ay), c.lineTo(bx, ay), c.stroke();
          c.beginPath(), c.moveTo(bx, ay), c.lineTo(bx - 10 * Math.cos(0.5), ay - 10 * Math.sin(0.5)), c.lineTo(bx - 10 * Math.cos(0.5), ay + 10 * Math.sin(0.5));
          c.closePath(), c.fill();
          c.beginPath(), c.arc(ax, ay, 3.5, 0, Math.PI * 2), c.fillStyle = '#fff', c.fill(), c.lineWidth = 1.5, c.stroke();
          c.globalAlpha = 1;
        }
      }
    }

    // Layer frames: hover, then the selected one with its corners and anchor (a layer over the whole screen — round, on
    // the screen's edge)
    // (on a screen with pages — on every page, where that page has it; the first page's frame is returned)
    const frame = (l, color, width) => {
      const s = screenOf(l.id);
      if (!s || !this.shown().includes(s)) return null;
      let first = null;
      for (let k = 0; k < (paged(s) ? pagesOf(s) : 1); k++) {
        const b = this.boxes(s, k)[s.layers.indexOf(l)];
        if (!b) continue;
        const ox = s.x + k * PAGE_STEP, [x0, y0] = V(ox + b[0], s.y + b[1]), [x1, y1] = V(ox + b[2], s.y + b[3]);
        c.strokeStyle = color, c.lineWidth = width;
        if (KINDS[l.type].round && ROUND) c.beginPath(), c.arc((x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0 - width) / 2, 0, Math.PI * 2), c.stroke();
        else c.strokeRect(x0, y0, x1 - x0, y1 - y0);
        if (!k) first = [x0, y0, x1, y1];
      }
      return first;
    };
    if (hov && hov.type === 'layer' && !(sel && sel.type === 'layer' && sel.id === hov.id) && !this.drag) {
      const l = layerById(hov.id);
      if (l) frame(l, 'rgba(76,141,255,0.7)', 1);
    }
    if (sel && sel.type === 'link') {
      const l = layerById(sel.id);
      if (l) frame(l, 'rgba(255,255,255,0.7)', 1.5);
    }
    for (const [dir, v, s] of this.guides) {
      c.strokeStyle = '#ff4fa3', c.lineWidth = 1;
      c.beginPath();
      if (dir === 'c') {  // (a circle round the screen's middle: where a curved layer's edge snapped)
        const [ox, oy] = V(s.x + CX, s.y + CX), rr = V(s.x + CX + v, s.y + CX)[0] - ox;
        c.arc(ox, oy, rr, 0, Math.PI * 2), c.stroke();
        continue;
      }
      if (dir === 'v') c.moveTo(...V(s.x + v, s.y)), c.lineTo(...V(s.x + v, s.y + TH));
      else c.moveTo(...V(s.x, s.y + v)), c.lineTo(...V(s.x + TW, s.y + v));
      c.stroke();
    }
    const d = this.drag;
    if (d && d.kind === 'list' && d.moved && d.box) {  // a title or a card dragged in its list: it, and where it goes
      const s = screenOf(d.id), b = d.box;
      if (s) {
        const [x0, y0] = V(s.x + b[0] + d.dx, s.y + b[1] + d.dy), [x1, y1] = V(s.x + b[2] + d.dx, s.y + b[3] + d.dy);
        c.fillStyle = 'rgba(76,141,255,0.12)', c.fillRect(x0, y0, x1 - x0, y1 - y0);
        c.setLineDash([4, 3]), c.strokeStyle = '#4c8dff', c.lineWidth = 1.5, c.strokeRect(x0, y0, x1 - x0, y1 - y0), c.setLineDash([]);
        if (d.to !== d.from && d.line != null) {
          const [ax, ly] = V(s.x + Math.min(b[0], CX - 160), s.y + d.line), [bx] = V(s.x + Math.max(b[2], CX + 160), 0);
          c.strokeStyle = c.fillStyle = '#4c8dff', c.lineWidth = 2;
          c.beginPath(), c.moveTo(ax, ly), c.lineTo(bx, ly), c.stroke();
          for (const x of [ax, bx]) c.beginPath(), c.arc(x, ly, 3.5, 0, Math.PI * 2), c.fill();
        }
      }
    }
    const l = selected();
    if (l && !l.hidden) {
      const r = frame(l, '#4c8dff', 1.5);
      if (r) {
        const hs = HANDLE;
        // Corners — in Edit (in Map a layer is only shown: it can't be grabbed there)
        const corners = resizable(l) && state.mode !== 'map';
        for (const [hx, hy] of corners ? [[r[0], r[1]], [r[2], r[1]], [r[2], r[3]], [r[0], r[3]]] : []) {
          c.fillStyle = '#fff', c.fillRect(hx - hs / 2, hy - hs / 2, hs, hs);
          c.strokeStyle = '#4c8dff', c.lineWidth = 1, c.strokeRect(hx - hs / 2, hy - hs / 2, hs, hs);
        }
        const s = screenOf(l.id), [ax, ay] = this.anchor(l), [px, py] = V(s.x + ax, s.y + ay), a = 4;
        c.beginPath();
        c.moveTo(px - a, py), c.lineTo(px + a, py), c.moveTo(px, py - a), c.lineTo(px, py + a);
        c.stroke();
      }
    }
  }
}

// A layer with a size: a font size or a scale (the dots have neither)
const resizable = (l) => !KINDS[l.type].fixed && ('size' in l || 'scale' in l);

// A screen's name above its bezel (top: the screen's top in the window; the bezel is 10 board px — thicker at a bigger
// zoom) with a gap, centered at cx; the home — a house before it. Where it is: [x, y, w, h]
const NAME = { active: '#c9cad0', other: '#85868d', hover: '#ececee', page: '#6c6d74' };
// sub — dimmer, after it (a paged screen's first city)
function drawName(c, name, cx, top, z, house, color, sub = '') {
  c.font = '600 12.5px "Google Sans Flex", system-ui, sans-serif';
  c.textBaseline = 'alphabetic';
  const w1 = c.measureText(name).width;
  c.font = '500 12.5px "Google Sans Flex", system-ui, sans-serif';
  const w2 = sub ? c.measureText(' · ' + sub).width : 0;
  const w = w1 + w2, icon = house ? 18 : 0, x = cx - (w + icon) / 2, y = top - 10 * z - 14;
  if (house) drawHouse(c, x, y - 11, 12, color);
  c.font = '600 12.5px "Google Sans Flex", system-ui, sans-serif';
  c.fillStyle = color;
  c.fillText(name, x + icon, y);
  if (sub) c.font = '500 12.5px "Google Sans Flex", system-ui, sans-serif', c.fillStyle = NAME.page, c.fillText(' · ' + sub, x + icon + w1, y);
  return [x - 4, y - 16, w + icon + 8, 22];
}

// A small house (the home screen) with its left edge at x, top at y
function drawHouse(c, x, y, size, color) {
  c.save();
  c.strokeStyle = color, c.lineWidth = 1.5, c.lineJoin = c.lineCap = 'round';
  c.beginPath();
  c.moveTo(x, y + size * 0.48), c.lineTo(x + size / 2, y + size * 0.08), c.lineTo(x + size, y + size * 0.48);
  c.moveTo(x + size * 0.18, y + size * 0.36), c.lineTo(x + size * 0.18, y + size);
  c.lineTo(x + size * 0.82, y + size), c.lineTo(x + size * 0.82, y + size * 0.36);
  c.stroke();
  c.restore();
}

function segDist(px, py, ax, ay, bx, by) {
  const dx = bx - ax, dy = by - ay, L = dx * dx + dy * dy || 1;
  const t = Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / L));
  return Math.hypot(px - ax - t * dx, py - ay - t * dy);
}
