// main.js — PopkaStudio as a Mac app (Electron): the editor in its own window, no browser around it.
// The page is studio.py's, as in a browser (http://localhost:8466): the app starts studio.py when nothing answers there
// and stops it when the app quits (a studio.py started by hand is left running). No title bar: the window's three
// buttons sit in the editor's top left group, before the name (index.html: html.app — the page knows it is in the app
// by "PopkaStudioApp" in its user agent); the strip along the top moves the window.
//   npm start      — run it from here (app/)
//   ./build.sh     — PopkaStudio.app with its icon, into /Applications (README.md: "Приложение для Mac")
const { app, BrowserWindow, Menu, shell, screen } = require('electron');
const path = require('path');
const fs = require('fs');
const http = require('http');
const { spawn } = require('child_process');

const PORT = 8466;
const PAGE = `http://localhost:${PORT}/`;
// Where studio.py is: build.sh writes home.json into the packed app (it is copied away from the repository);
// run from app/ — the folder above
function home() {
  try {
    return JSON.parse(fs.readFileSync(path.join(__dirname, 'home.json'), 'utf8'));
  } catch {
    const studio = path.resolve(__dirname, '..'), venv = path.resolve(studio, '..', '.venv', 'bin', 'python');
    return { studio, python: fs.existsSync(venv) ? venv : '/opt/homebrew/opt/python@3.14/bin/python3.14' };
  }
}

// The window buttons' place: in the top left group (studio.css: .app .dock-l) — its middle at y 29
const BUTTONS = { x: 24, y: 22 };

let win = null, server = null, log = [], quitting = false;
const LOGS = path.join(app.getPath('logs'));
const logFile = path.join(LOGS, 'studio.log');

// ---------- studio.py ----------
// Is studio.py there: its page answers
function answers() {
  return new Promise((done) => {
    const rq = http.get(PAGE, { timeout: 1500 }, (r) => { r.resume(); done(r.statusCode === 200); });
    rq.on('error', () => done(false));
    rq.on('timeout', () => { rq.destroy(); done(false); });
  });
}

// Start it: from a Finder launch the PATH is bare — studio.py needs arduino-cli (Load) and em++ (the page's engine)
function startServer() {
  const { studio, python } = home();
  fs.mkdirSync(LOGS, { recursive: true });
  const out = fs.createWriteStream(logFile, { flags: 'w' });
  const env = { ...process.env, PYTHONUNBUFFERED: '1' };
  env.PATH = ['/opt/homebrew/bin', '/opt/homebrew/sbin', '/usr/local/bin', env.PATH || '/usr/bin:/bin:/usr/sbin:/sbin'].join(':');
  // Its own process group: quitting stops studio.py and what it runs (a Load's arduino-cli)
  server = spawn(python, ['studio.py'], { cwd: studio, env, detached: true });
  const take = (b) => {
    out.write(b);
    log.push(...b.toString().split('\n').filter(Boolean));
    log = log.slice(-60);
    tellLoading();
  };
  server.stdout.on('data', take);
  server.stderr.on('data', take);
  server.on('error', (e) => take(Buffer.from(`Could not start ${python}: ${e.message}\n`)));
  server.on('exit', (code) => {
    server = null;
    if (!quitting) failed(`studio.py stopped (code ${code})`);
  });
}

function stopServer() {
  if (!server) return;
  try { process.kill(-server.pid, 'SIGTERM'); } catch { /* gone */ }
  server = null;
}

// ---------- The window ----------
const stateFile = path.join(app.getPath('userData'), 'window.json');
function savedBounds() {
  try {
    const b = JSON.parse(fs.readFileSync(stateFile, 'utf8'));
    const area = screen.getDisplayMatching(b).workArea;
    const inside = b.x >= area.x - 40 && b.y >= area.y - 40 && b.x + 200 < area.x + area.width && b.y + 100 < area.y + area.height;
    return inside ? b : null;
  } catch {
    return null;
  }
}

// The loading page (loading.html): what studio.py says while it starts, its log when it fails
function tellLoading(error) {
  if (!win || win.webContents.getURL().startsWith('http')) return;
  const js = `window.show && show(${JSON.stringify(log.slice(-14))}, ${JSON.stringify(error || '')})`;
  win.webContents.executeJavaScript(js).catch(() => {});
}
function failed(why) {
  if (win && win.webContents.getURL().startsWith('http')) win.loadFile(path.join(__dirname, 'loading.html')).then(() => tellLoading(why));
  else tellLoading(why);
}

function makeWindow() {
  const b = savedBounds() || { width: 1440, height: 900 };
  win = new BrowserWindow({
    ...b,
    minWidth: 960,
    minHeight: 640,
    title: 'PopkaStudio',
    backgroundColor: '#131416',
    titleBarStyle: 'hidden',
    trafficLightPosition: BUTTONS,
    show: false,
    webPreferences: { contextIsolation: true, sandbox: true, spellcheck: false },
  });
  win.webContents.setUserAgent(win.webContents.getUserAgent() + ' PopkaStudioApp');
  win.once('ready-to-show', () => win.show());
  if (b.maximized) win.maximize();
  // Full screen: no buttons in the corner — the group gives their place back
  const full = (on) => win.webContents.executeJavaScript(`document.documentElement.classList.toggle('fullscreen', ${on})`).catch(() => {});
  win.on('enter-full-screen', () => full(true));
  win.on('leave-full-screen', () => full(false));
  win.webContents.on('did-finish-load', () => full(win.isFullScreen()));
  win.on('close', () => {
    try {
      fs.mkdirSync(path.dirname(stateFile), { recursive: true });
      fs.writeFileSync(stateFile, JSON.stringify({ ...win.getNormalBounds(), maximized: win.isMaximized() }));
    } catch { /* not kept */ }
  });
  win.on('closed', () => { win = null; });
  // Links out of the editor — in the browser; the window stays the editor's
  win.webContents.setWindowOpenHandler(({ url }) => { shell.openExternal(url); return { action: 'deny' }; });
  win.webContents.on('will-navigate', (e, url) => {
    if (!url.startsWith(PAGE) && !url.startsWith('file:')) e.preventDefault(), shell.openExternal(url);
  });
  // studio.py restarted (or a reload while it was away): the page comes back by itself
  win.webContents.on('did-fail-load', (e, code, desc, url) => {
    if (code !== -3 && url.startsWith('http')) win.loadFile(path.join(__dirname, 'loading.html')).then(open);
  });
  win.loadFile(path.join(__dirname, 'loading.html'));
}

// The editor, once studio.py answers (the first start may build the page's engine: up to a few minutes)
async function open() {
  let started = false;
  while (!quitting && win) {
    if (await answers()) return win.loadURL(PAGE);
    if (server || started) {
      if (!server) return;  // (it stopped: the loading page shows why)
    } else {
      startServer(), started = true;
    }
    await new Promise((r) => setTimeout(r, 300));
  }
}

function menu() {
  Menu.setApplicationMenu(Menu.buildFromTemplate([
    { role: 'appMenu' },
    { role: 'editMenu' },  // (⌘Z, ⌘C, ⌘V in fields; the editor's own ⌘Z comes first — it takes the key)
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'forceReload' },
        { role: 'toggleDevTools' },
        { type: 'separator' },
        { role: 'togglefullscreen' },
      ],
    },
    { role: 'windowMenu' },
    {
      role: 'help',
      submenu: [
        { label: 'Open in the Browser', click: () => shell.openExternal(PAGE) },
        { label: 'studio.py Log', click: () => shell.openPath(logFile) },
      ],
    },
  ]));
}

// ---------- The app ----------
if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on('second-instance', () => {
    if (!win) return makeWindow(), open();
    if (win.isMinimized()) win.restore();
    win.focus();
  });
  app.whenReady().then(() => {
    const icon = path.join(__dirname, 'icon.png');
    if (!app.isPackaged && app.dock && fs.existsSync(icon)) app.dock.setIcon(icon);
    menu();
    makeWindow();
    open();
  });
  app.on('activate', () => { if (!win) makeWindow(), open(); });
  app.on('window-all-closed', () => app.quit());  // (one window: closed — the app and its studio.py are done)
  app.on('before-quit', () => { quitting = true; });
  app.on('will-quit', stopServer);
  process.on('exit', stopServer);
}
