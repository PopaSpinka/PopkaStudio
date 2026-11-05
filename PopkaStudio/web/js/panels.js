// panels.js — Elements (three folders and the shelf of the open one: what can go on a screen), Screens, Functions (the
// watch's: Always On, the buttons, Wi-Fi, Bluetooth — each with a switch; in the left card, folds), Layers (of the active screen) and Properties (of
// a layer, a screen, a link, the way to the gallery or a function: what can be set, no more).
import {
  state, screens, home, screenById, activeScreen, layers, selected, select, change, KINDS, layerById, screenOf,
  linkedScreens, newScreen, emit, PAGED, sync as saved, mainScreen, canLink, targetsFor, ICONS, TAPS, PAGES, paged,
  pagesOf, pageName, weatherPaged,
  LISTED, listed, moveInList, newLayer, FACE_SCREEN, FUNCTIONS, fnOf, setFn, ZONES, timeOf, setTime, citiesOf, unitsOf,
  setWeather, WX_MAX, moveCity, isGallery, galleryFor, SECTIONS, sectionParts, addSection, SECTION_PATH, titleOf, subGallery, keepHead, gamesGallery, newList, stepsSet, STEP_FAMILY,
  savedFaces, saveFace, removeFace, faceLayers,
  FACE_PRESETS, FOUR_SIDES, CURVES, INFOS, PIXEL_ART,
} from './state.js';
import { thumb } from './previews.js';

// What a tile adds, made apart from the project (for its picture): layers on a screen of their own
const apart = (parts) => () => {
  const s = { id: 'pv', layers: [] };
  for (const [kind, look] of parts) s.layers.push(newLayer(kind, look, s));
  return s.layers;
};

const $ = (id) => document.getElementById(id);
const h = (tag, attrs = {}, ...kids) => {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (v === undefined || v === null || v === false) continue;
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else if (k === 'html') e.innerHTML = v;
    else e.setAttribute(k, v === true ? '' : v);
  }
  for (const k of kids) if (k != null) e.append(k);
  return e;
};

const ICON = {
  clock: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="6"/><path d="M8 4.8V8l2.2 1.4"/></svg>',
  watch: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="4.3"/><path d="M6 3.9 6.5 1.5h3l.5 2.4M6 12.1l.5 2.4h3l.5-2.4M8 6v2l1.3.8"/></svg>',
  apps: '<svg viewBox="0 0 16 16"><circle cx="4" cy="4" r="1.6"/><circle cx="8" cy="4" r="1.6"/><circle cx="12" cy="4" r="1.6"/><circle cx="4" cy="8" r="1.6"/><circle cx="8" cy="8" r="1.6"/><circle cx="12" cy="8" r="1.6"/><circle cx="4" cy="12" r="1.6"/><circle cx="8" cy="12" r="1.6"/><circle cx="12" cy="12" r="1.6"/></svg>',
  date: '<svg viewBox="0 0 16 16"><rect x="2.5" y="3.5" width="11" height="10" rx="2"/><path d="M2.5 6.5h11M5.5 2v3M10.5 2v3"/></svg>',
  weather: '<svg viewBox="0 0 16 16"><path d="M4.5 12.5h7a2.5 2.5 0 0 0 0-5 3.5 3.5 0 0 0-6.8.6A2.2 2.2 0 0 0 4.5 12.5z"/></svg>',
  stars: '<svg viewBox="0 0 16 16"><path d="M6 2.5l.9 2.3 2.3.9-2.3.9L6 8.9l-.9-2.3-2.3-.9 2.3-.9zM11.5 8l.6 1.4 1.4.6-1.4.6-.6 1.4-.6-1.4-1.4-.6 1.4-.6zM4.5 11.5h0M13 3.5h0" /></svg>',
  parrot: '<svg viewBox="0 0 16 16"><path d="M11 5.2a3.4 3.4 0 0 0-6.3 1.6c0 .6.1 1.1.4 1.6L2 13l4.4-2a4.6 4.6 0 0 0 6.1-4.2"/><path d="M11 5.2l2.6.8-2 2"/><circle cx="8.9" cy="5.6" r=".5"/></svg>',
  steps: '<svg viewBox="0 0 16 16"><path d="M4.6 2.2c1.5 0 2.2 1.6 2.2 3.4S6 8.8 4.6 8.8 2.6 7.3 2.6 5.5s.6-3.3 2-3.3zM3.2 10.6h2.9l-.2 1.6a1.3 1.3 0 0 1-2.6 0zM11.4 4.8c1.4 0 2 1.5 2 3.3s-.6 3.3-2 3.3-2.2-1.4-2.2-3.2.7-3.4 2.2-3.4zM10 13.2h2.9l-.2 1.4a1.3 1.3 0 0 1-2.6 0z"/></svg>',
  curve: '<svg viewBox="0 0 16 16"><path d="M2.5 7.5a6.5 6.5 0 0 1 11 0M4.5 10.5a4 4 0 0 1 7 0"/></svg>',
  stepring: '<svg viewBox="0 0 16 16"><path d="M8 2a6 6 0 1 1-5.7 7.9"/></svg>',
  batring: '<svg viewBox="0 0 16 16"><path d="M8 2a6 6 0 1 1-5.7 7.9"/><rect x="6.2" y="6" width="3.6" height="5" rx=".8"/><path d="M7.2 5.2h1.6"/></svg>',
  weekarc: '<svg viewBox="0 0 16 16"><circle cx="8" cy="5" r="2"/><path d="M2.5 9.5h.01M5 6.5h.01M11 6.5h.01M13.5 9.5h.01"/></svg>',
  infoline: '<svg viewBox="0 0 16 16"><path d="M2.5 8h3M7.5 8h6"/></svg>',
  forecast: '<svg viewBox="0 0 16 16"><path d="M3 11a2 2 0 0 1 .4-4 3 3 0 0 1 5.8.9A1.6 1.6 0 1 1 9.6 11z"/><path d="M11.5 3.5v1M14 6h-1M13.3 4.2l-.7.7"/></svg>',
  dateframe: '<svg viewBox="0 0 16 16"><path d="M4 4.5h8M4 11.5h8M6 2.5h4M6 13.5h4"/></svg>',
  dotclock: '<svg viewBox="0 0 16 16"><path d="M3 5h.01M3 8h.01M3 11h.01M6 5h.01M6 11h.01M8 6.5h.01M8 9.5h.01M10 5h.01M10 8h.01M10 11h.01M13 5h.01M13 8h.01M13 11h.01"/></svg>',
  stnum: '<svg viewBox="0 0 16 16"><path d="M3 12V4l-1.5 1.2M6.5 5.5a2 2 0 1 1 3.2 1.6L6.5 12h3.6M12 4.5h2.3l-1.6 2.6a1.9 1.9 0 1 1-1.6 3"/></svg>',
  stcap: '<svg viewBox="0 0 16 16"><path d="M2.5 6.5h11M2.5 9.5h7"/></svg>',
  stfoot: '<svg viewBox="0 0 16 16"><path d="M3 11.5h10M5.5 8.5h5"/></svg>',
  stchart: '<svg viewBox="0 0 16 16"><path d="M3 13V9M6.3 13V5M9.7 13V7.5M13 13V3.5"/></svg>',
  arc: '<svg viewBox="0 0 16 16"><path d="M2.5 10a5.5 5.5 0 0 1 11 0M8 4.5v2"/></svg>',
  city: '<svg viewBox="0 0 16 16"><path d="M8 14s-4.5-4.2-4.5-7.5a4.5 4.5 0 0 1 9 0C12.5 9.8 8 14 8 14z"/><circle cx="8" cy="6.5" r="1.6"/></svg>',
  wxicon: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="2.8"/><path d="M8 1.8v1.5M8 12.7v1.5M1.8 8h1.5M12.7 8h1.5M3.6 3.6l1 1M11.4 11.4l1 1M3.6 12.4l1-1M11.4 4.6l1-1"/></svg>',
  wxtemp: '<svg viewBox="0 0 16 16"><path d="M6.5 9.6V3.5a1.5 1.5 0 0 1 3 0v6.1a3 3 0 1 1-3 0z"/></svg>',
  feels: '<svg viewBox="0 0 16 16"><path d="M5 9.6V3.5a1.5 1.5 0 0 1 3 0v6.1a3 3 0 1 1-3 0z"/><path d="M10.5 5.5c.9-.7 1.8-.7 2.7 0M10.5 8.5c.9-.7 1.8-.7 2.7 0"/></svg>',
  wxhilo: '<svg viewBox="0 0 16 16"><path d="M5 13V3M2.8 5.2 5 3l2.2 2.2M11 3v10M8.8 10.8 11 13l2.2-2.2"/></svg>',
  hourly: '<svg viewBox="0 0 16 16"><rect x="1.5" y="4.5" width="13" height="7" rx="3.5"/><path d="M5 8h0M8 8h0M11 8h0" stroke-width="2.4"/></svg>',
  dots: '<svg viewBox="0 0 16 16"><path d="M3 10.5h0M6.3 11.8h0M9.7 11.8h0M13 10.5h0" stroke-width="2.6"/></svg>',
  rain: '<svg viewBox="0 0 16 16"><path d="M4.5 10h7a2.5 2.5 0 0 0 0-5 3.5 3.5 0 0 0-6.8.6A2.2 2.2 0 0 0 4.5 10z"/><path d="M6 12.3l-.6 1.4M9 12.3l-.6 1.4M12 12.3l-.6 1.4"/></svg>',
  newScreen: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="5.8"/><path d="M8 5.5v5M5.5 8h5"/></svg>',
  whole: '<svg viewBox="0 0 16 16"><rect x="2.5" y="2.5" width="4.5" height="4.5" rx="1"/><rect x="9" y="2.5" width="4.5" height="4.5" rx="1"/><rect x="2.5" y="9" width="4.5" height="4.5" rx="1"/><rect x="9" y="9" width="4.5" height="4.5" rx="1"/></svg>',
  empty: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="5.8" stroke-dasharray="2.3 2"/></svg>',
  home: '<svg viewBox="0 0 16 16"><path d="M2.5 7.5 8 3l5.5 4.5M4 6.5V13h8V6.5"/></svg>',
  screen: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="5.8"/></svg>',
  link: '<svg viewBox="0 0 16 16"><path d="M2.5 11.5C4 6 9 4 13.5 5.5M10.5 3.2l3 2.3-2.6 2.7"/></svg>',
  up: '<svg viewBox="0 0 16 16"><path d="M4 10l4-4 4 4"/></svg>',
  down: '<svg viewBox="0 0 16 16"><path d="M4 6l4 4 4-4"/></svg>',
  eye: '<svg viewBox="0 0 16 16"><path d="M1.5 8s2.4-4.5 6.5-4.5S14.5 8 14.5 8 12.1 12.5 8 12.5 1.5 8 1.5 8z"/><circle cx="8" cy="8" r="2"/></svg>',
  eyeOff: '<svg viewBox="0 0 16 16"><path d="M2 2l12 12M6.6 3.7A6.9 6.9 0 0 1 8 3.5C12.1 3.5 14.5 8 14.5 8a11 11 0 0 1-1.9 2.5M10.1 10.4A2 2 0 0 1 6.6 7.5M4.2 5.1A11 11 0 0 0 1.5 8S3.9 12.5 8 12.5a6.4 6.4 0 0 0 2.6-.6"/></svg>',
  trash: '<svg viewBox="0 0 16 16"><path d="M3 4.5h10M6.5 4.5V3h3v1.5M4.5 4.5l.6 8.5h5.8l.6-8.5"/></svg>',
  chev: '<svg viewBox="0 0 16 16"><path d="M6 3.5 10.5 8 6 12.5"/></svg>',
  plus: '<svg viewBox="0 0 16 16"><path d="M8 3v10M3 8h10"/></svg>',
  title: '<svg viewBox="0 0 16 16"><path d="M3.5 4h9M8 4v8.5"/></svg>',
  card: '<svg viewBox="0 0 16 16"><rect x="1.5" y="4" width="13" height="8" rx="4"/><circle cx="5.5" cy="8" r="1.6"/></svg>',
  edge: '<svg viewBox="0 0 16 16"><path d="M3 9.5h10c0 2.2-2.2 4-5 4s-5-1.8-5-4z"/></svg>',
  scroll: '<svg viewBox="0 0 16 16"><path d="M11.5 3.5a6 6 0 0 1 0 9" stroke-width="1.2" opacity=".5"/><path d="M13 6a6 6 0 0 1 0 4" stroke-width="2.2"/></svg>',
  gallery: '<svg viewBox="0 0 16 16"><rect x="2.5" y="2.5" width="11" height="4" rx="2"/><rect x="2.5" y="9.5" width="11" height="4" rx="2"/></svg>',
  functions: '<svg viewBox="0 0 16 16"><path d="M3 4.5h5M11 4.5h2M3 11.5h2M8 11.5h5"/><circle cx="9.5" cy="4.5" r="1.5"/><circle cx="6.5" cy="11.5" r="1.5"/></svg>',
  aod: '<svg viewBox="0 0 16 16"><path d="M13 9.6A5.5 5.5 0 1 1 6.4 3a4.3 4.3 0 0 0 6.6 6.6z"/></svg>',
  boot: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="5.8"/><circle cx="8" cy="8" r="2"/></svg>',
  pwr: '<svg viewBox="0 0 16 16"><path d="M8 2.5V8M4.6 4.6a4.8 4.8 0 1 0 6.8 0"/></svg>',
  display: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="2.6"/><path d="M8 1.8v1.6M8 12.6v1.6M1.8 8h1.6M12.6 8h1.6M3.6 3.6l1.1 1.1M11.3 11.3l1.1 1.1M3.6 12.4l1.1-1.1M11.3 4.7l1.1-1.1"/></svg>',
  music: '<svg viewBox="0 0 16 16"><path d="M6 12V3.5l7-1.5v8.5"/><circle cx="4.3" cy="12" r="1.8"/><circle cx="11.3" cy="10.5" r="1.8"/></svg>',
  games: '<svg viewBox="0 0 16 16"><rect x="1.8" y="4.5" width="12.4" height="7" rx="3.5"/><path d="M5 6.6v2.8M3.6 8h2.8"/><circle cx="10.4" cy="7.2" r=".6"/><circle cx="11.8" cy="8.8" r=".6"/></svg>',
  snkdots: '<svg viewBox="0 0 16 16"><path d="M3 13h4v-4h6V3"/></svg>',
  snkspace: '<svg viewBox="0 0 16 16"><path d="M3 13h4v-4h6V3"/></svg>',
  snkiso: '<svg viewBox="0 0 16 16"><path d="M3 13h4v-4h6V3"/></svg>',
  recorder: '<svg viewBox="0 0 16 16"><rect x="5.8" y="1.8" width="4.4" height="7.6" rx="2.2"/><path d="M3.6 7.6a4.4 4.4 0 0 0 8.8 0M8 12v2.2"/></svg>',
  battery: '<svg viewBox="0 0 16 16"><rect x="1.5" y="5" width="11.5" height="6" rx="1.6"/><path d="M14.5 7v2M3.6 7.2v1.6M5.6 7.2v1.6"/></svg>',
  buttons: '<svg viewBox="0 0 16 16"><circle cx="7" cy="8" r="5.3"/><path d="M14 4.6v1.8M14 9.6v1.8"/></svg>',
  tap: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="2.3"/><path d="M3.6 5.2a5.2 5.2 0 0 1 8.8 0M3.6 10.8a5.2 5.2 0 0 0 8.8 0" opacity=".55"/></svg>',
  raise: '<svg viewBox="0 0 16 16"><path d="M3.5 12.5c0-4.6 3-7.6 7.4-7.6M8.8 2.8l2.4 2.1-2.4 2.1"/></svg>',
  wifi: '<svg viewBox="0 0 16 16"><path d="M1.6 6.2a9.2 9.2 0 0 1 12.8 0M3.7 8.4a6.2 6.2 0 0 1 8.6 0M5.8 10.6a3.2 3.2 0 0 1 4.4 0"/><circle cx="8" cy="12.9" r=".6"/></svg>',
  bt: '<svg viewBox="0 0 16 16"><path d="M4.8 5.2l6 5.6L8 13.3V2.7l2.8 2.5-6 5.6"/></svg>',
  lock: '<svg viewBox="0 0 16 16"><rect x="3.5" y="7" width="9" height="6.5" rx="1.5"/><path d="M5.5 7V5.3a2.5 2.5 0 0 1 5 0V7"/></svg>',
  refresh: '<svg viewBox="0 0 16 16"><path d="M13 8a5 5 0 1 1-1.6-3.7M13 2.8v2.7h-2.7"/></svg>',
  check: '<svg viewBox="0 0 16 16"><path d="M3.5 8.5l3 3 6-7"/></svg>',
  pen: '<svg viewBox="0 0 16 16"><path d="M3 13h2.3l6.9-6.9-2.3-2.3L3 10.7zM8.6 4.8l2.5 2.5"/></svg>',
  saved: '<svg viewBox="0 0 16 16"><path d="M4.5 2.5h7v11L8 10.8l-3.5 2.7z"/></svg>',
  info: '<svg viewBox="0 0 16 16"><circle cx="8" cy="8" r="5.8"/><path d="M8 7.4v3.4"/><circle cx="8" cy="5.1" r=".55" fill="currentColor" stroke="none"/></svg>',
};
// Wi-Fi's strength: n of its 3 arcs lit (the board's dBm: -60 and up — 3, -75 and up — 2)
const signal = (rssi) => {
  const n = rssi == null ? 3 : rssi >= -60 ? 3 : rssi >= -75 ? 2 : 1;
  return `<svg class="sig" viewBox="0 0 16 16"><path${n < 3 ? ' class="off"' : ''} d="M1.6 6.2a9.2 9.2 0 0 1 12.8 0"/><path${n < 2 ? ' class="off"' : ''} d="M3.7 8.4a6.2 6.2 0 0 1 8.6 0"/><path d="M5.8 10.6a3.2 3.2 0 0 1 4.4 0"/><circle cx="8" cy="12.9" r=".6"/></svg>`;
};

// ---------- Elements: the dock at the bottom — three folders in a row (Watch faces, Face, App); the
// open one shows its menu above the dock,
// over the field (it moves nothing). A card of an element: drag it onto a screen, or click — onto the active screen,
// at its place on HaloWatch. A card of a screen: a new screen to the right on the map. The menu closes (folding back
// into its folder) after a card is used, by its folder again, Esc or a click anywhere else
const FOLDERS = [
  { id: 'faces', name: 'Watch faces', icon: 'watch', title: 'Whole watch faces: a click makes it the watch face in place of the one before' },
  { id: 'face', name: 'Face', icon: 'clock', title: 'HaloWatch\'s watch face: the whole screen or its parts — date, clock, weather pill' },
  { id: 'app', name: 'App', icon: 'apps', title: 'Apps: Gallery, Display, Battery, Music, Recorder, Weather, Steps, games — each its own screen' },
];
const FACE_SUB = 'HaloWatch\'s watch face: the date, the clock and the weather pill — becomes the watch face in place of the one before';
const GALLERY_SUB = 'A new gallery: the title, the top clock, one card and More; a swipe up on the face opens it';
const SCREEN_CARDS = {
  fwhole: { name: 'HaloWatch', icon: 'whole', kind: 'face', sub: FACE_SUB },
  fours: { name: 'Four sides', icon: 'whole', kind: 'foursides', sub: 'The clock, and the weekday, the date, the weather and the steps along the four sides — becomes the watch face in place of the one before' },
  fempty: { name: 'Empty', icon: 'empty', kind: 'blankface', sub: 'An empty watch face: nothing on it — put on it what you like (Face) — in place of the one before' },
  fscreen: { name: 'Watch face', icon: 'clock', kind: 'face', sub: FACE_SUB },
  gscreen: { name: 'Gallery', icon: 'gallery', kind: 'gallery', sub: GALLERY_SUB },
  empty: { name: 'Empty', icon: 'empty', kind: 'empty', sub: 'A blank screen' },
};
// A folder's tiles by category: each under its header (a chevron, its icon, its name, how many) — a click on the header
// folds its tiles away or shows them again; what is folded is kept (localStorage 'popka.shut': "folder:category") — the
// same after a restart
const SHELVES = {
  faces: [
    { name: 'Built-in', icon: 'watch', items: ['fwhole', 'fours', 'fempty'] },
    { name: 'Saved', icon: 'saved', items: [] },  // (the project's own faces and Add: savedFaces)
  ],
  face: [
    { name: 'Clock', icon: 'clock', items: ['clock', 'pDotClock'] },
    { name: 'Date', icon: 'date', items: ['date', 'pDateArc', 'pWeekday', 'pDayMonth', 'pWeek', 'pDateFrame'] },
    { name: 'Weather', icon: 'weather', items: ['weather', 'pWxArc', 'pForecast'] },
    { name: 'Steps', icon: 'steps', items: ['steps', 'pStepsRing', 'pStepsArc'] },
    { name: 'Battery', icon: 'battery', items: ['pBatRing', 'pBatArc'] },
    { name: 'Combined', icon: 'infoline', items: ['pDateSteps', 'pInfoWords', 'pInfoShort', 'pInfoIcons'] },  // (several at once)
    { name: 'Backgrounds', icon: 'stars', items: ['stars', 'parrot'] },
  ],
  app: [
    { name: 'Gallery', icon: 'gallery', items: ['gscreen', 'display', 'battery'] },
    { name: 'Media', icon: 'music', items: ['music', 'recorder'] },
    { name: 'Info', icon: 'info', items: ['wxscreen', 'stepscreen'] },
    { name: 'Games', icon: 'games', items: ['snkdots', 'snkspace', 'snkiso'] },  // (in the Games gallery: toGames)
  ],
};
// A face part or piece not put in a category yet (a new one) — in "More" at the end of Face, never lost
{
  const placed = new Set(SHELVES.face.flatMap((c) => c.items));
  const rest = [...Object.keys(KINDS).filter((k) => KINDS[k].group === 'face'), ...Object.keys(FACE_PRESETS)].filter((k) => !placed.has(k));
  if (rest.length) SHELVES.face.push({ name: 'More', icon: 'plus', items: rest });
}
const shut = new Set((() => { try { return JSON.parse(localStorage.getItem('popka.shut')) || []; } catch { return []; } })());
let shelf = '', paletteStage = null;  // the open folder

// A category: its header and its tiles (folded — the tiles slide shut, studio.css .cat-body)
function category(folder, cat, tiles, count) {
  const key = `${folder}:${cat.name}`;
  const box = h('div', { class: 'cat' + (shut.has(key) ? ' shut' : '') });
  const hd = h('button', { class: 'cat-hd', 'aria-expanded': String(!shut.has(key)) },
    h('span', { class: 'cat-chev', html: ICON.chev }), h('span', { class: 'cat-ic', html: ICON[cat.icon] }), cat.name,
    h('span', { class: 'cat-n' }, String(count)));
  hd.addEventListener('click', () => {
    shut.has(key) ? shut.delete(key) : shut.add(key);
    localStorage.setItem('popka.shut', JSON.stringify([...shut]));
    box.classList.toggle('shut', shut.has(key));
    hd.setAttribute('aria-expanded', String(!shut.has(key)));
  });
  box.append(hd, h('div', { class: 'cat-body' }, h('div', { class: 'cat-clip' }, h('div', { class: 'cat-tiles' }, ...tiles))));
  return box;
}
const tile = (stage, key, i) => SCREEN_CARDS[key] ? screenCard(stage, key, i) : SECTIONS[key] ? sectionCard(stage, key)
  : FACE_PRESETS[key] ? presetCard(stage, key) : elementCard(stage, key);

// A menu closes by folding back into its folder (shelf-close), then it is gone; another folder's — at once
function setShelf(id) {
  const open = document.querySelector('#palette .shelf');
  shelf = id;
  if (id || !open) return renderPalette(paletteStage);
  open.classList.add('closing');
  for (const b of document.querySelectorAll('#palette > .tool')) b.classList.remove('open');
  setTimeout(() => { if (!shelf) renderPalette(paletteStage); }, 120);
}

export function renderPalette(stage) {
  const box = $('palette');
  if (!paletteStage) {
    document.addEventListener('pointerdown', (e) => { if (shelf && !e.target.closest('#palette')) setShelf(''); });
    // Esc closes the shelf only (before main.js's Esc, which drops the selection)
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape' && shelf) e.stopPropagation(), setShelf(''); }, true);
  }
  paletteStage = stage;
  const folders = FOLDERS.map((f) => {
    const b = h('button', { class: 'tool' + (shelf === f.id ? ' open' : ''), title: f.title, 'aria-expanded': String(shelf === f.id) },
      h('span', { html: ICON[f.icon] }), f.name);
    b.addEventListener('click', () => setShelf(shelf === f.id ? '' : f.id));
    return b;
  });
  const kids = [...folders];
  if (shelf && SHELVES[shelf]) {
    const notch = h('i', { class: 'shelf-notch' });
    const body = h('div', { class: 'shelf-grid' }, ...SHELVES[shelf].map((cat) => cat.name === 'Saved' && shelf === 'faces'
        ? category(shelf, cat, [...savedFaces().map((f) => savedFaceCard(stage, f)), addFaceCard(stage)], savedFaces().length)
        : category(shelf, cat, cat.items.map((key, i) => tile(stage, key, i)), cat.items.length)));
    const el = h('div', { class: 'shelf' }, notch, body);
    kids.push(el);
    const at = folders[FOLDERS.findIndex((q) => q.id === shelf)];  // (taken now: by the next frame it may be closed)
    requestAnimationFrame(() => {  // the notch at the open folder, by the laid-out page
      const f = at.getBoundingClientRect(), r = el.getBoundingClientRect();
      if (r.width) notch.style.setProperty('--notch-x', `${f.left + f.width / 2 - r.left}px`);
    });
  }
  box.replaceChildren(...kids);
}

// The left card's Functions: folded or not by its title, kept as the folders' categories are (popka.shut)
export function initFold() {
  const panel = $('fn-panel'), btn = $('fn-fold'), key = 'left:Functions';
  const show = () => { panel.classList.toggle('shut', shut.has(key)), btn.setAttribute('aria-expanded', String(!shut.has(key))); };
  btn.addEventListener('click', () => {
    shut.has(key) ? shut.delete(key) : shut.add(key);
    localStorage.setItem('popka.shut', JSON.stringify([...shut]));
    show();
  });
  show();
}

function elementCard(stage, kind) {
  const d = KINDS[kind];
  const b = h('button', { class: 'tool', draggable: 'true', title: `${d.name} — ${d.sub}. Drag onto a screen, or click: onto the active one, at its place on HaloWatch` },
    thumb('kind:' + kind, ICON[kind], apart(kind === 'dots' ? [['stnum', { hidden: true }], ['dots', null]] : [[kind, null]]),
      kind === 'stars'), d.name);  // (the dots show on a screen with pages: a hidden part of the Steps screen gives two)
  b.addEventListener('click', () => { setShelf(''); IN_GALLERY.has(kind) ? toGallery(stage, kind) : stage.add(kind, activeScreen()); });
  b.addEventListener('dragstart', (e) => {
    e.dataTransfer.setData('text/x-popka-kind', kind);
    e.dataTransfer.effectAllowed = 'copy';
    // the menu over the field steps aside: see-through, the drop lands on the screen under it too
    setTimeout(() => b.closest('.shelf')?.classList.add('dragging'));
  });
  b.addEventListener('dragend', () => setShelf(''));
  return b;
}

// What goes into a list goes into a gallery — the screen being edited if it is one, else the gallery (the swipe up's);
// no gallery yet — one is made, the whole of it (a title, the top clock, a card, More). Never onto the face or another
// screen over everything. A section (Display, Battery): its card in the gallery and its screen behind it; a new gallery
// gets it as its first card (and no plain card)
export const IN_GALLERY = new Set(['title', 'card', 'edge', 'scroll']);
// Battery and Display go deeper (SECTION_PATH: Gallery › General, Gallery › General › Display): from the gallery being
// edited if it is on that path, else from the main one; the galleries on the way are made if missing — one undo step
export function toGallery(stage, kind) {
  let g = galleryFor(activeScreen());
  const fresh = !g;
  if (fresh) g = addScreen(stage, null, 'gallery', { card: !SECTIONS[kind] });
  if (SECTIONS[kind]) {
    const key = `app:${kind}:${performance.now()}`;
    let path = SECTION_PATH[kind] || [];
    const at = path.indexOf(titleOf(g));
    if (at >= 0) path = path.slice(at + 1);
    else if (path.length && isGallery(mainScreen())) g = mainScreen();
    for (const name of path) g = g && subGallery(g, name, key);
    const card = g && addSection(g, kind, key);
    if (card) select({ type: 'layer', id: card.id });
  } else if (!fresh) {
    stage.add(kind, g);
  }
  if (g && state.screen !== g.id) state.screen = g.id, emit('screen');
}

// A game: its card in the Games gallery (made the first time: the card "Games" in the main gallery and the gallery
// behind it) and its screen behind the card
export function toGames(stage, kind) {
  const main = mainScreen();
  let g = isGallery(main) ? main : galleryFor(activeScreen());
  if (!g) g = addScreen(stage, null, 'gallery', { card: false });
  const key = `app:${kind}:${performance.now()}`;  // (one undo step)
  const games = gamesGallery(g, key);
  if (!games) return;
  const card = addSection(games, kind, key);
  if (card) select({ type: 'layer', id: card.id });
  if (state.screen !== games.id) state.screen = games.id, emit('screen');
}

// A ready piece of a face (state.js FACE_PRESETS): its layers onto the active screen, in their looks
function presetCard(stage, key) {
  const p = FACE_PRESETS[key];
  const b = h('button', { class: 'tool', title: `${p.name} — ${KINDS[p.parts[0][0]].sub}. Click: onto the active screen` },
    thumb('preset:' + key, ICON[p.icon], apart(p.parts)), p.name);
  b.addEventListener('click', () => {
    setShelf('');
    const s = activeScreen(), ls = p.parts.map(([kind, look]) => newLayer(kind, look, s));
    change(() => s.layers.push(...ls), { structure: true });
    select({ type: 'layer', id: ls[ls.length - 1].id });
  });
  return b;
}

function sectionCard(stage, kind) {
  const sub = SECTIONS[kind].sub || KINDS[kind].sub, what = sub.startsWith('HaloWatch') ? `HaloWatch's ${sub.replace(/^HaloWatch's \w+: /, '')}` : sub;
  const where = SECTION_PATH[kind] ? `Gallery › ${SECTION_PATH[kind].join(' › ')}` : KINDS[kind]?.game ? 'the Games gallery' : 'the gallery';
  const b = h('button', { class: 'tool', title: `${SECTIONS[kind].tile || SECTIONS[kind].text} — ${what}: its card in ${where} and its screen` },
    thumb('section:' + kind, ICON[SECTIONS[kind].tile ? 'whole' : kind],
      apart(sectionParts(kind).map((t) => [t, null])), true),
    SECTIONS[kind].tile || SECTIONS[kind].text);
  b.addEventListener('click', () => { setShelf(''); KINDS[kind]?.game ? toGames(stage, kind) : toGallery(stage, kind); });
  return b;
}

function screenCard(stage, key, i) {
  const c = SCREEN_CARDS[key];
  const face = c.kind === 'face' || c.kind === 'foursides' || c.kind === 'blankface';  // (Watch faces: the home's)
  const b = h('button', { class: 'tool' + (i === 0 && key.endsWith('whole') ? ' main' : ''), title: `${c.sub}. Click: ${face ? 'it becomes the watch face' : 'a new screen on the map'}` },
    c.kind === 'empty' ? h('span', { html: ICON[c.icon] }) : thumb('screen:' + c.kind, ICON[c.icon], () => newScreen(c.kind).layers, true), c.name);
  b.addEventListener('click', () => {
    setShelf('');
    if (face) toHomeFace(stage, c.kind);
    else stage.focus(addScreen(stage, null, c.kind));
  });
  return b;
}

// A whole face from Watch faces becomes THE watch face (the home: the first screen) in place of the one before: the
// home keeps its id and its place on the map, its layers are the new face's — the old face is gone (Undo brings it
// back; Watch faces → Add keeps one). What the old face's layers opened, the new face's layers of the same part open
// (the pill — Weather); a pill that opens nothing — the first Weather screen, as on HaloWatch. The same face again
// puts it back as it was (a part deleted or changed — there again); untouched — nothing happens
export function toHomeFace(stage, kind) {
  const own = kind.startsWith('saved:') ? savedFaces().find((f) => 'saved:' + f.id === kind) : null;
  if (kind.startsWith('saved:') && !own) return;
  const hm = home(), was = hm.name, old = hm.layers;
  const title = own ? own.name : { face: 'HaloWatch', foursides: 'Four sides', blankface: 'Watch face' }[kind];
  const part = (l) => l.type + ':' + (l.what || '');
  hm.layers = [];  // (made on the home itself, one by one: each new layer's id counts the ones before it)
  if (own) hm.layers = faceLayers(own, hm);
  else if (kind !== 'blankface') {  // (Empty: no layers — what the face gets is added from Face)
    for (const [type, look] of kind === 'foursides' ? FOUR_SIDES : FACE_SCREEN.map((t) => [t, null])) hm.layers.push(newLayer(type, look, hm));
  }
  const ls = hm.layers;
  hm.layers = old;
  const taps = old.filter((l) => l.tap && screenById(l.tap.to));
  for (const l of ls) {
    const i = l.tap ? -1 : taps.findIndex((q) => part(q) === part(l));
    if (i >= 0) l.tap = { ...taps[i].tap }, taps.splice(i, 1);
  }
  const weather = screens().find((q) => q !== hm && q.layers.some((l) => PAGED.has(l.type)));
  const pill = ls.find((l) => l.type === 'weather' && !l.tap);
  if (weather && pill) pill.tap = { to: weather.id, fx: 'fade' };
  const plain = (a) => JSON.stringify(a.map(({ id, ...l }) => l));
  if (hm.face === kind && plain(ls) === plain(old)) {
    saved.note = `${hm.name} is the watch face already`;
    return stage.focus(hm);
  }
  const again = hm.face === kind;
  change(() => {
    hm.layers = ls;
    hm.face = kind;
    hm.name = title;
    for (let k = 2; screens().some((q) => q !== hm && q.name === hm.name); k++) hm.name = `${title} ${k}`;
  }, { structure: true });
  saved.note = kind === 'blankface' ? `${hm.name}: empty — put on it what you like (Face)` : again ? `${hm.name}: back as it was`
    : was === hm.name ? `${hm.name}: the watch face` : `${hm.name}: the watch face now, in place of ${was}`;
  select({ type: 'screen', id: hm.id });
  state.screen = hm.id;
  emit('screen');
  stage.focus(hm);
  return hm;
}
// A face saved from the project: its picture, its name; the bin — out of Watch faces (Undo brings it back)
function savedFaceCard(stage, f) {
  const b = h('button', { class: 'tool', title: `${f.name} — saved from this project. Click: it becomes the watch face` },
    thumb('saved:' + f.id, ICON.whole, () => f.layers, true), f.name);
  const del = h('span', { class: 'tool-del', title: 'Delete this watch face', role: 'button', html: ICON.trash });
  del.addEventListener('click', (e) => {
    e.stopPropagation();
    removeFace(f.id);
    saved.note = `${f.name}: deleted from Watch faces`;
    renderPalette(stage);
  });
  b.append(del);
  b.addEventListener('click', () => { setShelf(''); toHomeFace(stage, 'saved:' + f.id); });
  return b;
}
// Add: the watch face now saved as a new one — first its name (one is offered); Enter saves, Esc or a click away — not
function addFaceCard(stage) {
  const b = h('button', { class: 'tool', title: 'Save the current watch face as a new one' }, h('span', { class: 'tool-pic', html: ICON.plus }), 'Add');
  b.addEventListener('click', () => {
    const base = home().name, taken = (n) => savedFaces().some((f) => f.name === n) || n === 'HaloWatch' || n === 'Four sides';
    let name = base;
    for (let k = 2; taken(name); k++) name = `${base} ${k}`;
    const input = h('input', { class: 'tool-name', type: 'text', value: name, spellcheck: 'false', 'aria-label': 'Name' });
    const edit = h('div', { class: 'tool editing' }, h('span', { class: 'tool-pic', html: ICON.plus }), input);
    b.replaceWith(edit);
    input.focus(), input.select();
    const done = (ok) => {
      const v = input.value.trim();
      if (ok && v) saveFace(v), (saved.note = `${v}: saved in Watch faces`);
      renderPalette(stage);
    };
    input.addEventListener('keydown', (e) => {
      e.stopPropagation();
      if (e.key === 'Enter') done(true);
      if (e.key === 'Escape') done(false);
    });
    input.addEventListener('blur', () => setTimeout(() => { if (document.contains(input)) done(false); }));
  });
  return b;
}

// Any screen as the watch face: it goes first (the home) and takes the home's place on the map; the old home stays
export function makeHome(stage, s) {
  const was = home();
  if (s === was) return stage.focus(s);
  change((p) => {
    p.screens.splice(p.screens.indexOf(s), 1), p.screens.unshift(s);
    [s.x, was.x] = [was.x, s.x], [s.y, was.y] = [was.y, s.y];
  }, { structure: true });
  saved.note = `${s.name}: the watch face now · ${was.name} stays on the map`;
  select({ type: 'screen', id: s.id });
  state.screen = s.id;
  emit('screen');
  stage.focus(s);
  return s;
}

// ---------- Functions (the left card, between Screens and Layers) — a row each: its icon, its name, a switch; a click on the row — its
// settings in Properties (the shelf stays: the other switches are at hand); Wi-Fi's open right here, under its row;
// Bluetooth has none (a click — its switch) ----------
export function renderFunctions() {
  const list = $('functions');
  if (!list) return;
  const focus = document.activeElement, at = focus?.selectionStart;  // (typing a password: it stays in its field)
  list.replaceChildren(...FN_ROWS.flatMap((id) => {
    const f = fnRow(id), sel = (state.sel?.type === 'fn' && fnRowOf(state.sel.id) === id) || (id === 'wifi' && wifi.open && f.on);
    const row = h('li', { class: 'layer fn' + (sel ? ' sel' : '') + (f.on ? '' : ' off') },
      h('span', { class: 'layer-icon', html: ICON[id] }), h('span', { class: 'layer-name' }, f.name), h('span'),
      toggle(f.on, (on) => setRow(id, on), f.name));
    row.addEventListener('click', () => {
      if (id === 'bt') return setFn(id, { on: !f.on }, { structure: true });
      if (id !== 'wifi') return select({ type: 'fn', id });
      if (!f.on) return;
      wifi.open = !wifi.open;
      if (state.sel?.type === 'fn') select(null);
      if (wifi.open && !wifi.loaded) wifi.loaded = true, wifiCall().then(drawWifi, () => {}), wifiScan();
      renderFunctions();
    });
    return id === 'wifi' && wifi.open && f.on ? [row, wifiBox()] : [row];
  }));
  if (focus && focus !== document.activeElement && list.contains(focus)) {
    focus.focus();
    if (at != null) focus.setSelectionRange(at, at);
  }
}

// The rows: the buttons are one row — Buttons (both on its switch, each its own action in Properties)
const FN_ROWS = ['aod', 'buttons', 'wifi', 'bt'];
const fnRowOf = (id) => (id === 'boot' || id === 'pwr' ? 'buttons' : id);
const fnRow = (id) => (id === 'buttons' ? { name: 'Buttons', on: fnOf('boot').on || fnOf('pwr').on } : fnOf(id));
function setRow(id, on) {
  if (id !== 'buttons') return setFn(id, { on }, { structure: true });
  change((p) => {
    p.functions ??= {};
    for (const b of ['boot', 'pwr']) p.functions[b] = { ...p.functions[b], on };
  }, { structure: true });
}

// ---------- Wi-Fi, opened under its row: the board's network on top (Connected — its last sync went well), then the
// networks the board sees (studio.py asks it over USB: wifi scan; ↻ — again), a password under the picked one; Join —
// studio.py keeps it on this Mac (secrets/wifi.json: never in the project, never in git), the board joins it after
// Flash. Other… — a network by its name (a hidden one, or the board not on USB) ----------
const OTHER = '\u0000other';
const wifi = { open: false, loaded: false, busy: false, nets: [], status: null, current: '', saved: [], pick: null, err: '',
               name: '', pass: '', show: false, focus: false };
let wifiEl = null;

async function wifiCall(body) {
  const r = await fetch('/api/wifi', body ? { method: 'POST', body: JSON.stringify(body) } : {});
  const d = await r.json().catch(() => ({}));
  if (d.current !== undefined) wifi.current = d.current, wifi.saved = d.saved || [];
  if (!r.ok) throw new Error(d.error || `${r.status}`);
  return d;
}
async function wifiScan() {
  if (wifi.busy) return;
  wifi.busy = true, wifi.err = '', drawWifi();
  try {
    const d = await wifiCall({ scan: true });
    wifi.nets = d.networks || [], wifi.status = d.status || null;
  } catch (e) {
    wifi.err = e.message;
  }
  wifi.busy = false, drawWifi();
}
async function wifiJoin(ssid, password) {
  if (!ssid) return;
  try {
    await wifiCall(password == null ? { ssid } : { ssid, password });
    wifi.pick = null, wifi.pass = '', wifi.name = '', wifi.err = '';
  } catch (e) {
    wifi.err = e.message;
  }
  drawWifi();
}

function wifiBox() {
  if (!wifiEl) wifiEl = h('li', { class: 'acc' });
  drawWifi();
  return wifiEl;
}
function drawWifi() {
  if (!wifiEl) return;
  const focus = document.activeElement, key = wifiEl.contains(focus) ? focus.dataset.k : null, at = focus?.selectionStart;
  const st = wifi.status, here = wifi.nets.find((n) => n.ssid === wifi.current);
  const tag = !st || !wifi.current ? '' : st.ssid !== wifi.current ? 'After Load'
    : { synced: 'Connected', nonet: 'Can\'t connect', noinet: 'No internet' }[st.state] || '';
  const rows = [];
  if (wifi.current) {
    rows.push(h('li', { class: 'layer cur' }, h('span', { class: 'layer-icon', html: signal(here?.rssi) }),
      h('span', { class: 'layer-name' }, wifi.current), h('span', { class: 'tag' }, tag),
      h('span', { class: 'layer-icon ok-ic', html: ICON.check })));
  }
  rows.push(h('li', { class: 'acc-h' }, 'Networks', h('button', {
    class: 'icon-btn' + (wifi.busy ? ' spin' : ''), title: 'Look again', 'aria-label': 'Look again', html: ICON.refresh,
    onclick: (e) => { e.stopPropagation(); wifiScan(); },
  })));
  if (wifi.err) rows.push(h('li', { class: 'empty' }, wifi.err));
  // The networks scroll in a list of their own (a crowded street has dozens: the menu would grow past the window's
  // top); the board's network above it and Other… below stay put
  const nets = [], was = wifiEl.querySelector('.nets')?.scrollTop || 0;
  for (const n of wifi.nets.filter((q) => q.ssid !== wifi.current)) {
    const row = h('li', { class: 'layer net' + (wifi.pick === n.ssid ? ' sel' : '') }, h('span', { class: 'layer-icon', html: signal(n.rssi) }),
      h('span', { class: 'layer-name' }, n.ssid), h('span', { class: 'lk', html: n.lock ? ICON.lock : '' }), h('span'));
    row.addEventListener('click', () => { wifi.pick = wifi.pick === n.ssid ? null : n.ssid, wifi.pass = '', wifi.focus = true, drawWifi(); });
    nets.push(row);
    if (wifi.pick === n.ssid) nets.push(joinRow(n.ssid, n.lock && !wifi.saved.includes(n.ssid)));
  }
  const list = nets.length ? h('ul', { class: 'layers nets' }, ...nets) : null;
  if (list) rows.push(h('li', {}, list));
  const other = h('li', { class: 'layer net' + (wifi.pick === OTHER ? ' sel' : '') }, h('span', { class: 'layer-icon', html: ICON.plus }),
    h('span', { class: 'layer-name' }, 'Other…'), h('span'), h('span'));
  other.addEventListener('click', () => { wifi.pick = wifi.pick === OTHER ? null : OTHER, wifi.pass = '', wifi.focus = true, drawWifi(); });
  rows.push(other);
  if (wifi.pick === OTHER) rows.push(joinRow(null, true));
  wifiEl.replaceChildren(h('ul', { class: 'layers' }, ...rows));
  if (list) {
    list.scrollTop = was;  // (redrawn: the list stays where it was scrolled)
    list.querySelector('.pw')?.scrollIntoView({ block: 'nearest' });  // the picked one's password — in sight
  }
  const back = key && wifiEl.querySelector(`[data-k="${key}"]`);
  if (back) back.focus(), at != null && back.setSelectionRange(at, at);
}
// Under the picked network: its password (a saved or open one — none) and Join; Other… — its name too
function joinRow(ssid, needsPass) {
  const field = (k, kind, label) => {
    const input = h('input', { type: kind, value: wifi[k], placeholder: label, 'aria-label': label, spellcheck: 'false', autocomplete: 'off', 'data-k': k });
    input.addEventListener('input', () => { wifi[k] = input.value; });
    input.addEventListener('keydown', (e) => { e.stopPropagation(); if (e.key === 'Enter') join(); });
    return input;
  };
  const join = () => wifiJoin(ssid ?? wifi.name.trim(), needsPass ? wifi.pass : null);
  const eye = h('button', { class: 'icon-btn', title: wifi.show ? 'Hide' : 'Show', 'aria-label': 'Show the password', html: wifi.show ? ICON.eyeOff : ICON.eye });
  eye.addEventListener('click', (e) => { e.stopPropagation(); wifi.show = !wifi.show, drawWifi(); });
  const kids = [];
  if (ssid == null) kids.push(h('label', { class: 'field wide' }, field('name', 'text', 'Name')));
  if (needsPass) kids.push(h('label', { class: 'field' }, field('pass', wifi.show ? 'text' : 'password', 'Password'), eye));
  else kids.push(h('span'));
  const go = h('button', { class: 'primary' }, 'Join');
  go.addEventListener('click', (e) => { e.stopPropagation(); join(); });
  kids.push(go);
  const row = h('li', { class: 'pw' }, ...kids);
  row.addEventListener('click', (e) => e.stopPropagation());
  if (wifi.focus) wifi.focus = false, setTimeout(() => row.querySelector('input')?.focus());  // (just picked)
  return row;
}

// A switch (on — accent); set(on) on a click (it doesn't select the row under it)
function toggle(on, set, label) {
  return h('button', {
    class: 'switch' + (on ? ' on' : ''), role: 'switch', 'aria-checked': String(on), 'aria-label': label,
    onclick: (e) => { e.stopPropagation(); set(!on); },
  });
}

// ---------- Screens ----------
// A new screen (empty, HaloWatch's Weather screen or a new gallery). linkFrom — the layer that opens it (of the face,
// or a card or the button of a gallery); as on HaloWatch, the Weather screen is opened by the face's first weather
// pill that opens nothing yet; the gallery — by a swipe up on the face (if nothing else is there yet)
export function addScreen(stage, linkFrom, kind = 'empty', opts = {}) {
  if ((kind === 'face' || kind === 'foursides') && !home().layers.length) return fillHome(kind);
  const s = kind === 'list' ? newList(layerById(linkFrom)) : newScreen(kind, opts);
  if (!linkFrom && kind === 'weather') linkFrom = home().layers.find((l) => l.type === 'weather' && !l.tap)?.id;
  const main = kind === 'gallery' && !mainScreen();
  change((p) => {
    p.screens.push(s);
    const from = linkFrom && layerById(linkFrom);
    if (from) from.tap = { to: s.id, fx: TAPS.has(from.type) ? 'a16' : 'fade' };
    if (main) p.main = s.id;
  }, { structure: true });
  if (kind === 'weather') saved.note = `${s.name}: HaloWatch's Weather screen` + (linkFrom ? ` · ${layerById(linkFrom).name} on ${home().name} opens it` : '');
  if (kind === 'face') saved.note = `${s.name}: HaloWatch's watch face on a screen of its own (the home is ${home().name})`;
  if (kind === 'foursides') saved.note = `${s.name}: the clock and four curved lines (the home is ${home().name})`;
  if (kind === 'gallery') saved.note = `${s.name}: the title, the top clock, a card and More` + (main ? ` · a swipe up on ${home().name} opens it` : '');
  if (kind === 'list') saved.note = `${s.name}: the title, the top clock, a card and More · ${layerById(linkFrom).name} opens it`;
  select(linkFrom && kind === 'empty' ? { type: 'link', id: linkFrom } : { type: 'screen', id: s.id });
  state.screen = s.id;
  emit('screen');
  return s;
}

// HaloWatch's watch face onto the empty home: the date, the clock and the pill at their places; as on HaloWatch, the
// pill opens the first Weather screen (Fade)
function fillHome(kind = 'face') {
  const s = home(), weather = screens().find((q) => q !== s && q.layers.some((l) => PAGED.has(l.type)));
  change(() => {
    if (kind === 'foursides') for (const [type, look] of FOUR_SIDES) s.layers.push(newLayer(type, look, s));
    else for (const type of FACE_SCREEN) s.layers.push(newLayer(type, null, s));
    const pill = s.layers.find((l) => l.type === 'weather');
    if (weather && pill) pill.tap = { to: weather.id, fx: 'fade' };
  }, { structure: true });
  saved.note = `${s.name}: HaloWatch's watch face` + (weather ? ` · its pill opens ${weather.name}` : '');
  select({ type: 'screen', id: s.id });
  state.screen = s.id;
  emit('screen');
  return s;
}

export function removeScreen(id) {
  change((p) => {
    p.screens = p.screens.filter((s) => s.id !== id);
    for (const s of p.screens) for (const l of s.layers) if (l.tap && l.tap.to === id) l.tap = null;
    if (p.main === id) p.main = null;
  }, { structure: true });
  if (state.screen === id) state.screen = home().id, emit('screen');
  select(null);
}

// A screen with pages (the Weather screen's cities): a twist shows them under it — a click on one picks it
const opened = new Set();
export function renderScreens(stage) {
  $('screens').replaceChildren(...screens().flatMap((s, i) => {
    const pages = paged(s), open = pages && opened.has(s.id), cur = s.id === state.screen;
    const row = h('li', { class: 'layer' + (cur && !open ? ' sel' : '') },
      h('span', { class: 'layer-icon', html: i === 0 ? ICON.home : s === mainScreen() ? ICON.gallery : ICON.screen }),
      h('span', { class: 'layer-name', ondblclick: (e) => rename(e, s.name, (v) => { s.name = v; }) }, s.name),
      pages ? h('button', {
        class: 'icon-btn twist' + (open ? ' open' : ''), title: open ? 'Hide its pages' : 'Its pages', html: ICON.chev,
        onclick: (e) => { e.stopPropagation(); open ? opened.delete(s.id) : opened.add(s.id); renderScreens(stage); },
      }) : h('span', { class: 'tag' }, i === 0 || linkedScreens().includes(s) ? '' : 'not linked'),
      i === 0 ? h('span') : h('button', {
        class: 'icon-btn', title: 'Delete the screen', html: ICON.trash,
        onclick: (e) => { e.stopPropagation(); removeScreen(s.id); },
      }));
    // (Map: only picked, as a click on it there — the map stays still; Edit, Try: shown in the screen's place)
    row.addEventListener('click', () => { select({ type: 'screen', id: s.id }); if (state.mode !== 'map') stage.focus(s); });
    if (!open) return [row];
    const subs = Array.from({ length: pagesOf(s) }, (_, k) => {
      const sub = h('li', { class: 'layer sub' + (cur && state.page === k ? ' sel' : '') },
        h('span', { class: 'page-dot' }), h('span', { class: 'layer-name' }, pageName(s, k)));
      sub.addEventListener('click', () => stage.showPage(s, k));
      return sub;
    });
    if (weatherPaged(s)) sortable(subs, (from, to) => { moveCity(from, to); stage.keepPage(); });  // (the cities' order)
    return [row, ...subs];
  }));
}

// ---------- Layers of the active screen: the top row is drawn last (in front) ----------

export function renderLayers() {
  const list = $('layers'), s = activeScreen();
  $('layers-title').textContent = s.name;
  if (!s.layers.length) {
    list.replaceChildren(h('li', { class: 'empty' }, 'The screen is empty — add elements from above or drag them onto it.'));
    return;
  }
  const shown = s.layers.slice().reverse();  // (the top row — drawn last, in front)
  const rows = shown.map((l) => {
    const sel = state.sel && state.sel.type === 'layer' && state.sel.id === l.id;
    const row = h('li', { class: 'layer' + (sel ? ' sel' : '') + (l.hidden ? ' off' : '') },
      h('span', { class: 'layer-icon', html: ICON[l.type] }),
      h('span', { class: 'layer-name', ondblclick: (e) => rename(e, l.name, (v) => { l.name = v; }) },
        l.name, l.tap && screenById(l.tap.to) ? h('span', { class: 'link-dot', title: 'A tap opens ' + screenById(l.tap.to).name, html: ICON.link }) : null),
      h('button', {
        class: 'icon-btn eye', title: l.hidden ? 'Show' : 'Hide', html: l.hidden ? ICON.eyeOff : ICON.eye,
        onclick: (e) => { e.stopPropagation(); toggleHidden(l.id); },
      }),
      h('button', {
        class: 'icon-btn', title: 'Delete', html: ICON.trash,
        onclick: (e) => { e.stopPropagation(); remove(l.id); },
      }));
    row.addEventListener('click', () => select({ type: 'layer', id: l.id }));
    return row;
  });
  list.replaceChildren(...rows);
  // a row to place `to` of the list: above the one there (going up) or below it (going down)
  sortable(rows, (from, to) => move(shown[from].id, shown[to].id, to < from));
}

// ---------- A short list put in order by dragging a row (the layers, the cities): the row lifts and follows the
// pointer, the others slide apart to show where it lands; let go — it settles into the gap; Esc — all back ----------
let sorting = null, clickAfterSort = false;
function sortable(rows, onMove) {
  if (rows.length < 2) return;
  rows.forEach((row, k) => {
    row.classList.add('sortable');
    row.addEventListener('pointerdown', (e) => {
      if (e.button !== 0 || e.target.closest('button, input')) return;
      sorting = { rows, k, to: k, y0: e.clientY, id: e.pointerId, row, live: false, onMove };
    });
  });
}
document.addEventListener('pointermove', (e) => {
  const s = sorting;
  if (!s || e.pointerId !== s.id) return;
  const dy = e.clientY - s.y0;
  if (!s.live) {
    if (Math.abs(dy) < 4) return;  // (a click, not a drag)
    s.live = true;
    s.rects = s.rows.map((r) => r.getBoundingClientRect());
    s.step = s.rects[1].top - s.rects[0].top;  // a row and the gap after it
    s.row.classList.add('lifted');
    for (const r of s.rows) if (r !== s.row) r.classList.add('making-room');
    document.body.classList.add('sorting');
  }
  const r0 = s.rects[s.k], d = Math.max(s.rects[0].top - r0.top, Math.min(s.rects.at(-1).bottom - r0.bottom, dy));
  s.row.style.transform = `translateY(${d}px) scale(1.02)`;
  // where it lands: past the middle of a row — that row's place
  const mid = r0.top + r0.height / 2 + d;
  s.to = s.rects.filter((r, i) => i !== s.k && mid > r.top + r.height / 2).length;
  s.rows.forEach((r, i) => {
    if (i === s.k) return;
    const shift = s.k < s.to && i > s.k && i <= s.to ? -s.step : s.k > s.to && i >= s.to && i < s.k ? s.step : 0;
    r.style.transform = shift ? `translateY(${shift}px)` : '';
  });
});
function endSort(keep) {
  const s = sorting;
  sorting = null;
  if (!s?.live) return;
  clickAfterSort = true;  // (the click that ends a drag picks nothing)
  setTimeout(() => { clickAfterSort = false; });
  const to = keep ? s.to : s.k;
  s.row.classList.add('settling');
  s.row.style.transform = `translateY(${(to - s.k) * s.step}px)`;
  if (!keep) for (const r of s.rows) if (r !== s.row) r.style.transform = '';
  setTimeout(() => {
    document.body.classList.remove('sorting');
    for (const r of s.rows) r.style.transform = '', r.classList.remove('lifted', 'settling', 'making-room');
    if (to !== s.k) s.onMove(s.k, to);  // (the list is drawn anew in its new order)
  }, 150);
}
document.addEventListener('pointerup', (e) => { if (sorting && e.pointerId === sorting.id) endSort(true); });
document.addEventListener('pointercancel', (e) => { if (sorting && e.pointerId === sorting.id) endSort(false); });
document.addEventListener('keydown', (e) => { if (e.key === 'Escape' && sorting?.live) e.stopPropagation(), endSort(false); }, true);
document.addEventListener('click', (e) => { if (clickAfterSort) e.stopPropagation(), e.preventDefault(); }, true);

// Double click on a name: edit it in place
function rename(e, name, set) {
  const span = e.currentTarget;
  const input = h('input', { value: name });
  span.replaceChildren(input);
  input.focus(), input.select();
  let done = false;
  const finish = (ok) => {
    if (done) return;
    done = true;
    const v = input.value.trim();
    if (ok && v && v !== name) change(() => set(v), { structure: true });
    else emit('structure');
  };
  input.addEventListener('keydown', (ev) => {
    if (ev.key === 'Enter') finish(true);
    if (ev.key === 'Escape') finish(false);
    ev.stopPropagation();
  });
  input.addEventListener('blur', () => finish(true));
}

export function toggleHidden(id) {
  change(() => { const l = layerById(id); l.hidden = !l.hidden; }, { structure: true });
}

export function remove(id) {
  const s = screenOf(id), l = layerById(id);
  // A card or the button goes with its own screen if that is still empty and nothing else opens it
  const own = l && TAPS.has(l.type) && l.tap && screenById(l.tap.to);
  const alone = own && !own.layers.length && own !== home() && own !== mainScreen() &&
    !screens().some((q) => q.layers.some((v) => v !== l && v.tap?.to === own.id));
  change((p) => {
    s.layers = s.layers.filter((q) => q.id !== id);
    if (alone) p.screens = p.screens.filter((q) => q !== own);
  }, { structure: true });
  if (state.sel && state.sel.id === id) select(null);
}

export function duplicate(id) {
  const l = layerById(id), s = screenOf(id);
  if (!l) return;
  const copy = JSON.parse(JSON.stringify(l));
  const used = new Set(screens().flatMap((q) => q.layers.map((v) => v.id)));
  let n = 1;
  while (used.has(l.type + n)) n++;
  copy.id = l.type + n, copy.name = l.name + ' copy', copy.dx += 12, copy.dy += 12;
  const to = copy.tap && screenById(copy.tap.to);
  if (TAPS.has(copy.type) && to && !to.layers.length) copy.tap = null;  // (a card's copy gets a screen of its own)
  change(() => s.layers.splice(s.layers.indexOf(l) + 1, 0, copy), { structure: true });
  select({ type: 'layer', id: copy.id });
}

// Rows are shown top = front; in the project the front is last
function move(id, targetId, above) {
  const s = screenOf(id);
  if (screenOf(targetId) !== s) return;
  change(() => {
    const arr = s.layers, from = arr.findIndex((q) => q.id === id);
    const [l] = arr.splice(from, 1);
    const t = arr.findIndex((q) => q.id === targetId);
    arr.splice(above ? t + 1 : t, 0, l);
    keepHead(s, l);  // (a gallery: nothing above its title)
  }, { structure: true });
}

// ---------- Properties ----------
// A number field: label, value, unit; set(v) on typing (one undo step per field while typing)
function num(label, get, set, { step = 1, min, max, unit = '', key }) {
  const input = h('input', { type: 'number', step, min, max });
  input.value = fmt(get());
  input.addEventListener('input', () => {
    const v = parseFloat(input.value);
    if (Number.isFinite(v)) set(clamp(v, min, max), key);
  });
  input.addEventListener('keydown', (e) => e.stopPropagation());
  input.addEventListener('blur', () => { input.value = fmt(get()); });
  refreshers.push(() => { if (document.activeElement !== input) input.value = fmt(get()); });
  return h('label', { class: 'field' }, h('span', {}, label), input, unit ? h('span', { class: 'unit' }, unit) : null);
}

function slider(label, get, set, { min, max, step = 1, unit = '', key }) {
  const range = h('input', { type: 'range', min, max, step });
  range.value = get();
  range.addEventListener('input', () => set(parseFloat(range.value), key));
  refreshers.push(() => { if (document.activeElement !== range) range.value = get(); });
  return h('div', { class: 'slider' }, h('label', {}, label), range, num('', get, set, { step, min, max, unit, key }));
}

function color(label, get, set, key) {
  const pick = h('input', { type: 'color' });
  const text = h('input', { type: 'text', spellcheck: 'false', maxlength: '7' });
  const show = () => { pick.value = get().toLowerCase(); if (document.activeElement !== text) text.value = get().toUpperCase(); };
  show();
  pick.addEventListener('input', () => set(pick.value.toUpperCase(), key));
  text.addEventListener('input', () => {
    let v = text.value.trim();
    if (!v.startsWith('#')) v = '#' + v;
    if (/^#[0-9a-fA-F]{6}$/.test(v)) set(v.toUpperCase(), key);
  });
  text.addEventListener('keydown', (e) => e.stopPropagation());
  text.addEventListener('blur', show);
  refreshers.push(show);
  return h('div', { class: 'color' }, h('label', {}, label), h('label', { class: 'field' }, pick, text));
}

// A row of choices, one lit: options [[value, label], ...]; value — or a getter (the lit one follows it)
function segmented(label, value, options, set) {
  const now = typeof value === 'function' ? value : () => value;
  const buttons = options.map(([v, t]) => {
    const b = h('button', { role: 'radio' }, t);
    b.addEventListener('click', () => set(v));
    return b;
  });
  const mark = () => buttons.forEach((b, i) => {
    const on = options[i][0] === now();
    b.classList.toggle('on', on), b.setAttribute('aria-checked', String(on));
  });
  mark();
  refreshers.push(mark);
  return h('div', { class: 'color' }, h('label', {}, label), h('div', { class: 'seg', role: 'radiogroup', 'aria-label': label }, ...buttons));
}

// A choice: options [[value, label], ...]
function choice(label, value, options, set) {
  const sel = h('select', { class: 'select' }, ...options.map(([v, t]) => h('option', { value: v, selected: v === value }, t)));
  sel.addEventListener('change', () => set(sel.value));
  return h('div', { class: 'color' }, h('label', {}, label), sel);
}

const clamp = (v, lo, hi) => Math.min(hi ?? Infinity, Math.max(lo ?? -Infinity, v));
const fmt = (v) => (Math.abs(v - Math.round(v)) < 0.05 ? String(Math.round(v)) : v.toFixed(1));
const NEW = '__new__', NEW_LIST = '__list__';
// Go to's last choices: a new empty screen; for a card or the button — also a new gallery under it (newList)
const newChoices = (l) => [[NEW, '+ New screen'], ...(TAPS.has(l.type) ? [[NEW_LIST, '+ New gallery']] : [])];

let refreshers = [];
let shownFor;

// The panel's head: an icon, the name (an element or text) and what it is; right — on the right: {title, fn} — a trash
// button that deletes it, or an element (a function's switch)
function head(icon, title, kind, right) {
  return h('div', { class: 'props-head' },
    icon ? h('div', { class: 'layer-icon', html: icon }) : null,
    h('div', { class: 'props-name' }, typeof title === 'string' ? h('div', { class: 'props-title' }, title) : title,
      kind ? h('div', { class: 'props-kind' }, kind) : null),
    right instanceof Element ? right
      : right ? h('button', { class: 'icon-btn del', title: right.title, 'aria-label': right.title, html: ICON.trash, onclick: right.fn }) : null);
}

// Built anew when the selection changes; otherwise only its values are refreshed (typing keeps the focus)
export function renderProps(stage, force = false) {
  const key = JSON.stringify(state.sel);
  if (!force && key === shownFor) return refreshers.forEach((f) => f());
  shownFor = key;
  refreshers = [];
  // Only its head (a screen with nothing to set: its name and bin are in Screens too) — no card, as with nothing selected
  const s = state.sel, show = (parts) => {
    parts = parts.filter(Boolean);
    const bare = parts.every((p) => p.classList.contains('props-head'));
    $('props').replaceChildren(...(bare ? [] : parts));
    $('props').classList.toggle('empty', bare);
  };
  if (s && s.type === 'layer' && layerById(s.id)) return show(layerProps(stage, layerById(s.id)));
  if (s && s.type === 'link' && layerById(s.id)?.tap) return show(linkProps(stage, layerById(s.id)));
  if (s && s.type === 'main' && mainScreen()) return show(mainProps(stage, mainScreen()));
  if (s && (s.type === 'screen' || s.type === 'back') && screenById(s.id)) return show(screenProps(stage, screenById(s.id)));
  if (s && s.type === 'fn' && FN_ROWS.includes(fnRowOf(s.id))) return show(fnProps(fnRowOf(s.id)));
  show([]);  // nothing selected: no card
}

function layerProps(stage, l) {
  const d = KINDS[l.type], scr = screenOf(l.id);
  const upd = (prop) => (v, key) => change(() => { l[prop] = v; }, { key: l.id + prop });
  const title = h('div', { class: 'props-title' }, l.name);
  refreshers.push(() => { title.textContent = l.name; });  // (a card's name follows its title)
  const groups = [head(ICON[l.type], title, `${d.name} · ${scr.name}`, { title: 'Delete', fn: () => remove(l.id) })];
  if (canLink(l)) groups.push(onTap(stage, l));
  if (LISTED.has(l.type)) {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Position', listOrder(l, scr))));
  } else if (!d.fixed) {
    const base = () => stage.base(l);  // (asked each time: a turn moves it)
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Position'), h('div', { class: 'row' },
      num('X', () => base()[0] + l.dx, (v, k) => upd('dx')(v - base()[0], k), { key: 'x' }),
      num('Y', () => base()[1] + l.dy, (v, k) => upd('dy')(v - base()[1], k), { key: 'y' }))));
  }
  const text = (title, ...extra) => h('div', { class: 'group' }, h('h3', {}, title),
    slider('Size', () => l.size, upd('size'), { min: 8, max: 300, unit: 'px', key: 'size' }),
    slider('Weight', () => l.weight, upd('weight'), { min: 100, max: 1000, step: 10, key: 'weight' }),
    slider('Width', () => l.width, upd('width'), { min: 25, max: 151, key: 'width' }),
    slider('Spacing', () => l.spacing, upd('spacing'), { min: -20, max: 40, unit: 'px', key: 'spacing' }),
    ...extra);
  const scale = (label) => slider(label, () => Math.round(l.scale * 100), (v, k) => upd('scale')(Math.round(v) / 100, k), { min: 30, max: 300, unit: '%', key: 'scale' });
  // A turn round the screen's middle, 0–360° clockwise from the top (kept as -180…180, as the firmware takes it); its
  // place (Position) — once the board's code has drawn it there
  const turn = () => slider('Rotation', () => (Math.round(l.angle ?? 0) + 360) % 360, (v, k) => {
    v = Math.round(v) % 360;
    upd('angle')(v > 180 ? v - 360 : v, k);
    requestAnimationFrame(() => requestAnimationFrame(() => refreshers.forEach((f) => f())));
  }, { min: 0, max: 360, unit: '°', key: 'angle' });
  // The gallery's text: Latin letters (its fonts are HaloWatch's); a card's name follows its title
  // (and the screen it opens, while that is empty and named after it: the board shows that name)
  const words = (label, prop) => textField(label, () => l[prop] ?? '', (v, key) => {
    const named = prop === 'text' && l.type === 'card' && l.name === l.text;
    const own = prop === 'text' && TAPS.has(l.type) && l.tap && screenById(l.tap.to);
    const follows = own && !own.layers.length && own.name === (l.text || '').trim();
    change(() => {
      if (named) l.name = v || 'Card';
      if (follows && v.trim()) own.name = v.trim();
      l[prop] = v;
    }, { key: l.id + prop });
    if (named) renderLayers();  // (not the whole panel: the field keeps the focus)
    if (follows && window.popka?.stage) renderScreens(window.popka.stage);
  });
  if (l.type === 'stars') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Sky'),
      slider('Stars', () => l.count, (v, k) => upd('count')(Math.round(v), k), { min: 10, max: 200, key: 'count' }),
      slider('Brightness', () => Math.round(l.scale * 100), (v, k) => upd('scale')(Math.round(v) / 100, k), { min: 20, max: 200, unit: '%', key: 'scale' }),
      slider('Twinkle', () => Math.round(l.twinkle * 100), (v, k) => upd('twinkle')(Math.round(v) / 100, k), { min: 0, max: 300, unit: '%', key: 'twinkle' }),
      slider('Meteors', () => l.meteors, (v, k) => upd('meteors')(Math.round(v), k), { min: 0, max: 20, unit: '/m', key: 'meteors' }),
      color('Color', () => l.color, upd('color'), 'color')));
  } else if (PIXEL_ART.includes(l.type)) {  // (a trail — only a picture that has one: its length is kept as rainbow)
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Picture'), scale('Size'),
      slider('Rotation', () => Math.round(l.angle), (v, k) => upd('angle')(Math.round(v), k), { min: -180, max: 180, unit: '°', key: 'angle' }),
      'rainbow' in l ? slider('Rainbow', () => l.rainbow, (v, k) => upd('rainbow')(Math.round(v * 2) / 2, k), { min: 1, max: 5, step: 0.5, unit: '×', key: 'rainbow' }) : null,
      segmented(l.type === 'parrot' ? 'Notes' : 'Sparkles', () => l.sparkles, [['off', 'Off'], ['around', 'Around'], ['screen', 'Screen']], (v) => upd('sparkles')(v))));
  } else if (l.type === 'stchart') {  // (the goal and the accent — the Steps pill's too)
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Chart'), scale('Size'),
      num('Goal', () => l.goal, (v, k) => stepsSet('goal')(Math.round(v), k), { step: 500, min: 100, max: 100000, key: 'goal' }),
      color('Accent', () => l.color, stepsSet('accent'), 'color'),
      color('Other days', () => l.color2, upd('color2'), 'color2'),
      color('Letters', () => l.color3, upd('color3'), 'color3')));
  } else if (l.type === 'curve') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Line'),
      choice('Shows', l.what, CURVES, (v) => change(() => { l.what = v; }, { structure: true })),
      turn(),
      slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 230, unit: 'px', key: 'radius' })),
      text('Text', color('Color', () => l.color, upd('color'), 'color')));
  } else if (l.type === 'stepring') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Ring'), scale('Thickness'),
      slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 230, unit: 'px', key: 'radius' }),
      num('Goal', () => l.goal, (v, k) => stepsSet('goal')(Math.round(v), k), { step: 500, min: 100, max: 100000, key: 'goal' }),
      color('Accent', () => l.color, stepsSet('accent'), 'color'),
      color('Track', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'batring') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Ring'), scale('Thickness'),
      slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 230, unit: 'px', key: 'radius' }),
      color('Accent', () => l.color, upd('color'), 'color'),
      color('Track', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'weekarc') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Week'), turn(),
      slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 230, unit: 'px', key: 'radius' }),
      color('Days gone', () => l.color, upd('color'), 'color'),
      color('Days ahead', () => l.color2, upd('color2'), 'color2'),
      color('Today', () => l.color3, upd('color3'), 'color3')), text('Letters'));
  } else if (l.type === 'infoline') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Line'),
      segmented('Format', () => l.format, INFOS, (v) => change(() => { l.format = v; }, { structure: true }))),
      text('Text', color('Color', () => l.color, upd('color'), 'color'),
        l.format === 'icons' ? color('Shoe', () => l.color3, upd('color3'), 'color3') : null));
  } else if (l.type === 'forecast') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Forecast'), turn(), scale('Icons'),
      slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 210, unit: 'px', key: 'radius' }),
      color('Degrees', () => l.color, upd('color'), 'color'),
      color('Hours', () => l.color2, upd('color2'), 'color2')), text('Text'));
  } else if (l.type === 'dateframe') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Frame'),
      slider('Gap', () => l.gap, upd('gap'), { min: 40, max: 330, unit: 'px', key: 'gap' }),
      color('Text', () => l.color, upd('color'), 'color'),
      color('Lines', () => l.color2, upd('color2'), 'color2')), text('Letters'));
  } else if (l.type === 'dotclock') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Dots'), scale('Size'),
      color('Lit', () => l.color, upd('color'), 'color'),
      color('Dark', () => l.color2, upd('color2'), 'color2'),
      color('Colon', () => l.color3, upd('color3'), 'color3')));
  } else if (l.type === 'stnum' || l.type === 'stcap') {
    groups.push(text('Text', color('Color', () => l.color, upd('color'), 'color')));
  } else if (l.type === 'stfoot') {
    groups.push(text('Text', color('Accent', () => l.color, stepsSet('accent'), 'color')));
  } else if (l.type === 'steps') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Pill'), scale('Size'),
      num('Goal', () => l.goal, (v, k) => stepsSet('goal')(Math.round(v), k), { step: 500, min: 100, max: 100000, key: 'goal' }),
      h('div', { class: 'switch-row' }, h('label', {}, 'Background'),
        toggle(l.pill !== false, (on) => change(() => { l.pill = on; }, { structure: true }), 'Background')),
      l.pill !== false ? color('Pill', () => l.color, upd('color'), 'color') : null,
      color('Text', () => l.color2, upd('color2'), 'color2'),
      color('Accent', () => l.color3, stepsSet('accent'), 'color3')));
  } else if (l.type === 'weather' || l.type === 'hourly') {
    if (l.type === 'weather') groups.push(citiesGroup(stage, null, true));
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Pill'), scale('Size'),
      color('Pill', () => l.color, upd('color'), 'color'),
      color('Text', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'wxicon') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Icon'), scale('Size')));
  } else if (l.type === 'dots') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Dots'),
      color('Current', () => l.color, upd('color'), 'color'),
      color('Others', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'wxhilo') {
    groups.push(text('Text', color('High', () => l.color, upd('color'), 'color'), color('Low', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'arc') {
    groups.push(text('Text', slider('Radius', () => l.radius, upd('radius'), { min: 100, max: 240, unit: 'px', key: 'radius' }),
      color('Color', () => l.color, upd('color'), 'color')));
  } else if (l.type === 'title' || l.type === 'edge') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Text'), words('Text', 'text'),
      color(l.type === 'edge' ? 'Button' : 'Color', () => l.color, upd('color'), 'color')));
  } else if (l.type === 'display' || l.type === 'battery' || l.type === 'music' || l.type === 'recorder' || KINDS[l.type]?.game) {
    // (HaloWatch's screen as it is: nothing to set)
  } else if (l.type === 'card') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Card'), words('Title', 'text'), words('Subtitle', 'subtitle'),
      iconPicker(() => l.icon, (v) => change(() => { l.icon = v; })),
      color('Circle', () => l.color, upd('color'), 'color'),
      color('Card', () => l.color2, upd('color2'), 'color2')));
  } else if (l.type === 'scroll') {
    groups.push(h('div', { class: 'group' }, h('h3', {}, 'Indicator'),
      color('Thumb', () => l.color, upd('color'), 'color'),
      color('Track', () => l.color2, upd('color2'), 'color2')));
  } else {
    groups.push(text('Text', color('Color', () => l.color, upd('color'), 'color')));
  }
  if (TIMED.has(l.type)) groups.push(timeGroup());
  return groups;
}

// The time the watch shows — the same for all its clocks and dates: from Wi-Fi (the Wi-Fi function) or from the
// computer (over USB after Flash), the zone, 12 or 24 hours
const TIMED = new Set(['date', 'clock', 'arc']);
function timeGroup() {
  return h('div', { class: 'group' }, h('h3', {}, 'Time'),
    segmented('From', () => (fnOf('wifi').on ? 'wifi' : 'computer'), [['wifi', 'Wi-Fi'], ['computer', 'Computer']],
      (v) => setFn('wifi', { on: v === 'wifi' }, { structure: true })),
    choice('Zone', timeOf().zone, ZONES.map(([n]) => [n, n]), (v) => setTime({ zone: v })),
    segmented('Clock', () => (timeOf().h24 ? '24' : '12'), [['12', '12 h'], ['24', '24 h']], (v) => setTime({ h24: v === '24' })));
}

// A title's or a card's place in its list: a step up, a step down
function listOrder(l, s) {
  const items = listed(s), k = items.indexOf(l);
  if (k < 0) return null;
  return h('span', { class: 'order' },
    h('button', { class: 'icon-btn', title: 'Up', 'aria-label': 'Up', html: ICON.up, disabled: k === 0, onclick: () => moveInList(l.id, k - 1) }),
    h('button', { class: 'icon-btn', title: 'Down', 'aria-label': 'Down', html: ICON.down, disabled: k === items.length - 1, onclick: () => moveInList(l.id, k + 1) }));
}

// A text field (one undo step per field while typing)
function textField(label, get, set, key) {
  const input = h('input', { type: 'text', spellcheck: 'false', maxlength: '39' });
  input.value = get();
  input.addEventListener('input', () => set(input.value.replace(/[^\x20-\x7e]/g, ''), key));
  input.addEventListener('keydown', (e) => e.stopPropagation());
  input.addEventListener('blur', () => { input.value = get(); });
  refreshers.push(() => { if (document.activeElement !== input) input.value = get(); });
  return h('div', { class: 'color' }, h('label', {}, label), h('label', { class: 'field' }, input));
}

// The cards' icons (Font Awesome, as gl_ic24 has them) in a grid
function iconPicker(get, set) {
  const grid = h('div', { class: 'icons' }, ...Object.entries(ICONS).map(([k, code]) => {
    const b = h('button', { class: 'icon-pick', title: k, 'aria-label': k }, String.fromCodePoint(code));
    b.addEventListener('click', () => set(k));
    return b;
  }));
  const mark = () => { for (const b of grid.children) b.classList.toggle('on', b.title === get()); };
  mark();
  refreshers.push(mark);
  return h('div', { class: 'color' }, h('label', {}, 'Icon'), grid);
}

// Go to: which screen a tap on this layer opens (a layer of the face — over it, Fade; a card or the button — Android 16)
function onTap(stage, l) {
  const others = targetsFor(l), gallery = TAPS.has(l.type);
  const value = l.tap && others.some((s) => s.id === l.tap.to) ? l.tap.to : '';
  return h('div', { class: 'group' }, choice('Go to', value, [['', 'Nothing'], ...others.map((s) => [s.id, s.name]), ...newChoices(l)], (v) => {
    if (v === NEW) return addScreen(stage, l.id);
    if (v === NEW_LIST) return addScreen(stage, l.id, 'list');
    change(() => { l.tap = v ? { to: v, fx: gallery ? 'a16' : l.tap?.fx || 'fade' } : null; }, { structure: true });
    renderProps(stage, true);
  }));
}

// A link (a line from a layer to a screen): where it goes
function linkProps(stage, l) {
  const to = screenById(l.tap.to), from = screenOf(l.id);
  return [
    head(ICON.link, `${l.name} → ${to.name}`, `Link · ${from.name}`,
      { title: 'Remove the link', fn: () => { change(() => { l.tap = null; }, { structure: true }); select(null); } }),
    h('div', { class: 'group' }, choice('Go to', to.id, [...targetsFor(l).map((s) => [s.id, s.name]), ...newChoices(l)], (v) => {
      if (v === NEW) return addScreen(stage, l.id);
      if (v === NEW_LIST) return addScreen(stage, l.id, 'list');
      change(() => { l.tap.to = v; }, { structure: true });
      renderProps(stage, true);
    })),
  ];
}

// The gallery: a swipe up on the face opens it
function mainProps(stage, m) {
  return [
    head(ICON.gallery, `${home().name} → ${m.name}`, 'Swipe up',
      { title: 'Remove', fn: () => { change((p) => { p.main = null; }, { structure: true }); select(null); } }),
    h('div', { class: 'group' }, mainChoice(stage)),
  ];
}

// Which screen a swipe up on the face opens
function mainChoice(stage) {
  const m = mainScreen();
  return choice('Swipe up', m ? m.id : '', [['', 'Nothing'], ...screens().slice(1).map((s) => [s.id, s.name])], (v) => {
    change((p) => {
      p.main = v || null;
      for (const l of p.screens[0].layers) if (l.tap && l.tap.to === v) l.tap = null;  // the gallery is not opened by a tap
    }, { structure: true });
    renderProps(stage, true);
  });
}

// A function: its switch in the head; Always On — after how long idle, how bright; a button — what a press does (a lock
// is Always On with the touch off; a long press of that button unlocks)
const AFTER = [[10, '10 s'], [15, '15 s'], [30, '30 s'], [60, '1 m']];
// What brings the screen back from Always On or dark (power.h: FN_WAKE_*) — whether the watch went there by itself or
// by a button: a touch, picking it up, a button. At least one stays on (its switch can't be turned off); with a Steps
// layer the motion sensor is the pedometer's — no Raise
const WAKE = [['tap', 'Tap', 'tap'], ['raise', 'Raise', 'raise'], ['btn', 'Buttons', 'buttons']];
function wakeGroup() {
  const f = fnOf('aod'), steps = screens().some((s) => s.layers.some((l) => STEP_FAMILY.has(l.type)));
  const on = (k) => (k === 'raise' ? f.raise && !steps : k === 'btn' ? f.btn || !(f.tap || (f.raise && !steps)) : f[k]);
  const left = WAKE.filter(([k]) => on(k)).length;
  return h('div', { class: 'group' }, h('h3', {}, 'Wake up'), h('ul', { class: 'layers wake' }, ...WAKE.map(([k, name, icon]) => {
    const sw = toggle(on(k), (v) => setFn('aod', { [k]: v }, { structure: true }), name);
    if ((k === 'raise' && steps) || (on(k) && left === 1)) sw.disabled = true;  // (the last way; Steps has the sensor)
    if (k === 'raise' && steps) sw.title = 'Steps has the motion sensor';
    return h('li', { class: 'layer fn' + (on(k) ? '' : ' off') }, h('span', { class: 'layer-icon', html: ICON[icon] }),
      h('span', { class: 'layer-name' }, name), h('span'), sw);
  })));
}

// What a button does: nothing, the screen off, Always On (the button again — back), Always On locked (its long press —
// back)
const PRESS = [['none', 'Nothing'], ['off', 'Screen off'], ['aod', 'Always On'], ['lock', 'Lock']];
function pressOf(id) {
  const now = () => (fnOf(id).on ? fnOf(id).act : 'none');
  const buttons = PRESS.map(([v, t]) => {
    const b = h('button', { role: 'radio' }, t);
    b.addEventListener('click', () => setFn(id, v === 'none' ? { on: false } : { on: true, act: v }, { structure: true }));
    return b;
  });
  const mark = () => buttons.forEach((b, i) => {
    const on = PRESS[i][0] === now();
    b.classList.toggle('on', on), b.setAttribute('aria-checked', String(on));
  });
  mark();
  refreshers.push(mark);
  return h('div', { class: 'seg grid2', role: 'radiogroup', 'aria-label': FUNCTIONS[id].name }, ...buttons);
}
function fnProps(id) {
  if (id === 'buttons') {
    const f = fnRow(id);
    return [head(ICON.buttons, 'Buttons', 'Function', toggle(f.on, (on) => setRow(id, on), 'Buttons')),
      h('div', { class: 'group' }, h('h3', {}, 'BOOT'), pressOf('boot')),
      h('div', { class: 'group' }, h('h3', {}, 'PWR'), pressOf('pwr'))];
  }
  const f = fnOf(id), set = (v) => { setFn(id, v, { structure: true }); };
  const parts = [head(ICON[id], f.name, 'Function', toggle(f.on, (on) => set({ on }), f.name))];
  if (id === 'aod') {
    parts.push(h('div', { class: 'group' + (f.on ? '' : ' dim-off') },
      segmented('After', f.idle, AFTER, (v) => set({ idle: v })),
      slider('Brightness', () => Math.round(fnOf(id).bright * 100 / 255), (v, key) => setFn(id, { bright: Math.max(1, Math.round(v * 255 / 100)) }, { key }),
        { min: 2, max: 50, unit: '%', key: 'aod-bright' })),
      wakeGroup());
  }
  return parts;
}

// A screen: its name (typed in its head), the home — what a swipe up opens
function screenProps(stage, s) {
  const face = s === home(), main = s === mainScreen();
  const name = h('input', { class: 'props-title', type: 'text', value: s.name, spellcheck: 'false', 'aria-label': 'Name' });
  name.addEventListener('input', () => { const v = name.value.trim(); if (v) change(() => { s.name = v; }, { key: s.id + 'name' }); });
  name.addEventListener('change', () => emit('structure'));
  name.addEventListener('keydown', (e) => { e.stopPropagation(); if (e.key === 'Enter') name.blur(); });
  return [
    head(face ? ICON.home : main ? ICON.gallery : ICON.screen, name, face ? 'Home' : main ? 'Gallery' : 'Screen',
      face ? null : { title: 'Delete the screen', fn: () => removeScreen(s.id) }),
    face ? h('div', { class: 'group' }, mainChoice(stage)) : null,
    !face && !main && !isGallery(s) && !s.layers.some((l) => KINDS[l.type]?.group === 'section')
      ? h('div', { class: 'group' }, h('button', { class: 'primary', onclick: () => makeHome(stage, s) }, 'Make it the watch face')) : null,
    weatherPaged(s) ? citiesGroup(stage, s) : null,
  ];
}

// ---------- The weather's cities: the Weather screen's pages, one each (the first is the face's pill too). A click on
// a row — its page; the pen — another city in its place, the bin — the page goes (the screen stays: one city at
// least); + — one more page (WX_MAX at most). The search: a city's name or a US ZIP (studio.py /api/geo: Open-Meteo,
// Zippopotam) ----------
const cityEdit = { at: null, q: '', results: [], err: '' };  // at: the row whose city is being picked, -1 — a new one
let cityTimer = 0, citySeq = 0;

function citiesGroup(stage, s, first = false) {
  const cities = citiesOf(), cur = s && s.id === state.screen ? Math.min(state.page, cities.length - 1) : -1;
  const open = (k) => {
    cityEdit.at = cityEdit.at === k ? null : k, cityEdit.q = '', cityEdit.results = [], cityEdit.err = '';
    renderProps(stage, true);
  };
  const cityRows = [];
  const rows = (first ? cities.slice(0, 1) : cities).flatMap((c, k) => {
    const pen = h('button', { class: 'icon-btn', title: 'Another city', 'aria-label': 'Another city', html: ICON.pen });
    pen.addEventListener('click', (e) => { e.stopPropagation(); open(k); });
    const bin = cities.length > 1 && !first ? h('button', { class: 'icon-btn', title: 'Remove this city', 'aria-label': 'Remove this city', html: ICON.trash }) : null;
    bin?.addEventListener('click', (e) => {
      e.stopPropagation();
      cityEdit.at = null;
      setWeather((w) => w.cities.splice(k, 1));
    });
    const row = h('li', { class: 'layer city' + (k === cur ? ' sel' : '') + (cityEdit.at === k ? ' editing' : '') },
      h('span', { class: 'layer-icon', html: ICON.city }), h('span', { class: 'layer-name' }, c.name),
      h('span', { class: 'tag', title: c.where }, c.where), h('span', { class: 'acts' }, pen, bin));
    if (s) row.addEventListener('click', () => stage.showPage(s, k));
    cityRows.push(row);
    return cityEdit.at === k ? [row, citySearch(stage, k)] : [row];
  });
  if (!first && cityEdit.at === null) sortable(cityRows, (from, to) => { moveCity(from, to); stage.keepPage(); });
  const add = !first && cities.length < WX_MAX
    ? h('button', { class: 'icon-btn', title: 'Add a city', 'aria-label': 'Add a city', html: ICON.plus, onclick: () => open(-1) }) : null;
  return h('div', { class: 'group' }, h('h3', {}, first ? 'City' : 'Cities', add),
    h('ul', { class: 'layers cities' }, ...rows, cityEdit.at === -1 && !first ? citySearch(stage, -1) : null),
    first ? null : segmented('Units', unitsOf, [['C', '°C'], ['F', '°F']], (v) => setWeather((w) => { w.units = v; })));
}

// The search under a row (or under the list — a new city): results as you type
function citySearch(stage, k) {
  const input = h('input', { type: 'text', value: cityEdit.q, placeholder: 'City or US ZIP', spellcheck: 'false', autocomplete: 'off', 'aria-label': 'City or US ZIP' });
  const list = h('ul', { class: 'layers results' });
  const pick = (r) => {
    const n = citiesOf().length;
    cityEdit.at = null;
    setWeather((w) => {
      const c = { name: r.name, where: r.where, lat: r.lat, lon: r.lon };
      if (k < 0) w.cities.push(c);
      else w.cities[k] = c;
    });
    const s = activeScreen();
    if (k < 0 && paged(s)) stage.showPage(s, n);  // (the new city's page)
  };
  const draw = () => list.replaceChildren(...(cityEdit.err ? [h('li', { class: 'empty' }, cityEdit.err)]
    : cityEdit.results.map((r) => {
      const li = h('li', { class: 'layer' }, h('span', { class: 'layer-icon', html: ICON.city }),
        h('span', { class: 'layer-name' }, r.name), h('span', { class: 'tag', title: r.where }, r.where), h('span'));
      li.addEventListener('click', (e) => { e.stopPropagation(); pick(r); });
      return li;
    })));
  const find = async () => {
    const q = cityEdit.q.trim(), seq = ++citySeq;
    if (q.length < 2) return (cityEdit.results = [], cityEdit.err = ''), draw();
    try {
      const r = await fetch(`/api/geo?q=${encodeURIComponent(q)}`), d = await r.json();
      if (seq !== citySeq) return;  // (a newer search is on its way)
      cityEdit.results = d.results || [], cityEdit.err = !r.ok ? d.error || 'No answer' : cityEdit.results.length ? '' : 'Nothing found';
    } catch (e) {
      cityEdit.results = [], cityEdit.err = 'No answer';
    }
    draw();
  };
  input.addEventListener('input', () => { cityEdit.q = input.value; clearTimeout(cityTimer); cityTimer = setTimeout(find, 300); });
  input.addEventListener('keydown', (e) => {
    e.stopPropagation();
    if (e.key === 'Enter' && cityEdit.results[0]) pick(cityEdit.results[0]);
    if (e.key === 'Escape') cityEdit.at = null, renderProps(stage, true);
  });
  draw();
  setTimeout(() => input.focus());
  return h('li', { class: 'city-search' }, h('label', { class: 'field' }, input), list);
}

export { layers };
