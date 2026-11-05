// about.js — the About window: the heart at the top right, left of the board. Who made PopkaStudio and with whom, the
// links, the license, a contact; Credits — the second step (what is someone else's, and its license). Over the editor,
// blurred behind it, as the start window (the same look: .start, .start-win, a track of steps that slides).
const $ = (id) => document.getElementById(id);

export function initAbout() {
  const box = $('about'), win = $('about-win');
  const shown = () => box.classList.contains('on');
  const open = () => {
    win.classList.add('instant'), win.dataset.step = '1';
    requestAnimationFrame(() => win.classList.remove('instant'));
    box.classList.add('on'), box.setAttribute('aria-hidden', 'false');
    $('about-ok').focus({ preventScroll: true });
  };
  const close = () => { box.classList.remove('on'), box.setAttribute('aria-hidden', 'true'); };
  $('about-btn').addEventListener('click', open);
  for (const id of ['about-x', 'about-ok', 'about-ok2']) $(id).addEventListener('click', close);
  $('about-credits').addEventListener('click', () => { win.dataset.step = '2'; });
  $('about-back').addEventListener('click', () => { win.dataset.step = '1'; });
  box.addEventListener('pointerdown', (e) => { if (e.target === box) close(); });  // (a click beside the window)
  addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && shown()) e.stopPropagation(), close();
  }, true);
  fetch('/api/about').then((r) => r.json()).then((a) => { $('about-ver').textContent = 'v' + a.version; }).catch(() => {});
}
