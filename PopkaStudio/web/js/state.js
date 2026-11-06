// state.js — the project being edited, the selection, undo and redo, saving.
//
// The project (the same JSON studio.py turns into the board's layers.h): screens — the first is the watch face (the
// home), the others open over it. A screen: {id, name, x, y (its place on the map), layers (bottom to top)}. A layer:
//   {id, type, name, hidden, dx, dy, tap, ...} — one of HaloWatch's parts (KINDS): the watch face's date, clock and
//   weather pill, the Weather screen's clock along the top edge, city, icon, temperature, high/low, hourly pill, dots.
//   Text — size (px), weight (wght), width (wdth), spacing (px), color (the high/low: color2 — the low; the clock
//   along the edge: radius); the pills — scale (1 — as on HaloWatch), spacing, color (the pill), color2 (the text); the
//   icon — scale; the dots — color (the current one), color2 (the others). dx, dy — moved from HaloWatch's place.
//   tap (a layer of the watch face) — {to: screen id, fx: "fade" | "zoom" | "slide"} or null: a tap opens that screen.
//   The gallery's parts (HaloWatch's title, cards, More button, scroll indicator) — HaloWatch's fonts and places (they
//   don't move: the list places them), text, subtitle (a card's line under its title), icon (a key of ICONS), color,
//   color2; a card's or the button's tap — {to, fx: "a16"}: the Android 16 transition (none — a screen with its title).
// main — the screen a swipe up on the face opens (HaloWatch's gallery), or null.
// functions — the watch's Functions (firmware/PopkaWatch/power.h): {aod: {on, idle (s), bright (the panel in Always On,
// of 255), tap, raise, btn (what wakes it), home (out of it on the watch face)}, boot: {on, act}, pwr: {on, act}}; act —
// 'off' (the screen off) or 'lock' (Always On, locked).
// Anything that changes the project goes through change(): it keeps the undo history and saves (to studio.py).

// HaloWatch's look of each kind (studio.py: DEFAULTS — the same numbers); group — where the Elements rail has it
export const KINDS = {
  date: { group: 'face', name: 'Date line', sub: 'Weekday, day, month', size: 41, weight: 700, width: 100, spacing: 0, color: '#C2C3CB' },
  clock: { group: 'face', name: 'Big clock', sub: 'Hours and minutes, 12 h', size: 148, weight: 580, width: 106, spacing: -1, color: '#F8F7F9' },
  weather: { group: 'face', name: 'Weather pill', sub: 'Pill: icon and temperature', scale: 1, spacing: 0, color: '#2D2F2F', color2: '#F8F7F9' },
  // Not on HaloWatch (PopkaStudio's): today's steps — the motion chip counts them; in the firmware only with this layer
  // Not on HaloWatch (projects/Orbit's): a starry background — twinkling stars, a meteor now and then; the bottom layer
  stars: { group: 'face', fixed: true, round: true, name: 'Stars', sub: 'A starry background: twinkling, meteors (Orbit\'s)', scale: 1, count: 90, twinkle: 1, meteors: 6, color: '#C8DCFF' },
  // Not on HaloWatch: pixel art drawn cell by cell (pixelart.h) — Popka, the parrot, and the notes it sings
  parrot: { group: 'face', name: 'Popka', sub: 'Pixel art: the parrot flaps its wings and sings', scale: 1, angle: 0, sparkles: 'around' },
  steps: { group: 'face', name: 'Steps pill', sub: 'Pill: a ring to the goal round a shoe, and today\'s steps', scale: 1, spacing: 0, color: '#2D2F2F', color2: '#F8F7F9', color3: '#FF8A80', goal: 10000, pill: true },
  arc: { group: 'weather', name: 'Top clock', sub: 'The time along the top edge', size: 24, weight: 620, width: 104, spacing: 1, radius: 219, color: '#C6C6C6' },
  city: { group: 'weather', name: 'City', sub: 'The page\'s city', size: 34, weight: 620, width: 104, spacing: 0, color: '#F9F9F9' },
  wxicon: { group: 'weather', name: 'Weather icon', sub: 'Left of the temperature', scale: 1 },
  wxtemp: { group: 'weather', name: 'Temperature', sub: 'Now, in big digits', size: 91, weight: 660, width: 100, spacing: 0, color: '#F9F9F9' },
  wxhilo: { group: 'weather', name: 'High / low', sub: 'Right of the temperature', size: 37, weight: 660, width: 100, spacing: 0, color: '#F9F9F9', color2: '#A2A2A2' },
  hourly: { group: 'weather', name: 'Hourly', sub: 'Pill: the next three hours', scale: 1, spacing: 0, color: '#2E3132', color2: '#F9F9F9' },
  // HaloWatch's Settings sections: each fills its screen, behind a gallery card (SECTIONS; App → Display / Battery)
  display: { group: 'section', fixed: true, round: true, name: 'Display', sub: 'HaloWatch\'s Display: the brightness slider' },
  battery: { group: 'section', fixed: true, round: true, name: 'Battery', sub: 'The level along the edge and how long it lasts' },
  music: { group: 'section', fixed: true, round: true, name: 'Music', sub: 'HaloWatch\'s Music: the player, real music from the flash' },
  recorder: { group: 'section', fixed: true, round: true, name: 'Recorder', sub: 'Record with the microphones, play it back, forget it' },
  snkdots: { group: 'section', fixed: true, round: true, game: true, name: 'Dot Snake', sub: 'A self-playing snake on a matrix of dots' },
  snkspace: { group: 'section', fixed: true, round: true, game: true, name: 'Comet', sub: 'A self-playing comet snake in space: a black hole, asteroids' },
  snkiso: { group: 'section', fixed: true, round: true, game: true, name: 'Voxel Snake', sub: 'A self-playing snake of cubes on an isometric board' },
  // The Steps screen's parts (PopkaStudio's own, stepscreen.h): each on both pages — today and this week; one goal and
  // one accent in all of them and in the Steps pill (stepsSet)
  stnum: { group: 'steps', name: 'Number', sub: 'Today\'s steps; on the week page — the average a day', size: 64, weight: 660, width: 100, spacing: 0, color: '#FFF8EE', goal: 10000 },
  stcap: { group: 'steps', name: 'Caption', sub: '"of 10,000 today" and "a day this week"', size: 20, weight: 620, width: 104, spacing: 0, color: '#9E9E9E', goal: 10000 },
  stchart: { group: 'steps', name: 'Chart', sub: 'Today by the hour, the week by the day, the goal dashed', scale: 1, color: '#FF8A80', color2: '#4A4C52', color3: '#7A7A7A', goal: 10000 },
  // The face's widgets from the twenty mockups (widgets.h): in the Face folder as ready pieces (FACE_PRESETS)
  curve: { group: 'widget', name: 'Curved text', sub: 'A line along the edge: the date, the weather or the steps', what: 'shortdate', angle: 0, radius: 196, size: 19, weight: 650, width: 100, spacing: 3, color: '#9E9E9E' },
  stepring: { group: 'widget', name: 'Steps ring', sub: 'Today\'s steps to the goal along the edge', scale: 1, radius: 224, color: '#FF8A80', color2: '#2A2B2E', goal: 10000 },
  // As Steps ring, the battery's charge (the power chip's percent; widgets.h FL_BATRING)
  batring: { group: 'widget', name: 'Battery ring', sub: 'The battery\'s charge along the edge', scale: 1, radius: 224, color: '#6DD58C', color2: '#2A2B2E' },
  weekarc: { group: 'widget', name: 'Week', sub: 'The days along the edge, today in a circle', angle: 0, radius: 190, size: 18, weight: 650, width: 100, spacing: 0, color: '#9E9E9E', color2: '#5E5F64', color3: '#FF8A80' },
  infoline: { group: 'widget', name: 'Info line', sub: 'The weather and the steps in a line', format: 'words', size: 18, weight: 620, width: 100, spacing: 0, color: '#9E9E9E', color3: '#FF8A80' },
  forecast: { group: 'widget', name: 'Forecast', sub: 'The next five hours along the edge', angle: 0, scale: 0.5, radius: 168, size: 15, weight: 700, width: 100, spacing: 0, color: '#FFF8EE', color2: '#5E5F64' },
  dateframe: { group: 'widget', name: 'Date frame', sub: 'The weekday and the date with lines round the clock', gap: 160, size: 15, weight: 650, width: 100, spacing: 4, color: '#9E9E9E', color2: '#3A3B40' },
  dotclock: { group: 'widget', name: 'Dot clock', sub: 'The time in dots, as a station\'s board', scale: 1, color: '#FFF8EE', color2: '#1E1F23', color3: '#FF8A80' },
  stfoot: { group: 'steps', name: 'Footer', sub: 'What is left to the goal, the days at the goal', size: 20, weight: 620, width: 104, spacing: 0, color: '#FF8A80', goal: 10000 },
  feels: { group: 'weather', name: 'Feels like', sub: 'What it feels like, under the hourly pill', size: 20, weight: 620, width: 104, spacing: 0, color: '#A2A2A2' },
  dots: { group: 'weather', name: 'Page dots', sub: 'A dot a city, the worm', color: '#FFFFFF', color2: '#FFFFFF' },
  // The gallery (HaloWatch.ino): fixed — the list places them, in HaloWatch's fonts; a new card — a plain one
  title: { group: 'gallery', fixed: true, name: 'Title', sub: 'The list\'s heading', text: 'Gallery', color: '#FFF8EE' },
  card: { group: 'gallery', fixed: true, name: 'Card', sub: 'Icon, title, a line under it; opens a screen', text: 'Card', subtitle: 'Description', icon: 'star', color: '#A8C7FA', color2: '#2E3034' },
  edge: { group: 'gallery', fixed: true, name: 'More button', sub: 'At the bottom edge, grows at the list\'s end', text: 'More', color: '#2E3034' },
  scroll: { group: 'gallery', fixed: true, name: 'Scroll indicator', sub: 'On the right while the list moves', color: '#C6C6C6', color2: '#333333' },
};
export const KIND_INDEX = { date: 0, clock: 1, weather: 2, arc: 3, city: 4, wxicon: 5, wxtemp: 6, wxhilo: 7, hourly: 8, dots: 9, title: 10, card: 11, edge: 12, scroll: 13, steps: 14, stars: 15, feels: 17, display: 18, battery: 19, stnum: 20, stcap: 21, stfoot: 22, stchart: 23, curve: 24, stepring: 25, weekarc: 26, infoline: 27, forecast: 28, dateframe: 29, dotclock: 30, music: 31, recorder: 32, snkdots: 33, snkspace: 34, snkiso: 35, batring: 36, parrot: 37 };
// The pixel art layers (pixelart.h): what is around the picture, its trail
export const PIXEL_ART = ['parrot'];
export const CURVES = [['weekday', 'Weekday'], ['shortdate', 'Day and date'], ['daymonth', 'Date'], ['weather', 'Weather'], ['steps', 'Steps'], ['datesteps', 'Date and steps'], ['battery', 'Battery']];
export const INFOS = [['words', 'Words'], ['short', 'Short'], ['icons', 'Icons']];
// The Face folder's ready pieces from the twenty face mockups: each — layers with their looks (a kind and what differs)
const BLUE = '#A8C7FA', CORAL = '#FF8A80', GREEN = '#6DD58C';
export const FACE_PRESETS = {
  pDateArc: { name: 'Curved date', icon: 'curve', parts: [['curve', { name: 'Curved date', what: 'shortdate' }]] },
  pStepsRing: { name: 'Steps ring', icon: 'stepring', parts: [['stepring', {}], ['curve', { name: 'Curved steps', what: 'steps', angle: 180, color: CORAL }]] },
  pBatRing: { name: 'Battery ring', icon: 'batring', parts: [['batring', {}], ['curve', { name: 'Curved battery', what: 'battery', angle: 180, color: GREEN }]] },
  pWeekday: { name: 'Curved weekday', icon: 'curve', parts: [['curve', { name: 'Curved weekday', what: 'weekday', radius: 200, size: 20, spacing: 4 }]] },
  pDayMonth: { name: 'Curved day & month', icon: 'curve', parts: [['curve', { name: 'Curved day & month', what: 'daymonth', angle: 180, radius: 200, size: 20, spacing: 4 }]] },
  pWxArc: { name: 'Curved weather', icon: 'curve', parts: [['curve', { name: 'Curved weather', what: 'weather', angle: -90, radius: 200, size: 20, color: BLUE }]] },
  pStepsArc: { name: 'Curved steps', icon: 'curve', parts: [['curve', { name: 'Curved steps', what: 'steps', angle: 90, radius: 200, size: 20, color: CORAL }]] },
  pBatArc: { name: 'Curved battery', icon: 'curve', parts: [['curve', { name: 'Curved battery', what: 'battery', angle: 90, radius: 200, size: 20, color: GREEN }]] },
  pDateSteps: { name: 'Curved date & steps', icon: 'curve', parts: [['curve', { name: 'Curved date & steps', what: 'datesteps', angle: 180, radius: 200, size: 18, spacing: 2 }]] },
  pWeek: { name: 'Curved week', icon: 'weekarc', parts: [['weekarc', { name: 'Curved week' }]] },
  pInfoWords: { name: 'Weather & steps', icon: 'infoline', parts: [['infoline', { name: 'Weather & steps' }]] },
  pForecast: { name: 'Hourly forecast', icon: 'forecast', parts: [['forecast', { name: 'Hourly forecast' }]] },
  pDateFrame: { name: 'Date frame', icon: 'dateframe', parts: [['dateframe', {}]] },
  pInfoShort: { name: 'Weather & steps, short', icon: 'infoline', parts: [['infoline', { name: 'Weather & steps, short', format: 'short', color: CORAL }]] },
  pDotClock: { name: 'Dot clock', icon: 'dotclock', parts: [['dotclock', {}]] },
  pInfoIcons: { name: 'Weather & steps, icons', icon: 'infoline', parts: [['infoline', { name: 'Weather & steps, icons', format: 'icons', size: 20, weight: 700, color: '#FFF8EE', dy: -32 }]] },
};
// A whole face from the mockups: "Four sides" — the clock in the middle, the weekday, the date, the weather and the
// steps along the four sides (mockup 2)
export const FOUR_SIDES = [
  ['clock', { size: 116, weight: 560, width: 96, dy: 13 }],
  ['curve', { name: 'Curved weekday', what: 'weekday', radius: 200, size: 20, spacing: 4 }],
  ['curve', { name: 'Curved day & month', what: 'daymonth', angle: 180, radius: 200, size: 20, spacing: 4 }],
  ['curve', { name: 'Curved weather', what: 'weather', angle: -90, radius: 200, size: 20, color: BLUE }],
  ['curve', { name: 'Curved steps', what: 'steps', angle: 90, radius: 200, size: 20, color: CORAL }],
];
export const LOOK = ['size', 'weight', 'width', 'scale', 'spacing', 'radius', 'color', 'color2', 'color3', 'goal', 'count', 'twinkle', 'meteors', 'angle', 'sparkles', 'rainbow', 'pill', 'what', 'format', 'gap'];  // what a layer copies from its kind
export const TEXTS = ['text', 'subtitle', 'icon'];  // and its text (a new layer only: "HaloWatch look" keeps it)
// The gallery: a screen with any of these is a list, in the order of its layers (gallery.h); a tap on these opens
export const LISTED = new Set(['title', 'card']);
export const TAPS = new Set(['card', 'edge']);
// The cards' icons: Font Awesome (studio.py: ICONS — the same list); the first five — HaloWatch's cards'
export const ICONS = {
  chevron: 0xF054, bell: 0xF0F3, music: 0xF001, sun: 0xF185, gear: 0xF013, heart: 0xF004, moon: 0xF186, gauge: 0xF624,
  clock: 0xF017, calendar: 0xF073, envelope: 0xF0E0, phone: 0xF095, wifi: 0xF1EB, location: 0xF3C5, user: 0xF007,
  house: 0xF015, search: 0xF002, play: 0xF04B, chart: 0xF201, pulse: 0xF21E, battery: 0xF240, bolt: 0xF0E7,
  volume: 0xF028, contrast: 0xF042, gamepad: 0xF11B, camera: 0xF030, coffee: 0xF0F4, run: 0xF70C, snow: 0xF2DC,
  drop: 0xF043, cloud: 0xF0C2, fire: 0xF06D, star: 0xF005, bulb: 0xF0EB, car: 0xF1B9, plane: 0xF072, stopwatch: 0xF2F2,
  image: 0xF03E, chat: 0xF086, wrench: 0xF0AD, lock: 0xF023, menu: 0xF0C9, tv: 0xF26C, mic: 0xF130,
};
// The Android 16 transition (lib/Halo: halo_nav_open) — a card's link
export const A16 = { name: 'Push', sub: 'as on HaloWatch: the screen comes in from the right growing from 0.8, the gallery shrinks to 0.85, moves left and dims' };
// Paged: on a screen with any of these the screen pages sideways through its cities (one a page), as HaloWatch's Weather
export const PAGED = new Set(['city', 'wxicon', 'wxtemp', 'wxhilo', 'hourly', 'feels']);
// HaloWatch's watch face: its parts bottom to top, as studio.py's default project has them
export const FACE_SCREEN = ['date', 'clock', 'weather'];
// HaloWatch's Weather screen: its parts bottom to top, as weather.h draws them
export const WEATHER_SCREEN = ['arc', 'city', 'wxicon', 'wxtemp', 'wxhilo', 'hourly', 'feels', 'dots'];
// A new gallery: the title, one card, the button at the bottom edge and the clock along the top edge (bottom to top,
// as HaloWatch's menu_draw draws them); more cards — the apps' (App: each adds its card)
export const GALLERY_SCREEN = ['title', 'card', 'edge', 'arc'];
// lib/Halo's transitions for a screen opened over the face (halo_nav_face_open); HaloWatch's pill — Fade
export const FX = {
  fade: { name: 'Fade', index: 1, sub: 'as on HaloWatch: the face fades out, the screen grows from 0.92 and fades in' },
  zoom: { name: 'Zoom', index: 0, sub: 'the screen grows out of the element' },
  slide: { name: 'Slide', index: 2, sub: 'everything moves up: the screen comes from below' },
};
// The Functions, as power.h has them by default (studio.py: FUNCTIONS — the same)
export const FUNCTIONS = {
  aod: { name: 'Always On', on: true, idle: 15, bright: 24, tap: true, raise: true, btn: true, home: false },  // + how it wakes, on the face or not
  boot: { name: 'BOOT button', on: true, act: 'off' },
  pwr: { name: 'PWR button', on: true, act: 'lock' },
  wifi: { name: 'Wi-Fi', on: true },       // the time from the internet; the network — kept by studio.py, not here
  bt: { name: 'Bluetooth', on: false },    // the watch shows itself over Bluetooth (bt.h)
};
// The time (studio.py: ZONES, TIME): the zone the board keeps (iana — for the page's copies of the board) and 12 or
// 24 hours. From Wi-Fi (the Wi-Fi function on) or from the computer (over USB after Flash)
export const ZONES = [
  ['New York', 'America/New_York'], ['Chicago', 'America/Chicago'], ['Denver', 'America/Denver'],
  ['Los Angeles', 'America/Los_Angeles'], ['London', 'Europe/London'], ['Berlin', 'Europe/Berlin'], ['Kyiv', 'Europe/Kyiv'],
  ['Moscow', 'Europe/Moscow'], ['Dubai', 'Asia/Dubai'], ['Tokyo', 'Asia/Tokyo'], ['Sydney', 'Australia/Sydney'], ['UTC', 'UTC'],
];
export const TIME = { zone: 'New York', h24: false };
export const timeOf = () => ({ ...TIME, ...(state.project.time || {}) });
export function setTime(values) {
  change((p) => { p.time = { ...timeOf(), ...values }; }, { structure: true });
}
// How far the picked zone is ahead of this computer's, s (the page's copies show the board's wall clock). Worked out once
// a minute for a zone (every copy asks on every change of the project: Intl is slow)
const shifts = new Map(), zoneFormats = new Map();
export function zoneShift(name) {
  const minute = Math.floor(Date.now() / 60000), hit = shifts.get(name);
  if (hit && hit[0] === minute) return hit[1];
  const v = zoneShiftNow(name);
  shifts.set(name, [minute, v]);
  return v;
}
function zoneShiftNow(name) {
  const iana = (ZONES.find(([n]) => n === name) || ZONES[0])[1], now = new Date();
  try {
    if (!zoneFormats.has(iana)) zoneFormats.set(iana, new Intl.DateTimeFormat('en-US', {
      timeZone: iana, hourCycle: 'h23', year: 'numeric', month: 'numeric', day: 'numeric', hour: 'numeric', minute: 'numeric',
    }));
    const parts = zoneFormats.get(iana).formatToParts(now);
    const g = (k) => +parts.find((q) => q.type === k).value;
    const zone = (Date.UTC(g('year'), g('month') - 1, g('day'), g('hour'), g('minute')) - Math.floor(now / 60000) * 60000) / 1000;
    return Math.round((zone + now.getTimezoneOffset() * 60) / 60) * 60;
  } catch (e) {
    return 0;
  }
}
// A function's settings: the project's, the missing ones from FUNCTIONS
export const fnOf = (id) => ({ ...FUNCTIONS[id], ...(state.project.functions?.[id] || {}) });
// Wi-Fi and Bluetooth: one at a time — the board's internal memory has room for one of them next to the panel's
// buffers (Bluetooth keeps ~71 KB, Wi-Fi needs ~65 KB to start; 103 KB free): one on — the other off
const ALONE = { wifi: 'bt', bt: 'wifi' };
export function setFn(id, values, opts) {
  change((p) => {
    p.functions ??= {}, p.functions[id] = { ...p.functions[id], ...values };
    if (values.on && ALONE[id]) p.functions[ALONE[id]] = { ...p.functions[ALONE[id]], on: false };
  }, opts);
}

export const TILE = 466;  // a screen on the map, map units (board pixels)
export const GAP = 260;   // between screens on the map
export let PAGES = 1;                 // the pages of a screen with the Weather screen's parts: one a city (emit keeps it)
export const PAGE_STEP = TILE + GAP;  // on the map its pages stand in a row, as far apart as screens
export const STEP_PAGES = ['Today', 'This week'];  // the Steps screen's pages (stepscreen.h)
export const MAX_SCREENS = 24;        // what the engine has room for (studio.py: MAX_SCREENS)
// A screen that pages sideways: the Weather screen (a city a page) or the Steps screen (today, the week)
export const weatherPaged = (s) => !!s && s.layers.some((l) => PAGED.has(l.type));
export const STEPS_PARTS = new Set(['stnum', 'stcap', 'stfoot', 'stchart']);  // the Steps screen's (stepscreen.h)
export const STEPS_SCREEN = ['stchart', 'stnum', 'stcap', 'stfoot', 'dots', 'arc'];  // a new one, bottom to top
export const stepsPaged = (s) => !!s && s.layers.some((l) => STEPS_PARTS.has(l.type));
export const paged = (s) => weatherPaged(s) || stepsPaged(s);
export const pagesOf = (s) => (weatherPaged(s) ? PAGES : stepsPaged(s) ? STEP_PAGES.length : 1);
export const pageName = (s, k) => (weatherPaged(s) ? citiesOf()[k]?.name : stepsPaged(s) ? STEP_PAGES[k] : '') || `Page ${k + 1}`;
export const span = (s) => (paged(s) ? TILE + (pagesOf(s) - 1) * PAGE_STEP : TILE);  // how wide a screen is on the map

// sel: {type: 'layer' | 'screen' | 'link' | 'main' | 'fn', id} — a layer, a screen, a link (by its layer), the swipe up
// to the gallery, a function (its key in FUNCTIONS); screen — the screen the Layers panel and the Elements work on
// page — the active screen's page picked (a screen with the Weather screen's parts: its city; Edit and Try show it)
export const state = { pid: 'main', project: null, rev: 0, sel: null, screen: 'face', page: 0, mode: 'edit', view: { x: 0, y: 0, z: 1 } };

const listeners = new Set();
export function on(fn) {
  listeners.add(fn);
}
// what: 'project' (something changed), 'structure' (screens or layers added, removed, reordered, renamed), 'select',
// 'screen' (another active screen), 'mode', 'view'
export function emit(what) {
  if (state.project) PAGES = citiesOf().length;
  for (const fn of listeners) fn(what);
}

// ---------- The weather: the cities — the Weather screen's pages, one each (the first is the face's pill too) — and
// °C or °F (studio.py: clean_weather, CITY, WX_MAX). The numbers: Open-Meteo through studio.py, the same the board asks
// for itself over Wi-Fi ----------
export const WX_MAX = 6;
export const CITY = { name: 'New York', where: 'New York, United States', lat: 40.71427, lon: -74.00597 };
export const citiesOf = () => (state.project?.weather?.cities?.length ? state.project.weather.cities : [CITY]);
export const unitsOf = () => (state.project?.weather?.units === 'F' ? 'F' : 'C');
export function setWeather(fn) {
  change((p) => {
    const w = { cities: citiesOf().map((c) => ({ ...c })), units: unitsOf() };
    fn(w);
    p.weather = w;
  }, { structure: true });
  if (state.page > citiesOf().length - 1) state.page = citiesOf().length - 1, emit('select');
}
// City `from` to place `to` (the order of the pages): the page on show follows its city
export function moveCity(from, to) {
  const n = citiesOf().length;
  to = Math.max(0, Math.min(n - 1, to));
  if (from === to || from < 0 || from >= n) return;
  const page = state.page;
  setWeather((w) => { const [c] = w.cities.splice(from, 1); w.cities.splice(to, 0, c); });
  const np = page === from ? to : from < page && to >= page ? page - 1 : from > page && to <= page ? page + 1 : page;
  if (np === state.page) return;
  state.page = np;
  if (state.sel?.type === 'screen' && state.sel.page !== undefined) state.sel = { ...state.sel, page: np };
  emit('select');
}
// What the cities' weather is (studio.py /api/weather — cached there for 10 minutes): asked again when the cities or the
// units change, and every 10 minutes; emit('weather') when it comes
export const weather = { key: '', data: null, at: 0 };
const weatherKey = () => JSON.stringify([unitsOf(), citiesOf().map((c) => [c.lat, c.lon])]);
export async function refreshWeather(force = false) {
  const key = weatherKey();
  if (!force && key === weather.key && Date.now() - weather.at < 600000) return;
  weather.key = key, weather.at = Date.now();
  try {
    const r = await fetch('/api/weather', { method: 'POST', body: JSON.stringify({ cities: citiesOf(), units: unitsOf() }) });
    const d = await r.json();
    if (key === weather.key) weather.data = r.ok ? d.data : null, emit('weather');
  } catch (e) {
    weather.at = 0;  // (no studio.py or no internet: again with the next change)
  }
}
// The numbers for city k, if they are the current cities' ({} — not yet)
export const weatherOf = (k) => (weather.key === weatherKey() && weather.data?.[k]) || {};

export const screens = () => state.project.screens;
export const home = () => screens()[0];
export const screenById = (id) => screens().find((s) => s.id === id) || null;
export const activeScreen = () => screenById(state.screen) || home();
export const layers = () => activeScreen().layers;
export const screenOf = (layerId) => screens().find((s) => s.layers.some((l) => l.id === layerId)) || null;
export const layerById = (id) => {
  for (const s of screens()) for (const l of s.layers) if (l.id === id) return l;
  return null;
};
export const selected = () => (state.sel && state.sel.type === 'layer' ? layerById(state.sel.id) : null);
// The gallery: the screen a swipe up on the face opens (null — none)
export const mainScreen = () => {
  const s = screenById(state.project.main);
  return s && s !== home() ? s : null;
};

// A layer that can open a screen: on the watch face (any of its layers: halo_nav_face_open) or a card or the button
// of the gallery on any screen (halo_nav_open)
export const canLink = (l, s = screenOf(l.id)) => s === home() ? !KINDS[l.type].fixed : TAPS.has(l.type);
// Which screens it can open: not the face, not the gallery, not its own
export const targetsFor = (l) => {
  const s = screenOf(l.id);
  return screens().filter((q) => q !== home() && q !== mainScreen() && q !== s);
};
// Links: visible layers that open a screen — {layer, from, to, fx}; fx 'a16' — from the gallery's card or button
export function links() {
  const out = [];
  for (const s of screens()) {
    for (const l of s.layers) {
      const to = l.tap && screenById(l.tap.to);
      if (!to || l.hidden || !canLink(l, s) || !targetsFor(l).includes(to)) continue;
      out.push({ layer: l, from: s, to, fx: s === home() ? l.tap.fx : 'a16' });
    }
  }
  return out;
}
// How the board gets to screen t from the face: [] — it is the face; steps — {main: true} (a swipe up: the gallery)
// or {from, layer} (a tap on that layer of that screen: over the face, or onto the gallery's stack — up to 4 deep, as
// lib/Halo's); null — nothing opens it. A screen a face's layer opens — over the face, the shortest way
export function wayTo(t) {
  if (t === home()) return [];
  const all = links(), face = all.find((k) => k.from === home() && k.to === t);
  if (face) return [{ from: home(), layer: face.layer }];
  const m = mainScreen();
  if (!m) return null;
  let paths = [[m, [{ main: true }]]];
  for (let depth = 0; depth <= 4 && paths.length; depth++) {
    const next = [];
    for (const [s, steps] of paths) {
      if (s === t) return steps;
      for (const k of all) {
        if (k.from !== s || steps.some((q) => q.to === k.to) || k.to === m) continue;
        next.push([k.to, [...steps, { from: s, layer: k.layer, to: k.to }]]);
      }
    }
    paths = next;
  }
  return null;
}

// Screens one can get to: what a link opens, and the gallery
export const linkedScreens = () => [...new Set([...links().map((k) => k.to), ...(mainScreen() ? [mainScreen()] : [])])];

// A screen's list: its shown titles and cards, top to bottom (the order of its layers: gallery.h)
export const listed = (s) => s.layers.filter((l) => LISTED.has(l.type) && !l.hidden);

// The gallery's head: its first title stands above all its cards (HaloWatch's list — the title, then the cards):
// nothing goes above it. The lowest place of the list (0 — the top) item l — or a new one of this type — may take:
// the head itself — 0 (it stays there), a card or another title — under the head
export function listMin(s, l, type = l?.type || 'card') {
  const items = listed(s), others = items.filter((q) => q !== l);
  const head = others.findIndex((q) => q.type === 'title');
  if (type === 'title' && (head < 0 || (l && items.indexOf(l) >= 0 && items.indexOf(l) < items.indexOf(others[head])))) return 0;
  return head + 1;
}
// Place `to` of the list made one the item may take (the head: only the top)
export function listPlace(s, l, to, type = l?.type || 'card') {
  const others = listed(s).filter((q) => q !== l), min = listMin(s, l, type);
  return type === 'title' && min === 0 ? 0 : Math.max(min, Math.min(others.length, to));
}
// After l moved among s's layers (the Layers card's rows, where the list's order is the layers' order): the head kept —
// l a card (or another title) gone above it: right under it; l the head gone under a card: back to the top of the list
export function keepHead(s, l) {
  if (!LISTED.has(l.type)) return;
  const items = listed(s), head = items.find((q) => q.type === 'title');
  if (!head || items[0] === head) return;
  if (l === head) s.layers.splice(s.layers.indexOf(l), 1), s.layers.splice(s.layers.indexOf(items[0]), 0, l);
  else s.layers.splice(s.layers.indexOf(l), 1), s.layers.splice(s.layers.indexOf(head) + 1, 0, l);
}
// A gallery made before this (or by an old page) with a card above its title: the title back to the top
function headFirst(p) {
  let n = 0;
  for (const s of p.screens) {
    const items = s.layers.filter((l) => LISTED.has(l.type) && !l.hidden), head = items.find((q) => q.type === 'title');
    if (!head || items[0] === head) continue;
    s.layers.splice(s.layers.indexOf(head), 1), s.layers.splice(s.layers.indexOf(items[0]), 0, head), n++;
  }
  return n;
}

// Put layer l at place `to` of screen s's list (0 — the top): right before the item there now, or after the last
// one; at — where it lands in the layers when l is not in them yet (a new card): the same rule. Never above the head
function listSpot(s, l, to, type = l?.type || 'card') {
  const others = listed(s).filter((q) => q !== l);
  to = listPlace(s, l, to, type);
  if (to < others.length) return s.layers.indexOf(others[to]);
  return others.length ? s.layers.indexOf(others[others.length - 1]) + 1 : s.layers.length;
}
// Move a title or a card to place `to` of its list
export function moveInList(id, to) {
  const s = screenOf(id), l = layerById(id);
  if (!s || !l) return;
  to = listPlace(s, l, to);
  if (listed(s).indexOf(l) === to) return;
  change(() => {
    s.layers.splice(s.layers.indexOf(l), 1);
    s.layers.splice(listSpot(s, l, to), 0, l);
  }, { structure: true });
}
// Where a new title or card goes into the layers to stand at place `to` of the list
export const listIndex = (s, to, type = 'card') => listSpot(s, null, to, type);

function nextId(prefix, extra = null) {  // extra — a screen not in the project yet
  const all = extra ? [...screens(), extra] : screens();
  const used = new Set([...all.map((s) => s.id), ...all.flatMap((s) => s.layers.map((l) => l.id))]);
  let n = 1;
  while (used.has(prefix + n)) n++;
  return prefix + n;
}

export function newLayer(type, at, screen = activeScreen()) {
  const d = KINDS[type];
  const same = screen.layers.filter((l) => l.type === type).length;
  const layer = { id: nextId(type, screen), type, name: same ? `${d.name} ${same + 1}` : d.name, hidden: false, dx: 0, dy: 0, tap: null };
  for (const k of [...LOOK, ...TEXTS]) if (k in d) layer[k] = d[k];
  if (type === 'card') layer.text = layer.name;  // "Card", "Card 2", ... (its name follows its title)
  if (STEP_FAMILY.has(type)) {  // (the goal and the accent the other Steps layers have)
    const q = state.project?.screens.flatMap((s) => s.layers).find((o) => STEP_FAMILY.has(o.type));
    if (q) layer.goal = q.goal;
    const acc = q && q[STEP_ACCENT[q.type]];
    if (acc && STEP_ACCENT[type]) layer[STEP_ACCENT[type]] = acc;
  }
  if (at) Object.assign(layer, at);
  return layer;
}

// A new screen to the right of the others on the map: empty, HaloWatch's watch face or Weather screen (all their
// parts at their places) or a new gallery (GALLERY_SCREEN) — under the face, as it comes from below it on the board
export function newScreen(kind = 'empty', { card = true } = {}) {
  const right = Math.max(...screens().map((s) => s.x + TILE));
  const n = screens().length;
  const named = { face: 'Watch face', foursides: 'Four sides', weather: 'Weather', gallery: 'Gallery' }[kind];
  const name = named ? (screens().some((s) => s.name === named) ? `${named} ${n}` : named) : `Screen ${n}`;
  const s = { id: nextId(named ? kind : 'screen'), name, x: right + GAP, y: home().y, layers: [] };
  if (kind === 'gallery') {
    const below = Math.max(...screens().filter((q) => Math.abs(q.x - home().x) < TILE).map((q) => q.y + TILE));
    s.x = home().x, s.y = below + GAP;
  }
  const parts = ({ face: FACE_SCREEN, weather: WEATHER_SCREEN, gallery: GALLERY_SCREEN }[kind] || []).filter((t) => card || t !== 'card');
  for (const type of parts) s.layers.push(newLayer(type, null, s));
  if (kind === 'foursides') for (const [type, look] of FOUR_SIDES) s.layers.push(newLayer(type, look, s));
  return s;
}

// A gallery under a card or the button (Go to → + New gallery): a new gallery's parts (GALLERY_SCREEN: the title, a
// card, More, the top clock) with the card's title on top; on the map next to the screen it is opened from
export function newList(from) {
  const text = (from.text || from.name || '').trim() || 'Gallery';
  let name = text;
  for (let k = 2; screens().some((s) => s.name === name); k++) name = `${text} ${k}`;
  const [x, y] = freeSpot(screenOf(from.id) || home(), screens());
  const s = { id: nextId('screen'), name, x, y, layers: [] };
  for (const type of GALLERY_SCREEN) s.layers.push(newLayer(type, null, s));
  s.layers.find((l) => l.type === 'title').text = text;
  return s;
}

// ---------- Steps: one goal and one accent — the Steps pill (its ring), the Steps screen's chart and footer ----------
export const STEP_FAMILY = new Set(['steps', 'stnum', 'stcap', 'stfoot', 'stchart', 'stepring']);
export const STEP_ACCENT = { steps: 'color3', stchart: 'color', stfoot: 'color', stepring: 'color' };
export function stepsSet(prop) {  // prop: 'goal' | 'accent' → a setter (v, key) for the props' fields
  return (v, key) => change((p) => {
    for (const s of p.screens) for (const q of s.layers) {
      if (prop === 'goal' && STEP_FAMILY.has(q.type)) q.goal = v;
      if (prop === 'accent' && STEP_ACCENT[q.type]) q[STEP_ACCENT[q.type]] = v;
    }
  }, { key: 'steps-' + prop });
}
// The Steps screen as one layer (a day of 4 Nov 2026) → its parts, in its place (the goal kept)
function splitSteps(p) {
  let n = 0;
  for (const s of p.screens) {
    const i = s.layers.findIndex((l) => l.type === 'stepscreen');
    if (i < 0) continue;
    const goal = s.layers[i].goal || 10000, parts = STEPS_SCREEN.filter((t) => t !== 'arc' || !s.layers.some((l) => l.type === 'arc'));
    s.layers.splice(i, 1, ...parts.map((t) => Object.assign(newLayer(t, null, s), { goal })));
    n++;
  }
  return n;
}

// ---------- The gallery: what goes into a list goes into a gallery ----------
// A gallery: a screen with a list (titles, cards) or the button at its bottom edge, or the one a swipe up opens
export const isGallery = (s) => !!s && (s.id === state.project.main || s.layers.some((l) => LISTED.has(l.type) || l.type === 'edge'));
// The gallery for what is added: the screen being edited if it is one, else the swipe up's, else any (null — none)
export const galleryFor = (s) => (isGallery(s) ? s : isGallery(mainScreen()) ? mainScreen() : screens().find(isGallery) || null);
// ---------- Watch faces saved from the project (Watch faces → Add): their layers as they were (project.faces) ----------
export const savedFaces = () => state.project.faces || [];
export function saveFace(name) {
  const s = home(), id = 'sf' + Date.now().toString(36);
  change((p) => {
    p.faces = p.faces || [];
    p.faces.push({ id, name, layers: JSON.parse(JSON.stringify(s.layers)) });
    s.face = 'saved:' + id;  // (this very screen is that face: picking it again brings this one back, no copy)
  }, { structure: true });
  return id;
}
export function removeFace(id) {
  change((p) => {
    p.faces = (p.faces || []).filter((f) => f.id !== id);
    for (const s of p.screens) if (s.face === 'saved:' + id) delete s.face;
  }, { structure: true });
}
// A saved face's layers for screen s: fresh ids, links kept only where their screen is still there
export function faceLayers(f, s) {
  const out = [];
  for (const l of JSON.parse(JSON.stringify(f.layers))) {
    l.id = nextId(l.type, { ...s, layers: [...s.layers, ...out] });
    if (l.tap && !screenById(l.tap.to)) l.tap = null;
    out.push(l);
  }
  return out;
}

// HaloWatch's Settings sections (settings.h): a card in the gallery, as HaloWatch's Settings rows, and its screen
export const SECTIONS = {
  // (Display's card is Brightness, in Gallery › General › Display; Battery's — in Gallery › General: SECTION_PATH)
  display: { text: 'Brightness', subtitle: 'screen level', icon: 'tv', color: '#A8C7FA', tile: 'Display' },
  battery: { text: 'Battery', subtitle: 'level and time left', icon: 'bolt', color: '#A4F9B5' },
  music: { text: 'Music', subtitle: 'piano classics', icon: 'music', color: '#F6B5EA' },
  recorder: { text: 'Recorder', subtitle: 'record and play back', icon: 'mic', color: '#F28B82' },
  snkdots: { text: 'Dot Snake', subtitle: 'LED matrix', icon: 'gamepad', color: '#FFF8EE' },
  snkspace: { text: 'Comet', subtitle: 'snake in space', icon: 'gamepad', color: '#8ECBFF' },
  snkiso: { text: 'Voxel Snake', subtitle: 'isometric 3D', icon: 'gamepad', color: '#B8FF6A' },
  stepscreen: { text: 'Steps', subtitle: 'today and this week', icon: 'run', color: '#FF8A80', tile: 'Steps',
    sub: 'The Steps screen: today by the hour and this week, two pages' },
  // HaloWatch's Weather screen as an app: its card as HaloWatch's gallery has it (Weather / now and hourly, the sun)
  wxscreen: { text: 'Weather', subtitle: 'now and hourly', icon: 'sun', color: '#FFB030', tile: 'Weather',
    sub: 'HaloWatch\'s Weather screen: now and hourly, a page a city' },
};
// What a section's screen is made of: the Steps and Weather screens — their parts; the others — the one layer
export const sectionParts = (kind) => ({ stepscreen: STEPS_SCREEN, wxscreen: WEATHER_SCREEN }[kind] || [kind]);
// Settings as on a phone: Battery and Display go into General — the gallery behind the card "General" (all settings);
// Display one deeper: General › Display › Brightness (its slider's screen). The looks — as the user made them by hand
// (5 Nov 2026: Project 1)
export const SECTION_PATH = { battery: ['General'], display: ['General', 'Display'] };
const SUB_GALLERIES = {
  General: { text: 'General', subtitle: 'all settings', icon: 'gear', color: '#A8C7FA' },
  Display: { text: 'Display', subtitle: 'brightness', icon: 'tv', color: '#A4A7F9' },
};
export const titleOf = (s) => s?.layers.find((l) => l.type === 'title')?.text;
// The gallery that gallery g's card `name` opens (General, Display); none — it is made: its title, the top clock, More
// (no starter card: what goes there brings its own), its card at the end of g's list. key — one undo step for a click
export function subGallery(g, name, key = null) {
  const found = g.layers.filter((l) => l.type === 'card' && l.tap).map((l) => screenById(l.tap.to))
    .find((q) => isGallery(q) && titleOf(q) === name);
  if (found) return found;
  if (screens().length >= MAX_SCREENS) return null;
  let s = null, nm = name;
  for (let k = 2; screens().some((q) => q.name === nm); k++) nm = `${name} ${k}`;
  change((p) => {
    const [x, y] = freeSpot(g, p.screens);
    s = { id: nextId('screen'), name: nm, x, y, layers: [] };
    for (const type of GALLERY_SCREEN.filter((t) => t !== 'card')) s.layers.push(newLayer(type, null, s));
    s.layers.find((l) => l.type === 'title').text = name;
    p.screens.push(s);
    const card = newLayer('card', null, g);
    Object.assign(card, { name, ...SUB_GALLERIES[name] });
    card.tap = { to: s.id, fx: 'a16' };
    g.layers.splice(listIndex(g, listed(g).length), 0, card);
  }, { structure: true, key });
  return s;
}

// The Games gallery: the card "Games" in gallery g and a gallery behind it (its title "Games", the top clock, More —
// no starter card: the games' cards go there); the one there is, if any
export function gamesGallery(g, key = null) {
  const found = screens().find((s) => isGallery(s) && s.layers.some((l) => l.type === 'title' && l.text === 'Games'));
  if (found) return found;
  if (screens().length >= MAX_SCREENS) return null;
  let s = null;
  let name = 'Games';
  for (let k = 2; screens().some((q) => q.name === name); k++) name = `Games ${k}`;
  change((p) => {
    const [x, y] = freeSpot(g, p.screens);
    s = { id: nextId('screen'), name, x, y, layers: [] };
    for (const type of GALLERY_SCREEN.filter((t) => t !== 'card')) s.layers.push(newLayer(type, null, s));
    s.layers.find((l) => l.type === 'title').text = 'Games';
    p.screens.push(s);
    const card = newLayer('card', null, g);
    Object.assign(card, { name: 'Games', text: 'Games', subtitle: 'self-playing snakes', icon: 'gamepad', color: '#C3F584' });
    card.tap = { to: s.id, fx: 'a16' };
    g.layers.splice(listIndex(g, listed(g).length), 0, card);
  }, { structure: true, key });
  return s;
}
export function addSection(g, kind, key = null) {
  const d = SECTIONS[kind];
  if (screens().length >= MAX_SCREENS) return null;
  const card = newLayer('card', null, g);
  Object.assign(card, { name: d.text, text: d.text, subtitle: d.subtitle, icon: d.icon, color: d.color });
  let name = d.text;
  for (let k = 2; screens().some((s) => s.name === name); k++) name = `${d.text} ${k}`;
  change((p) => {
    const [x, y] = freeSpot(g, p.screens);
    const scr = { id: nextId('screen'), name, x, y, layers: [] };
    for (const type of sectionParts(kind)) scr.layers.push(newLayer(type, null, scr));
    p.screens.push(scr);
    card.tap = { to: scr.id, fx: 'a16' };
    // (Weather: as on HaloWatch, the face's first weather pill that opens nothing yet opens it too)
    const pill = kind === 'wxscreen' && p.screens[0].layers.find((l) => l.type === 'weather' && !l.tap);
    if (pill) pill.tap = { to: scr.id, fx: 'fade' };
    g.layers.splice(listIndex(g, listed(g).length), 0, card);
  }, { structure: true, key });
  return card;
}

// ---------- Undo / redo: snapshots of the whole project ----------
const undo = [], redo = [];
let lastKey = null, lastAt = 0;

function snapshot() {
  return JSON.stringify({ project: state.project, sel: state.sel, screen: state.screen });
}

// Run fn on the project. history: false — no undo step (the middle of a drag); key — repeated changes with the same
// key within 1.5 s are one undo step (a slider being dragged, a number being typed)
export function change(fn, { history = true, key = null, structure = false } = {}) {
  if (history) {
    const now = performance.now();
    if (!(key && key === lastKey && now - lastAt < 1500)) {
      undo.push(snapshot());
      if (undo.length > 200) undo.shift();
      redo.length = 0;
    }
    lastKey = key, lastAt = now;
  }
  fn(state.project);
  emit('project');
  if (structure) emit('structure');
  save();
}

// The empty screens the editor used to make by itself for a card or More without a link (until 4 Nov 2026): named
// after it, nothing on them, opened by it alone — gone, the link with them. A card leads nowhere until its screen is
// made (Go to → + New screen); the board shows HaloWatch's stub with its title
// Layers of a kind there is no more (a game taken out of the editor) — dropped
function dropUnknown(p) {
  let n = 0;
  for (const s of p.screens) {
    const keep = s.layers.filter((l) => KINDS[l.type]);
    n += s.layers.length - keep.length, s.layers = keep;
  }
  return n;
}
function dropPlaceholders(p) {
  let n = 0;
  for (const s of p.screens.slice(1)) {
    if (s.layers.length || s.id === p.main) continue;
    const from = p.screens.flatMap((q) => q.layers).filter((l) => l.tap && l.tap.to === s.id);
    if (from.length !== 1 || !TAPS.has(from[0].type) || ((from[0].text || '').trim() || from[0].name) !== s.name) continue;
    from[0].tap = null;
    p.screens.splice(p.screens.indexOf(s), 1);
    n++;
  }
  return n;
}

// A free place for a new screen next to screen `from`: to its right, a row at a time — level with it, then below and
// above; the next column further right
function freeSpot(from, all) {
  const m = GAP / 2, busy = (x, y) => all.some((q) => x < q.x + span(q) + m && x + TILE + m > q.x && y < q.y + TILE + m && y + TILE + m > q.y);
  const step = TILE + GAP;
  for (let c = 0; c < 12; c++) {
    for (const r of [0, 1, -1, 2, -2, 3]) {
      const x = from.x + span(from) + GAP + c * step, y = from.y + r * step;
      if (!busy(x, y)) return [x, y];
    }
  }
  return [from.x + span(from) + GAP, from.y];
}

// The start of a drag: one undo step for all of it
export function beginGesture() {
  undo.push(snapshot());
  redo.length = 0;
  lastKey = null;
}

function fixSelection() {
  if (!screenById(state.screen)) state.screen = home().id;
  const s = state.sel;
  if (!s) return;
  const ok = s.type === 'layer' || s.type === 'link' ? layerById(s.id) && (s.type !== 'link' || layerById(s.id).tap)
    : s.type === 'main' ? mainScreen() : s.type === 'fn' ? FUNCTIONS[s.id] : screenById(s.id);
  if (!ok) state.sel = null;
}

function restore(s) {
  const o = JSON.parse(s);
  state.project = o.project, state.sel = o.sel, state.screen = o.screen;
  fixSelection();
  emit('project'), emit('structure'), emit('select');
  save();
}
export function doUndo() {
  if (!undo.length) return;
  redo.push(snapshot());
  restore(undo.pop());
  lastKey = null;
}
export function doRedo() {
  if (!redo.length) return;
  undo.push(snapshot());
  restore(redo.pop());
  lastKey = null;
}
export const canUndo = () => undo.length > 0;
export const canRedo = () => redo.length > 0;

// Select something; a layer or a screen also makes its screen the active one
export function select(sel) {
  const same = JSON.stringify(sel) === JSON.stringify(state.sel);
  state.sel = sel;
  let scr = null;
  if (sel && sel.type === 'layer') scr = screenOf(sel.id);
  if (sel && (sel.type === 'screen' || sel.type === 'back' || sel.type === 'main')) scr = screenById(sel.id);
  const page = sel && sel.type === 'screen' && sel.page !== undefined ? sel.page : null;  // (a page of it: {…, page})
  if (scr && scr.id !== state.screen) state.screen = scr.id, state.page = page ?? 0, emit('screen');
  else if (page !== null) state.page = page;
  if (!same) emit('select');
}

// ---------- Saving: projects/<state.pid>.json through studio.py, half a second after the last change ----------
// Every save carries the revision it was made from (studio.py: save_project). Another window saved in between — this
// one takes that project (its own last change is lost, said in the status line); and every 2 s a window that has
// nothing to save checks for a newer revision and takes it: two windows show the same project.
let saveTimer = 0, saving = false;
export const sync = { note: '' };

function save() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveNow, 500);
}

async function saveNow() {
  clearTimeout(saveTimer);
  saveTimer = 0, saving = true;
  try {
    const r = await fetch(`/api/project?p=${state.pid}`, { method: 'PUT', body: JSON.stringify({ ...state.project, rev: state.rev }) });
    const j = await r.json();
    if (r.ok) state.rev = j.rev, emit('saved');
    else if (r.status === 409) adopt(j.project, 'The project was changed in another window — showing that version');
  } catch (e) { /* studio.py is not running: it is saved with the next change */ }
  saving = false;
}

function adopt(project, note) {
  state.rev = project.rev;
  delete project.rev;
  state.project = project;
  if (dropUnknown(project) + dropPlaceholders(project) + splitSteps(project) + headFirst(project)) save();  // (an old page in another window may still make them)
  fixSelection();
  sync.note = note;
  emit('project'), emit('structure'), emit('select');
}

async function poll() {
  if (saveTimer || saving) return;
  try {
    const pid = state.pid;
    const { rev } = await (await fetch(`/api/project/rev?p=${pid}`, { cache: 'no-store' })).json();
    if (rev === undefined || rev === state.rev || saveTimer || saving || pid !== state.pid) return;
    const p = await (await fetch(`/api/project?p=${pid}`, { cache: 'no-store' })).json();
    if (!saveTimer && !saving && pid === state.pid) adopt(p, 'Updated from another window');
  } catch (e) { /* studio.py is not running */ }
}

// The project pid (false — there is no such project)
async function fetchProject(pid) {
  const r = await fetch(`/api/project?p=${pid}`, { cache: 'no-store' });
  if (!r.ok) return false;
  const p = await r.json();
  state.pid = pid, state.rev = p.rev;
  delete p.rev;
  state.project = p;
  for (const s of p.screens) {
    s.x ??= 0, s.y ??= 0;
    for (const l of s.layers) {
      l.tap ??= null;
    }
  }
  state.screen = p.screens[0].id;
  if (dropUnknown(p) + dropPlaceholders(p) + splitSteps(p) + headFirst(p)) save();
  try { localStorage.setItem('popka.project', pid); } catch (e) { /* not kept: the newest one next time */ }
  return true;
}

// At the start: the project open last time in this browser, or the newest one (none — a new one)
export async function load() {
  let pid = null;
  try { pid = localStorage.getItem('popka.project'); } catch (e) { /* no storage */ }
  if (!pid || !(await fetchProject(pid))) {
    const { projects } = await (await fetch('/api/projects', { cache: 'no-store' })).json();
    pid = projects[0]?.id || (await (await fetch('/api/projects', { method: 'POST', body: JSON.stringify({ board: 'amoled175' }) })).json()).id;
    await fetchProject(pid);
  }
  setInterval(poll, 2000);
}

// Another project instead of this one (the start window): this one's last change saved first; no undo across them
export async function openProject(pid) {
  if (saveTimer) await saveNow();
  if (!(await fetchProject(pid))) return false;
  undo.length = redo.length = 0, lastKey = null;
  state.sel = null;
  emit('project'), emit('structure'), emit('select'), emit('screen');
  return true;
}

// The open project's name (the start window renames it here, so the next save keeps it)
export const projectName = () => state.project.name || state.pid.replace(/-/g, ' ').replace(/^./, (c) => c.toUpperCase());

export async function resetToDefault() {
  const r = await fetch('/api/default', { cache: 'no-store' });
  const fresh = await r.json();
  change((p) => { p.screens = fresh.screens; }, { structure: true });
  state.screen = home().id;
  select(null);
}
