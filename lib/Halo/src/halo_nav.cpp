// Слой интерфейса Halo: навигация как в Wear OS 6 — главный экран и экран, открытый поверх него.
//
// Числа — как в AmoledWear (wear_nav.cpp, docs/research/notes/motion_tokens.md):
//   открыть (Android 16, PredictiveBackNavHost): новый — сдвиг с W/2 (300/0,8), масштаб с 0,8 (500/1), проявление
//   (1500/1); главный — масштаб до 0,85 (150/1), сдвиг на −W/2 (200/0,8), гаснет до 0,6 (1400/1);
//   назад — свайп от левого края (30 dp): палец ведёт прогресс; уходящий экран сверху уезжает на W и уменьшается до
//   0,8, главный выходит из-под него с −W/2, с 0,8 и с 0,5 яркости; отпустили дальше половины или бросок 800 dp/с —
//   доводка пружиной 1500/1, иначе обратно.
//
// В AmoledWear на время перехода оба экрана — снимки в PSRAM (сдвиг и масштаб готовой картинки, ~13 мс кадра, и
// переход начинается через 3–5 кадров, пока новый экран рисуется в снимок). Здесь снимков нет: каждый экран ставит
// свои элементы через «вид» (HaloView: сдвиг, масштаб от середины, яркость) и рисуется вживую — переход начинается в
// том же кадре, а на шину, как всегда у Halo, уходит только изменившееся. Как у снимка:
//   * главный, уменьшенный, остаётся круглым — после него всё вне его круга закрашивается чёрным (halo_clip_circle);
//   * под новым — чёрный круг его размера: прозрачный, пока новый проявляется, потом — непрозрачный.
//
// Циферблат под всем (halo_nav_face): главный экран над ним, переход между ними — как Fade у экрана поверх циферблата
// (одна пружина p: 0 — циферблат, 1 — главный). Свайп вверх по циферблату: путь p = путь пальца / 280, циферблат уходит
// вверх на 70·p, уменьшается к 0,92 и гаснет за первую половину пути; главный во второй приходит снизу (70·(1 − p) → 0),
// уменьшаясь с 1,08 до 1, и проявляется. Свайп вниз от верхнего края главного — то же в зеркале: главный уходит вниз,
// циферблат приходит сверху. Одновременно их не видно никогда, а целиком главный не едет: раньше он выезжал на всю
// высоту экрана, и то, что было за его краем (заголовок прокрученной до конца ленты), проезжало поверх циферблата.
// С открытого на главном экрана — свой жест (HaloScreen::home): вниз от верхнего края — так же, вверх от нижнего — в
// зеркале (экран уходит вверх, циферблат приходит снизу, точки страниц тянутся — как назад с погоды поверх циферблата).
// Пришли на циферблат — открытый экран закрыт, главный — с начала (его enter).
//
// Экран прямо поверх циферблата (halo_nav_face_open): одна пружина q (0 — циферблат, 1 — экран) и вид каждого из двух
// по q — свой у каждого перехода (HALO_FX_*). Жест назад снизу прокручивает тот же переход пальцем: q = 1 − путь/280,
// и содержимое ещё и чуть идёт за пальцем (четверть пути — пружиной обратно, когда отпустили). Фон у обоих чёрный,
// поэтому ни круга под экраном, ни обрезки, ни перерисовки всего экрана не нужно: элементы сами сообщают, где
// изменились, — на шину уходят только они.
//
// Круг — только у круглого экрана (HALO_ROUND). На прямоугольном страница в переходе — прямоугольник: под открытым —
// чёрная плашка его размера со скруглёнными углами, а уменьшенный экран рисуется только внутри своей рамки.
#include "halo_internal.h"

#pragma GCC optimize("O3", "fast-math")

#define CX (HALO_W / 2.0f)
#define CY (HALO_H / 2.0f)

// Чёрная фигура размера страницы (под страницей с видом v)
#ifdef HALO_ROUND
#define COVER_KIND HI_CIRCLE
#define COVER_PUT(c, v) ((c).x = CX + (v).x, (c).y = CY + (v).y, (c).r = CX * (v).s)
#else
#define COVER_KIND HI_RRECT
#define PAGE_RADIUS 28.0f
#define COVER_PUT(c, v) \
  ((c).x = CX + (v).x, (c).y = CY + (v).y, (c).w = HALO_W * (v).s, (c).h = HALO_H * (v).s, (c).r = PAGE_RADIUS * (v).s)
#endif

enum { V_HOME, V_TOP, V_OPEN, V_BACK };
// Стопка экранов: scr[0] — главный, scr[1..depth] — открытые поверх него (из главного, из открытого — до NAV_MAX).
// Переход Android 16 и свайп назад — всегда между двумя верхними: scr[depth − 1] («нижний», вид v_home) и scr[depth]
// (вид v_top); назад — на один уровень. Остальные в это время спрятаны
#define NAV_MAX 4
static HaloScreen scr[NAV_MAX + 1];
static int depth;
static int view = V_HOME;
static bool back_drag, clip, settle_frame;
static volatile bool cmd_back;
#ifndef HALO_ROUND
// Экран вырастает из элемента (halo_nav_open_from): откуда (x0, y0, x1, y1), скругление и цвет; где окно сейчас
static bool grow_on;
static HaloSpring t_grow;
static HaloSpringK K_GROW;
static float grow_from[4], grow_r, grow_box[4];
static uint32_t grow_c;
// Открыть и назад — Material shared axis X (PopkaStudio, web/mockups/rect-transitions.html, вариант 6): по времени
// и кривой Material standard, без пружин — короткое движение без хвоста. ax_t — время пути 0..1, ax_e0 — откуда
// доигрывается назад после пальца
#define AX_OPEN_S 0.30f
#define AX_CLOSE_S 0.26f
#define AX_RETURN_S 0.16f
#define AX_SHIFT 30.0f
static HaloEase E_AXIS;
static float ax_t, ax_e0;
// Циферблат ↔ главный (sheet) — та же ось, только вертикальная, чуть дольше и на 40 px: это смена главного экрана.
// После пальца или по команде — по времени от того места, где отпустили (sh_x0), к цели sh_tgt
#define SH_S 0.38f
#define SH_SHIFT 40.0f
static float sh_t, sh_x0, sh_tgt = 1;
static bool sh_restart;
#endif
static float back_zone = 60;
static HaloSpring t_slide, t_scale, t_fade, t_oscale, t_oslide, t_ofade, back_p;
static HaloSpringK K_IN_SLIDE, K_IN_SCALE, K_IN_FADE, K_OUT_SCALE, K_OUT_SLIDE, K_OUT_FADE, K_DONE;
static struct {
  float nx, ns, na, ox, os, oa;  // открытый (n) и главный (o): сдвиг, масштаб, яркость
} T;
static HaloView v_home, v_top;
static HaloItem cover;  // чёрный круг под открытым экраном (в переходе)
static bool hide_under; // открытый экран в этом кадре непрозрачен — главный под ним не рисуем
static int nav_owner;   // «хозяин» жеста назад (адрес для halo_finger_claim)
static bool nav_moving; // в прошлом кадре шёл переход (halo_heavy — только в первом кадре движения)

// Циферблат под всем
static HaloScreen face;
static bool has_face, sheet_drag;
static bool has_main;  // был halo_nav_begin: с циферблата есть куда уйти свайпом вверх (без него циферблат — один)
static bool sheet_in;  // переход к главному (уходит циферблат); false — к циферблату
static bool main_seen;  // главный был виден с тех пор, как пришли на циферблат (по приходе — закрыть и в начало)
static float sheet_dir = 1;  // 1 — к главному вверх (к циферблату — вниз), −1 — к циферблату вверх (жест снизу)
static HaloSpring sheet = {1, 0, 1};  // 0 — циферблат, 1 — главный
static HaloSpring sheet_pull;  // жест на циферблат снизу: насколько тянут, px (точки страниц)
static float sheet_g0, face_zone = 90;
// Где жест забрал палец (y): путь перехода — от этой точки, а не от касания. Порог «это жест» (10 px) и то, что тач
// отдаёт разом, когда палец трогается (чип держит точку, пока палец не сдвинется, — у края и полсекунды, потом
// отпускает 11–13 px за отчёт), уходят в порог: экран трогается с места плавно, а не прыжком на эти пиксели
static float sheet_y0, over_y0;
static volatile int cmd_face = -1;
static int sheet_owner;
static HaloView v_face;

// Экран поверх циферблата
#define OVER_ZONE 70.0f     // жест назад — свайп вверх, начатый ниже H − 70
#define OVER_TRAVEL 280.0f  // столько px пальцем вверх — путь до циферблата целиком
const char *const HALO_FX_NAME[HALO_FX_COUNT] = {"Zoom", "Fade", "Slide"};
static HaloScreen over;
static bool over_on, over_drag, over_closing;
static uint8_t over_fx;
static float over_x, over_y, over_s;  // откуда открыли: центр элемента и его размер относительно экрана
static HaloSpring over_q, over_pull;  // 0 — циферблат, 1 — экран; насколько тянут снизу, px
static HaloSpringK K_OVER_IN, K_OVER_OUT, K_PULL;
static int over_owner;
static HaloView v_over;
static HaloItem over_cover;  // Zoom: чёрная «карточка» под экраном — циферблат за ней не просвечивает

// Always On поверх всего (halo_nav_ambient)
static float amb_k;
static HaloView amb_v;
static bool amb_face, amb_morph;  // в конце — циферблат; он и был на экране (иначе — fade through)

void halo_nav_ambient(float k, const HaloView &v, bool face) {
  k = halo_clamp01(k);
  if (amb_k == 0 && k > 0) amb_face = face, amb_morph = halo_nav_on_face();  // как идти — с места и до конца
  if ((amb_k == 0 || amb_k == 1) && k != amb_k) halo_heavy();  // тронулись с места: сразу вслед за проходом
  amb_k = k, amb_v = v;
}

// Вид экрана v — ещё и через вид o поверх него
static void view_under(HaloView &v, const HaloView &o) {
  v.x = o.x + v.x * o.s, v.y = o.y + v.y * o.s, v.s *= o.s, v.a *= o.a;
}

void halo_view_commit(HaloItem &it, const HaloView &v) {
  it.x = halo_view_x(v, it.x), it.y = halo_view_y(v, it.y);
  it.scale *= v.s, it.alpha *= v.a;
  if (v.over && v.a < 0.999f) it.on_black = false, it.on = nullptr;  // под экраном не чёрный и не сплошной цвет
  halo_item_commit(it);
}

void halo_nav_begin(const HaloScreen &h) {
  if (has_main) {  // главный меняют: прежний и открытые на нём — спрятать, на циферблат
    for (int d = depth; d >= 0; d--)
      if (scr[d].layout) scr[d].layout({0, 0, 1, 0});
    depth = 0, view = V_HOME, back_drag = sheet_drag = main_seen = false;
    if (has_face) halo_spring_set(sheet, 0), halo_spring_set(sheet_pull, 0);
  }
  scr[0] = h, has_main = h.layout != nullptr;
  K_IN_SLIDE = halo_spring(halo_stiff(300), 0.8f);
  K_IN_SCALE = halo_spring(halo_stiff(500), 1);
  K_IN_FADE = halo_spring(halo_stiff(1500), 1);
  K_OUT_SCALE = halo_spring(halo_stiff(150), 1);
  K_OUT_SLIDE = halo_spring(halo_stiff(200), 0.8f);
  K_OUT_FADE = halo_spring(halo_stiff(1400), 1);
  K_DONE = halo_spring(halo_stiff(1500), 1);
#ifndef HALO_ROUND
  K_GROW = halo_spring(0.36f, 1);  // (как растущая плитка видео: PopkaStudio, video.h)
  halo_ease_make(E_AXIS, 0.2f, 0, 0, 1);  // Material standard
#endif
  cover.kind = COVER_KIND, cover.color = 0, cover.alpha = 0;
  halo_command("back", [](const char *) { cmd_back = true; });
}

// Открытый экран уже на месте для глаза: пружина сдвига ещё ~0,2 с доигрывает перелёт в 3–4 px (у круга — до 467 мс от
// нажатия, хотя к ~270 мс экран на вид стоит). Касания ждать её не должны (PopkaStudio, 11 ноября 2026: нажатие сразу
// после перехода пропадало) — с этого места палец уже экрану, а открыть следующий можно сразу (finish_open)
static bool open_late() {
#ifndef HALO_ROUND
  return grow_on ? t_grow.x > 0.95f : ax_t > 0.8f;
#else
  return t_fade.x > 0.99f && fabsf(1 - t_slide.x) * (HALO_W / 2) < 4 && (1 - t_scale.x) * 0.2f * (HALO_W / 2) < 1;
#endif
}
// Переход открытия — сразу в конец (оставшиеся пиксели незаметны: этот экран тут же сам уходит под новый или назад)
static void finish_open() {
  t_slide = t_scale = t_fade = t_oscale = t_oslide = t_ofade = {1, 0, 1};
#ifndef HALO_ROUND
  if (grow_on) halo_spring_set(t_grow, 1), grow_on = false;
  ax_t = 1;
#endif
  view = V_TOP;
}

void halo_nav_open(const HaloScreen &s) {
  if (view == V_OPEN && open_late()) finish_open();
  if ((view != V_HOME && view != V_TOP) || depth >= NAV_MAX) return;
  if (has_face && !sheet_drag && sheet.target == 1) halo_spring_set(sheet, 1);  // главный ещё доезжал — сразу на место
  HaloScreen &top = scr[++depth];
  top = s;
  if (top.enter) top.enter();
  const void *owner = halo_finger.owner;  // палец этого кадра — не ему
  halo_finger.owner = &nav_owner;
  if (top.update) top.update(0);
  halo_finger.owner = owner;
  t_slide = t_scale = t_fade = t_oscale = t_oslide = t_ofade = {0, 0, 1};
  view = V_OPEN;
#ifndef HALO_ROUND
  grow_on = false, ax_t = 0;
#endif
}

#ifndef HALO_ROUND
void halo_nav_open_from(const HaloScreen &s, float x, float y, float w, float h, float r, uint32_t color) {
  halo_nav_open(s);
  if (view != V_OPEN || w < 1 || h < 1) return;
  grow_on = true, grow_r = r, grow_c = color;
  grow_from[0] = x - w / 2, grow_from[1] = y - h / 2, grow_from[2] = x + w / 2, grow_from[3] = y + h / 2;
  halo_spring_set(t_grow, 0), t_grow.target = 1;
}
#endif

void halo_nav_back() {
  cmd_back = true;
}

void halo_nav_back_zone(float x) {
  back_zone = x;
}

bool halo_nav_at_home() {  // главный на месте или уже доезжает к нему (отпустили) и виден
  return view == V_HOME && (!has_face || (!sheet_drag && sheet.target == 1 && sheet.x > 0.5f));
}

int halo_nav_depth() {
  return depth;
}

bool halo_nav_over() {
  return over_on;
}

void halo_nav_face(const HaloScreen &f, float zone) {
  face = f, has_face = true, face_zone = zone;
  halo_spring_set(sheet, 0);
  K_OVER_IN = halo_spring(0.45f, 0.9f);  // открыть: ~0,45 с, почти без перелёта
  K_OVER_OUT = halo_spring(0.38f, 1);    // закрыть (со скоростью пальца)
  K_PULL = halo_spring(0.3f, 1);
  over_cover.kind = COVER_KIND, over_cover.color = 0, over_cover.alpha = 0;
  if (face.enter) face.enter();
  halo_command("face", [](const char *a) { cmd_face = strstr(a, "off") ? 1 : 0; });
  halo_command("back", [](const char *) { cmd_back = true; });  // и без главного: закрыть экран поверх циферблата
}

void halo_nav_to_face(bool f) {
  cmd_face = f ? 0 : 1;
}

bool halo_nav_on_face() {
  return has_face && !over_on && !sheet_drag && sheet.x == 0 && sheet.target == 0;
}

// Экран поверх циферблата закрыт: спрятать и забыть
static void over_forget() {
  if (over.layout) over.layout({0, 0, 1, 0});
  over = {}, over_on = false;
  halo_spring_set(over_q, 0), halo_spring_set(over_pull, 0);
}

void halo_nav_face_open(const HaloScreen &s, uint8_t fx, float x, float y, float w, float h) {
  // Прошлый ещё доезжает к закрытию (отпустили), а видно уже циферблат — закрытие доигрывается сразу, и новый
  // открывается. Раньше нажатие в эти ~0,65 с после «назад» пропадало: экран считался открытым, пока не успокоятся обе
  // его пружины, и плашка погоды не нажималась
  if (over_on && !over_drag && over_q.target < 0.5f && over_q.x < 0.5f) over_forget();
  if (!halo_nav_on_face()) return;
  over = s, over_on = true, over_fx = fx < HALO_FX_COUNT ? fx : 0, over_closing = false, over_drag = false;
  over_x = x, over_y = y, over_s = fmaxf(w, h) / HALO_W;
  halo_spring_set(over_q, 0), halo_spring_set(over_pull, 0);
  over_q.target = 1;
  if (over.enter) over.enter();
  const void *owner = halo_finger.owner;  // палец этого кадра (нажатие на элемент) — не ему
  halo_finger.owner = &over_owner;
  if (over.update) over.update(0);
  halo_finger.owner = owner;
}

float halo_nav_pull() {
  return fmaxf(0, over_on ? over_pull.x : sheet_pull.x);
}

// Экран поверх циферблата: жест назад снизу, пружины; true — экран открыт или его ведут пальцем (палец — ему), false —
// его нет или он уже закрывается (палец — циферблату и галерее, хотя экран ещё доигрывает закрытие)
static bool over_update(float dt) {
  if (!over_on) return false;
  HaloFinger &f = halo_finger;
  // Жест назад — только пока экран открыт или открывается. Закрывающийся (отпустили, и он доезжает пружиной) жесты
  // не берёт: иначе свайп снизу сразу после «назад» (хотели открыть галерею) хватал его как новый «назад», ставил путь
  // по пальцу как от открытого — и экран погоды на долю секунды показывался снова
  const bool open = over_q.target > 0.5f;
  if (open && f.drag_start && !f.horiz && f.y < f.y0 && f.y0 > HALO_H - OVER_ZONE && !over_drag &&
      halo_finger_claim(&over_owner))
    over_drag = over_closing = true, over_y0 = f.y;
  if (cmd_back) {
    cmd_back = false;
    if (!over_drag) over_closing = true, over_q.target = 0;
  }
  if (over_drag) {
    if (f.down && f.owner == &over_owner) {  // переход — за пальцем
      const float d = fmaxf(0, over_y0 - f.y);
      over_pull.x = d, over_pull.v = -f.vy, over_pull.target = 0;
      over_q.x = 1 - halo_clamp01(d / OVER_TRAVEL), over_q.v = f.vy / OVER_TRAVEL;
    } else {  // отпустили: дальше трети пути или бросок вверх быстрее 700 px/с — закрыть, иначе обратно
      over_drag = false;
      over_q.target = over_q.x < 0.67f || f.vy < -700 ? 0 : 1;
    }
  }
  if (!over_drag) {
    halo_spring_run(over_q, over_q.target > 0.5f ? K_OVER_IN : K_OVER_OUT, dt);
    halo_spring_run(over_pull, K_PULL, dt);
  }
  const bool rest = !over_drag && over_q.x == over_q.target;
  if (rest && over_q.x == 1) over_closing = false;
  // Палец и пружины экрана — когда он открыт или уже открывается и прошёл полпути (как у главного: отпустили — следующее
  // касание уже его). Пружина открытия успокаивается только через ~0,48 с после нажатия, хотя к 0,3 с экран на вид уже
  // открыт, — и свайп, начатый раньше, пропадал: экран тогда стоял, начало жеста он не видел
  if (!over_drag && over_q.target == 1 && over_q.x > 0.5f && over_pull.x == 0 && over.update) over.update(dt);
  // Закрыли: спрятать и забыть — как только переход дошёл до циферблата. «Потягивание» (насколько тянули, для точек
  // страниц) гаснет дольше (~0,7 с от свайпа), но экрана уже не видно — его не ждём
  if (rest && over_q.x == 0) over_forget();
  return over_on && (over_drag || over_q.target > 0.5f);  // закрывается — палец уже циферблату и галерее
}

// Виды циферблата (vf) и экрана над ним (vo) по пружине перехода
static void over_views(HaloView &vf, HaloView &vo) {
  const float q = over_q.x, lift = 0.25f * over_pull.x;
  switch (over_fx) {
    case HALO_FX_ZOOM: {
      // Экран вырастает из элемента (масштаб — от его размера, центр — от его центра) и проявляется за первую треть;
      // циферблат гаснет под ним. Назад — тот же путь обратно, но циферблат проявляется раньше: его видно, пока тянут.
      // Под экраном — чёрный круг его размера (over_cover): экран — непрозрачная карточка, а не надписи поверх цифр
      // циферблата. Циферблат — без масштаба: раньше он ещё и «наезжал» на элемент (до 1,35), но огромные цифры с
      // масштабом — самое дорогое в кадре: циферблат в 1,2 раза крупнее — кадры открытия по 16 мс на ядро и разрывы
      const float s = over_s + (1 - over_s) * q;
      vo = {(over_x - CX) * (1 - q), (over_y - CY) * (1 - q) - lift, s, halo_clamp01(q * 3), true};
      vf = {0, 0, 1, halo_clamp01(1 - q / (over_closing ? 0.65f : 0.35f))};
      break;
    }
    case HALO_FX_FADE: {
      // Material fade through: старый гаснет за первые 35 %, новый проявляется после, вырастая с 0,92; одновременно
      // их не видно никогда. Назад — одно непрерывное движение вверх по пути q (и за пальцем, и после него): экран
      // уходит вверх на T·(1 − q), уменьшается к 0,92 и гаснет за первую половину пути; циферблат во второй приходит
      // снизу с той же скоростью (T·q → 0), уменьшается с 1,08 до 1 и проявляется — те же числа в зеркале. T = 70 px:
      // содержимое идёт за пальцем на четверть его пути. С экраном они не перекрываются: масштаб огромных цифр по
      // чёрному фону укладывается в кадр
      if (!over_closing) {
        const float k = halo_clamp01((q - 0.35f) / 0.65f);
        vo = {0, -lift, 0.92f + 0.08f * k, k, true};
        vf = {0, 0, 1, halo_clamp01(1 - q / 0.35f)};
      } else {
        const float T = 0.25f * OVER_TRAVEL, k = halo_clamp01((q - 0.5f) / 0.5f), kf = halo_clamp01((0.5f - q) / 0.5f);
        vo = {0, -(1 - q) * T, 0.92f + 0.08f * k, k, true};
        vf = {0, q * T, 1 + 0.08f * (1 - kf), kf};
      }
      break;
    }
    default: {  // HALO_FX_SLIDE: всё едет вверх на 0,3 экрана — и при открытии, и назад (по направлению пальца); как
      // у fade through, одновременно их не видно: старый гаснет, потом проявляется новый
      const float D = HALO_H * 0.3f;
      if (!over_closing) {
        vo = {0, (1 - q) * D, 1, halo_clamp01((q - 0.4f) / 0.6f), true};
        vf = {0, -q * D, 1, halo_clamp01(1 - q / 0.4f)};
      } else {
        vo = {0, -(1 - q) * D, 1, halo_clamp01((q - 0.5f) / 0.5f), true};
        vf = {0, q * D, 1, halo_clamp01((0.5f - q) / 0.5f)};
      }
      break;
    }
  }
  // Последние доли процента масштаба не видны, а рисовать с масштабом в разы дороже: у 1 — ровно 1
  if (fabsf(vo.s - 1) < 0.006f) vo.s = 1;
  if (fabsf(vf.s - 1) < 0.006f) vf.s = 1;
  // Циферблата уже (или ещё) не видно или под экраном чёрная карточка — под ним чёрный: гаснуть тусклым цветом, а не
  // смешиванием (в разы дешевле)
  over_cover.alpha = 0;
  if (over_fx == HALO_FX_ZOOM && vf.a > 0) {
    // карточка непрозрачна уже к 1/6 пути, пока она маленькая: полупрозрачный чёрный круг — это смешивание каждого
    // пикселя под ним (большой — 10 мс кадра), непрозрачный — заливка, а циферблат под ним и не рисуется
    COVER_PUT(over_cover, vo);
    over_cover.alpha = over_closing ? vo.a : halo_clamp01(q * 6);
  }
  if (vf.a <= 0 || over_cover.alpha > 0) vo.over = false;
}

// Пришли на циферблат: открытый на главном экран закрыть, главный — в начало (его enter): в следующий раз его
// открывают, как впервые. Назад жестом слева с открытого экрана главный, наоборот, остаётся, где был
static void face_arrived() {
  if (!main_seen) return;
  main_seen = false;
  if (view != V_HOME) {
    for (; depth > 0; depth--)
      if (scr[depth].layout) scr[depth].layout({0, 0, 1, 0});
    view = V_HOME, back_drag = false;
  }
  if (scr[0].enter) scr[0].enter();
}

// Сразу на циферблат (в Always On его и так уже видно одного): закрыть всё над ним, главный — в начало, а выход из
// Always On — как с циферблата (сам циферблат из вида amb_v, без fade through)
void halo_nav_home_now() {
  const HaloView hidden = {0, 0, 1, 0};
  if (over_on) over_forget();
  over_drag = over_closing = false;
  if (has_face) {
    if (sheet.x > 0 || sheet.target > 0) main_seen = true;
    sheet_drag = false;
    halo_spring_set(sheet, 0), halo_spring_set(sheet_pull, 0);
    face_arrived();  // открытое на главном — закрыть, главный — с начала
  }
  for (; depth > 0; depth--)
    if (scr[depth].layout) scr[depth].layout(hidden);
  view = V_HOME, back_drag = false, cmd_back = false, cmd_face = -1;
  if (amb_k > 0) amb_morph = true;
  halo_damage_all();
}

// Жест с главного на циферблат: 1 — свайп вниз от верхнего края, −1 — вверх от нижнего, 0 — не он. Какой — у
// открытого на главном экрана или у самого главного (HaloScreen::home; 0 — вниз). Только от самого края: у списка
// весь остальной экран — прокрутка
static float home_gesture(const HaloFinger &f) {
  uint8_t g = scr[depth].home;
  if (!g) g = HALO_HOME_DOWN;
  if (g & HALO_HOME_DOWN && f.y > f.y0 && f.y0 < face_zone) return 1;
  if (g & HALO_HOME_UP && f.y < f.y0 && f.y0 > HALO_H - OVER_ZONE) return -1;
  return 0;
}

// Циферблат и главный над ним: жест, доводка; true — главному можно отдать палец (его update можно звать): он на
// месте или доезжает к нему после того, как отпустили. Пока главный тянут пальцем — нет, а отпустили — следующее
// касание уже его: ленту можно листать, пока она ещё доезжает и отскакивает (иначе свайпы в эти ~0,4 с пропадали)
static bool sheet_update(float dt) {
  HaloFinger &f = halo_finger;
  if (!has_main) {  // главного нет: циферблат на месте всегда, свайп вверх по нему — его собственный
    cmd_face = -1;
    if (face.update) face.update(dt);
    return false;
  }
  const bool up = sheet.target > 0.5f;
  const bool here = view == V_HOME || view == V_TOP;  // не посреди перехода Android 16 между ними
  // Вверх по циферблату (откуда угодно); с главного или открытого на нём экрана — его жест
  if (f.drag_start && !f.horiz && here && !sheet_drag) {
    const float dir = up ? home_gesture(f) : f.y < f.y0 ? 1 : 0;
    if (dir != 0 && halo_finger_claim(&sheet_owner)) {
      // Подхватили на ходу движением в ту же сторону — переход идёт дальше (вперёд или назад) без скачка. В другую
      // (ушли с погоды вверх и сразу свайп вверх к галерее) — прошлый доигрывается сразу, новый — с места
      if (sheet.x != sheet.target && dir != sheet_dir) {
        halo_spring_set(sheet, sheet.target), halo_spring_set(sheet_pull, 0);
        if (sheet.x == 0) face_arrived();
      }
      // Куда переход, решаем только с места: подхватили в пути — масштаб у того, кого сейчас видно, не скачет
      if (sheet.x == sheet.target) sheet_in = sheet.x < 0.5f, sheet_dir = dir;
      sheet_drag = true, sheet_g0 = sheet.x, sheet_y0 = f.y;
      // Экран поверх циферблата ещё доезжал к закрытию — закрыть сразу: новый жест уже о другом
      if (over_on) halo_spring_set(over_q, 0), halo_spring_set(over_pull, 0);
    }
  }
  if (cmd_face >= 0) {
    if (here && !sheet_drag) {
      if (sheet.x == sheet.target) {  // с места; в пути — тот же переход вперёд или назад
        sheet_in = cmd_face == 1;
        sheet_dir = cmd_face == 0 && view == V_TOP && scr[depth].home == HALO_HOME_UP ? -1 : 1;
      }
      sheet.target = cmd_face;
    }
    cmd_face = -1;
  }
  if (sheet_drag) {
    if (f.down && f.owner == &sheet_owner) {  // переход — за пальцем: путь 280 px, как жест назад у экрана поверх
      const float d = (sheet_y0 - f.y) * sheet_dir;  // px к главному
      sheet.x = halo_clamp01(sheet_g0 + d / OVER_TRAVEL), sheet.v = -f.vy * sheet_dir / OVER_TRAVEL;
#ifndef HALO_ROUND
      // (скорость — только жесту снизу, которому «потягивание» и нужно: при свайпе вверх к главному его пружина
      // раскачивалась от скорости пальца, и экран перерисовывался целиком ещё ~0,4 с после конца перехода)
      sheet_pull = {sheet_dir < 0 ? fmaxf(0, sheet_y0 - f.y) : 0, sheet_dir < 0 ? -f.vy : 0, 0};
#else
      sheet_pull = {sheet_dir < 0 ? fmaxf(0, sheet_y0 - f.y) : 0, -f.vy, 0};
#endif
    } else {  // отпустили: бросок быстрее 700 px/с — куда бросили, иначе — дальше трети пути от начала
      sheet_drag = false;
      const float u = -f.vy * sheet_dir;  // скорость к главному, px/с
      sheet.target = u > 700 ? 1 : u < -700 ? 0 : sheet_in ? (sheet.x > 0.33f ? 1 : 0) : (sheet.x < 0.67f ? 0 : 1);
#ifndef HALO_ROUND
      sh_restart = true;
#endif
    }
  }
  if (!sheet_drag) {
#ifndef HALO_ROUND
    // Прямоугольник: остаток пути — по времени (своя доля SH_S) и кривой Material standard, без пружины и её хвоста
    if (sh_restart || sheet.target != sh_tgt) sh_x0 = sheet.x, sh_t = 0, sh_tgt = sheet.target, sh_restart = false;
    if (sheet.x != sheet.target) {
      sh_t += dt / (SH_S * fmaxf(fabsf(sheet.target - sh_x0), 0.1f));
      sheet.x = sh_t >= 1 ? sheet.target : sh_x0 + (sheet.target - sh_x0) * halo_ease(E_AXIS, sh_t);
    }
    sheet.v = 0;
#else
    halo_spring_run(sheet, sheet.target > 0.5f ? K_OVER_IN : K_OVER_OUT, dt);  // те же, что у Fade
#endif
    halo_spring_run(sheet_pull, K_PULL, dt);
  }
  // К циферблату: последний пиксель пути (его сдвиг 70·x, масштаб — в четверти пикселя) не ждём — иначе циферблат
  // брал палец только через ~0,55 с после свайпа, хотя к ~0,38 с на вид уже стоял
  if (!sheet_drag && sheet.target == 0 && sheet.x > 0 && sheet.x < 0.015f) halo_spring_set(sheet, 0);
  if (sheet.x > 0) main_seen = true;
  const bool rest = !sheet_drag && sheet.x == sheet.target;
  if (rest && sheet.x == 0) {
    face_arrived();
    if (face.update) face.update(dt);  // палец и пружины циферблата — только когда он на месте
  }
  return !sheet_drag && sheet.target == 1;
}

// Кадр верхнего экрана: жест назад от левого края — или его собственное обновление (может открыть ещё один поверх)
static void top_frame(float dt) {
  HaloFinger &f = halo_finger;
  bool go = f.drag_start && f.horiz && f.x > f.x0 && f.x0 < back_zone && halo_finger_claim(&nav_owner);
  if (cmd_back) cmd_back = false, go = true;
  if (go) {
    if (view == V_OPEN) finish_open();  // (открытие ещё доигрывало хвост)
    back_drag = f.owner == &nav_owner && f.down;
    back_p = {0, 0, back_drag ? 0.0f : 1.0f};
    view = V_BACK;
#ifndef HALO_ROUND
    ax_t = 0, ax_e0 = 0;
#endif
    return;
  }
  if (scr[depth].update) scr[depth].update(dt);
}

void halo_nav_update(float dt) {
  HaloFinger &f = halo_finger;
  const bool main_on = !has_face || (!over_update(dt) && sheet_update(dt));
  if (!main_on && !over_on) cmd_back = false;  // на циферблате «назад» некуда: не копить до следующего экрана
  if (main_on) switch (view) {
    case V_HOME:
      cmd_back = false;
      if (scr[0].update) scr[0].update(dt);  // может открыть экран (halo_nav_open) — тогда ниже уже V_OPEN
      break;
    case V_TOP:
      top_frame(dt);  // может открыть ещё один поверх — тогда уже V_OPEN
      break;
    case V_OPEN:
#ifndef HALO_ROUND
      if (grow_on) {  // (последние полтора процента пути не стоят кадра с масштабом — сразу на месте, как плитка видео)
        halo_spring_run(t_grow, K_GROW, dt);
        if (t_grow.x > 0.985f) halo_spring_set(t_grow, 1), grow_on = false, view = V_TOP;
      } else {
        ax_t += dt / AX_OPEN_S;
        if (ax_t >= 1) ax_t = 1, view = V_TOP;
      }
#else
      halo_spring_run(t_slide, K_IN_SLIDE, dt), halo_spring_run(t_scale, K_IN_SCALE, dt);
      halo_spring_run(t_fade, K_IN_FADE, dt), halo_spring_run(t_oscale, K_OUT_SCALE, dt);
      halo_spring_run(t_oslide, K_OUT_SLIDE, dt), halo_spring_run(t_ofade, K_OUT_FADE, dt);
      if (fabsf(t_slide.x - 1) < 0.002f && fabsf(t_slide.v) < 0.05f && fabsf(t_scale.x - 1) < 0.002f) view = V_TOP;
#endif
      // На месте для глаза — палец уже экрану (хвост пружины доигрывается тут же, вид не меняется). И в кадре, где
      // переход кончился, — тоже: раньше в этот кадр экран не обновлялся, и нажатие или отпускание, пришедшееся на
      // него, пропадало
      if (view == V_TOP || open_late()) top_frame(dt);
      break;
    case V_BACK:
#ifndef HALO_ROUND
      // Прямоугольник: путь — по пальцу (280 px — весь, как у перехода к циферблату свайпом); отпустили — бросок
      // быстрее 700 px/с туда, куда бросили, иначе дальше трети пути — назад. Дальше по времени: назад — остаток пути за
      // свою долю AX_CLOSE_S, обратно — за AX_RETURN_S, по кривой Material standard
      if (back_drag) {
        if (f.down) {
          back_p.x = halo_clamp01((f.x - f.x0) / OVER_TRAVEL);
        } else {
          back_drag = false;
          back_p.target = f.vx > 700 ? 1 : f.vx < -700 ? 0 : back_p.x > 0.33f ? 1 : 0;
          ax_t = 0, ax_e0 = back_p.x;
        }
      }
      if (!back_drag) {
        const bool go = back_p.target > 0.5f;
        ax_t += dt / (go ? AX_CLOSE_S * fmaxf(1 - ax_e0, 0.1f) : AX_RETURN_S);
        if (ax_t > 1) ax_t = 1;
        const float k = halo_ease(E_AXIS, ax_t);
        back_p.x = go ? ax_e0 + (1 - ax_e0) * k : ax_e0 * (1 - k), back_p.v = 0;
        if (ax_t >= 1) back_p.x = back_p.target;
      }
#else
      if (back_drag) {
        if (f.down) {
          back_p.x = halo_clamp01((f.x - f.x0) / (HALO_W - f.x0)), back_p.v = 0;
        } else {  // отпустили: дальше половины или бросок быстрее 800 dp/с — назад, иначе обратно
          back_drag = false;
          back_p.target = back_p.x > 0.5f || f.vx > 1600 ? 1 : 0;
          back_p.v = f.vx / HALO_W;
        }
      }
      if (!back_drag) halo_spring_run(back_p, K_DONE, dt);
#endif
      // Дошёл для глаза — готово: назад — нижний экран в 2 px от места (масштаб — в полпикселе), обратно — верхний в
      // 2 px; пружина без перелёта, остаток незаметен. Раньше ждали 0,002 и покоя — ещё ~60 мс, и нажатие пропадало
      if (!back_drag && fabsf(back_p.x - back_p.target) < (back_p.target > 0.5f ? 0.01f : 0.005f)) {
        if (back_p.target > 0.5f) {  // закрыли верхний: спрятать его, ниже — на уровень
          if (scr[depth].layout) scr[depth].layout({0, 0, 1, 0});
          depth--;
        }
        view = depth ? V_TOP : V_HOME;
        // кадр, где переход кончился, — уже экрана, к которому вернулись (иначе касание в нём пропадало)
        if (view == V_TOP) top_frame(dt);
        else if (scr[0].update) cmd_back = false, scr[0].update(dt);
      }
      break;
  }
  // Виды экранов в этом кадре
  const HaloView id{}, hidden = {0, 0, 1, 0};
  clip = false;
  if (view == V_OPEN) {
    T.nx = (1 - t_slide.x) * HALO_W / 2, T.ns = fminf(1, 0.8f + 0.2f * t_scale.x), T.na = halo_clamp01(t_fade.x);
    T.ox = -t_oslide.x * HALO_W / 2, T.os = 1 - 0.15f * t_oscale.x, T.oa = 1 - 0.4f * halo_clamp01(t_ofade.x);
  } else if (view == V_BACK) {
    const float p = halo_clamp01(back_p.x);
    T.nx = p * HALO_W, T.ns = 1 - 0.2f * p, T.na = 1;
    T.ox = -(1 - p) * HALO_W / 2, T.os = 0.8f + 0.2f * p, T.oa = 0.5f + 0.5f * p;
  }
#ifndef HALO_ROUND
  if (view == V_OPEN && grow_on) {
    // Элемент вырастает в экран (halo_nav_open_from) — как плитка видео в плеер. Окно (cover) идёт от прямоугольника
    // элемента ко всему экрану, скругление — к 0. Сначала оно проявляется поверх элемента его же цветом (надпись и значок
    // элемента растворяются), и за первую треть пути цвет уходит в чёрный. Нижний гаснет за первую пятую: пока окно
    // мало, видно и то и другое — дальше рисовать его незачем. Экран внутри проявляется на чёрном (с 0,3 до 0,7 пути),
    // уменьшенный так, чтобы заполнить окно с запасом; лишнее обрезано окном (grow_box). Своего размера он достигает,
    // когда окно открыто на 85 %, и дальше окно только открывает его края: экран с масштабом стоит на 3 мс дороже
    // (плеер Music на P4 — 15,7 мс против 12,7), и последние кадры пути опаздывали
    const float q = halo_clamp01(t_grow.x), k = 1 - halo_clamp01(q / 0.3f);
    const float x0 = grow_from[0] * (1 - q), y0 = grow_from[1] * (1 - q);
    const float x1 = grow_from[2] + (HALO_W - grow_from[2]) * q, y1 = grow_from[3] + (HALO_H - grow_from[3]) * q;
    const float r = grow_r * (1 - q), in = r * 0.3f;  // (на 0,3 радиуса внутрь: в скруглённые углы экран не вылезает)
    grow_box[0] = x0 + in, grow_box[1] = y0 + in, grow_box[2] = x1 - in, grow_box[3] = y1 - in;
    v_home = {0, 0, 1, halo_clamp01(1 - q / 0.2f)};
    // (его середина — от середины окна к середине экрана: в своём размере он стоит, где останется)
    const float fill = fmaxf((x1 - x0) / HALO_W, (y1 - y0) / HALO_H), sc = fminf(1, fill / 0.85f), mv = (1 - sc) / fmaxf(1 - fill, 1e-3f);
    v_top = {((x0 + x1) / 2 - CX) * mv, ((y0 + y1) / 2 - CY) * mv, sc, halo_clamp01((q - 0.3f) / 0.4f)};
    if (fabsf(v_top.s - 1) < 0.006f) v_top = {0, 0, 1, v_top.a};
    clip = true;
    cover.x = (x0 + x1) / 2, cover.y = (y0 + y1) / 2, cover.w = x1 - x0, cover.h = y1 - y0, cover.r = r, cover.scale = 1;
    cover.color = (uint32_t)((grow_c >> 16 & 255) * k) << 16 | (uint32_t)((grow_c >> 8 & 255) * k) << 8 | (uint32_t)((grow_c & 255) * k);
    cover.alpha = halo_clamp01(q / 0.12f), cover.on_black = false;
    hide_under = cover.alpha >= 0.999f;
    halo_damage_all();
    settle_frame = true;
  } else if (view == V_OPEN || view == V_BACK) {
    // Material shared axis X (вариант 6 из web/mockups/rect-transitions.html). Открыть: нижний за первые 35 % пути
    // уходит влево на 30 px и гаснет, потом открытый приходит справа с 30 px и проявляется. Назад — зеркально: верхний
    // уходит вправо и гаснет, нижний приходит слева. Без масштаба, вместе их не видно: ни чёрной плашки, ни перерисовки
    // всего экрана — элементы сами сообщают, где изменились. (До того были: fade through как у свайпа к циферблату —
    // экраны с содержимым наверху съезжали вниз, переход читался «сверху вниз»; рост из карточки и въезд с отскоком —
    // длинные движения на большом экране)
    const bool op = view == V_OPEN;
    const float e = op ? halo_ease(E_AXIS, ax_t) : halo_clamp01(back_p.x);
    const float a = halo_clamp01(e / 0.35f), b = halo_clamp01((e - 0.35f) / 0.65f);
    if (op) v_home = {-AX_SHIFT * a, 0, 1, 1 - a}, v_top = {AX_SHIFT * (1 - b), 0, 1, b};
    else v_top = {AX_SHIFT * a, 0, 1, 1 - a}, v_home = {-AX_SHIFT * (1 - b), 0, 1, b};
    cover.alpha = 0, hide_under = false;
    halo_subpixel = true;  // (30 px за 0,3 с — медленно: буквы на доли пикселя, иначе дрожат)
    settle_frame = true;
  } else
#endif
  if (view == V_OPEN || view == V_BACK) {
    v_home = {T.ox, 0, T.os, T.oa};
    v_top = {T.nx, 0, T.ns, T.na, true};
    clip = true;
    // Чёрный круг под открытым экраном непрозрачен уже к четверти его проявления: полупрозрачный — это смешивание
    // каждого пикселя под ним (а под ним — уменьшенный главный с карточками): 2–4 мс на ядро, у Settings кадры по 22 мс.
    // Непрозрачный — заливка, главный под ним не рисуется, а элементы открытого гаснут тусклым цветом на чёрном
    cover.alpha = view == V_OPEN && scr[depth].opaque && T.na < 0.999f ? 0 : halo_clamp01(T.na * 4);
    hide_under = cover.alpha >= 0.999f;
    if (hide_under) v_top.over = false;
    halo_damage_all();  // уменьшенный главный обрезается по кругу — проще перерисовать всё (и так меняется всё)
    settle_frame = true;
  } else {
    v_home = view == V_HOME ? id : hidden;
    v_top = view == V_TOP ? id : hidden;
    cover.alpha = 0, hide_under = false;
    if (settle_frame) halo_damage_all(), settle_frame = false;  // кадр после перехода — начисто
  }
  // Циферблат и главный по пути p (0 — циферблат, 1 — главный): тот, от кого уходим, гаснет за первую половину пути и
  // уменьшается к 0,92, тот, к кому идём, проявляется во второй, уменьшаясь с 1,08 до 1 (как Fade назад). Оба едут за
  // пальцем на четверть его пути: к главному — вверх, к циферблату — вниз. Фон у обоих чёрный, и вместе их не видно —
  // ни чёрного круга под главным, ни перерисовки всего экрана: элементы сами сообщают, где изменились
  if (has_face) {
    const float p = sheet.x, T = 0.25f * OVER_TRAVEL, oy = sheet_dir * (1 - p) * T;  // sheet_dir < 0 — в зеркале
    if (p <= 0.0005f) {
      v_face = id, v_home = v_top = hidden;
    } else if (p >= 0.9995f) {
      v_face = hidden, v_home.y += oy, v_top.y += oy;
    } else {
#ifndef HALO_ROUND
      // Прямоугольник — Material shared axis Y (как открыть и назад, только вертикально): тот, от кого уходим, за
      // первые 35 % пути уходит на 40 px по ходу пальца и гаснет, потом тот, к кому идём, приходит с той стороны и
      // проявляется. Без масштаба (уменьшаясь к середине, экраны с содержимым наверху съезжали вниз) и без пружины
      const float e = sheet_in ? p : 1 - p, a = halo_clamp01(e / 0.35f), b = halo_clamp01((e - 0.35f) / 0.65f);
      const float out = 1 - a, in = b, dy_out = SH_SHIFT * a, dy_in = SH_SHIFT * (1 - b);
      // (sheet_dir 1: к главному — вверх, к циферблату — вниз; −1 — наоборот)
      const float yf = sheet_in ? -sheet_dir * dy_out : -sheet_dir * dy_in, ym = sheet_in ? sheet_dir * dy_in : sheet_dir * dy_out;
      v_face = {0, yf, 1, sheet_in ? out : in};
      for (HaloView *v : {&v_home, &v_top}) v->y += ym, v->a *= sheet_in ? in : out, v->over = false;
#else
      const float kf = halo_clamp01(1 - p / 0.5f), km = halo_clamp01((p - 0.5f) / 0.5f);  // видно циферблат / главный
      float sf = sheet_in ? 0.92f + 0.08f * kf : 1 + 0.08f * (1 - kf);
      float sm = sheet_in ? 1 + 0.08f * (1 - km) : 0.92f + 0.08f * km;
      if (fabsf(sf - 1) < 0.006f) sf = 1;  // последние доли процента масштаба не видны, а рисовать с ним дороже
      if (fabsf(sm - 1) < 0.006f) sm = 1;
      v_face = {0, -sheet_dir * p * T, sf, kf};
      for (HaloView *v : {&v_home, &v_top}) v->y += oy, v->s *= sm, v->a *= km, v->over = false;
#endif
    }
    if (p != sheet.target || sheet_drag || sheet_pull.x != 0) halo_subpixel = true, settle_frame = true;
    if (over_on) {  // экран поверх циферблата (галерея в это время внизу)
      over_views(v_face, v_over);
      if (over_drag || over_q.x != over_q.target || over_pull.x != 0) halo_subpixel = true;
    }
  }
  // Переход тронулся с места (открыть, назад, к циферблату и обратно, экран поверх него — пружиной или пальцем): сразу
  // вслед за проходом. Иначе движок начинал «впереди» и через 3–4 кадра переходил вслед уже посреди движения — по
  // кадру, показанному дважды, на каждый переход (halo_heavy)
  const bool moving = view == V_OPEN || view == V_BACK || sheet_drag || sheet.x != sheet.target ||
                      sheet_pull.x != 0 || (over_on && (over_drag || over_q.x != over_q.target || over_pull.x != 0));
  if (moving && !nav_moving) halo_heavy();
  nav_moving = moving;
  // Always On (halo_nav_ambient): все виды — ещё и через вид amb_v. Был циферблат (или в конце не он) — всё просто
  // уходит в этот вид; с другого экрана — fade through: всё, что на экране, гаснет за первую половину пути, циферблат
  // в виде amb_v проявляется во второй. Вместе их не видно: ни смешивания, ни перерисовки всего экрана
  if (amb_k > 0) {
    HaloView *const all[4] = {&v_face, &v_home, &v_top, &v_over};
    for (HaloView *v : all) view_under(*v, amb_v);
    if (amb_face && !amb_morph) {
      const float out = halo_clamp01(1 - amb_k / 0.5f), in = halo_clamp01((amb_k - 0.5f) / 0.5f);
      for (HaloView *v : all) v->a *= out, v->over = false;
      if (in > 0) v_face = amb_v, v_face.a *= in;
    }
    if (amb_k < 1) halo_subpixel = true;
    if (over_on) COVER_PUT(over_cover, v_over);
  }
#ifndef HALO_ROUND
  if (clip && !(view == V_OPEN && grow_on)) COVER_PUT(cover, v_top);  // (растущее окно поставлено выше)
#else
  if (clip) COVER_PUT(cover, v_top);  // круги — по итоговым видам
#endif
  if (has_face && face.layout) face.layout(v_face);
  // v_home — нижнему из двух верхних (на главном — самому главному), v_top — верхнему
  if (depth == 0) {
    if (scr[0].layout) scr[0].layout(v_home);
  } else {
    if (scr[depth - 1].layout) scr[depth - 1].layout(v_home);
    if (scr[depth].layout) scr[depth].layout(v_top);
  }
  if (over_on && over.layout) over.layout(v_over);
  if (!over_on) over_cover.alpha = 0;
  halo_item_commit(over_cover);
  halo_item_commit(cover);
}

#ifdef HALO_ROUND
// Пачка целиком под кругом (cx, cy, r): обе её крайние строки внутри него по всей ширине круга экрана
static bool band_under(float cx, float cy, float r) {
  for (int k = 0; k < 2; k++) {
    const float y = k ? hb.y + hb.rows - 0.5f : hb.y + 0.5f, ds = y - CX, dc = y - cy;
    if (fabsf(dc) >= r) return false;
    const float hs = halo_sqrt(fmaxf(0, CX * CX - ds * ds)), hc = halo_sqrt(r * r - dc * dc);
    if (fmaxf(hb.x0, CX - hs) < cx - hc || fminf(hb.x1, CX + hs) > cx + hc) return false;
  }
  return true;
}

void halo_nav_draw() {
  // Циферблат — только в строках, которые главный закрывает не целиком; под главным — чёрный круг. Под чёрной
  // карточкой экрана (Zoom) — тоже: там, где она закрывает всю строку, ни циферблат, ни её саму не рисуем
  const bool card = over_cover.shown && over_cover.alpha >= 0.999f;
  const bool under = card && band_under(over_cover.x, over_cover.y, over_cover.r);
  if (has_face && !under) {
    // Под непрозрачной карточкой циферблат в этой пачке — только левее и правее того, что она закрывает во всех её
    // строках (по самой дальней от центра карточки строке): огромные цифры за карточкой — работа впустую
    const int x0 = hb.x0, x1 = hb.x1;
    float hc = -1;
    if (card) {
      const float dm = fmaxf(fabsf(hb.y + 0.5f - over_cover.y), fabsf(hb.y + hb.rows - 0.5f - over_cover.y));
      if (dm < over_cover.r) hc = halo_sqrt(over_cover.r * over_cover.r - dm * dm);
    }
    const int c0 = (int)(over_cover.x - hc) + 1, c1 = (int)(over_cover.x + hc) - 1;  // закрыто целиком
    if (hc > 2 && c0 < c1) {
      if (c0 > x0) hb.x1 = min(x1, c0 - 1), face.draw ? face.draw() : (void)0, halo_item_draw(over_cover);
      if (c1 < x1) hb.x0 = max(x0, c1 + 1), hb.x1 = x1, face.draw ? face.draw() : (void)0, halo_item_draw(over_cover);
      hb.x0 = x0, hb.x1 = x1;
    } else {
      if (face.draw) face.draw();
      halo_item_draw(over_cover);
    }
  }
  if (over_on && over.draw) over.draw();  // экран поверх циферблата
  // Под непрозрачным открытым экраном главный не виден: рисуем его только левее круга открытого во всех строках
  // пачки (его левый край — по самой дальней от центра строке пачки), как slide_draw в AmoledWear
  const int x0 = hb.x0, x1 = hb.x1;
  if (hide_under) {
    const float cx = cover.x, r = cover.r;
    const float dm = fmaxf(fabsf(hb.y + 0.5f - cover.y), fabsf(hb.y + hb.rows - 0.5f - cover.y));
    if (dm < r) hb.x1 = min(x1, (int)(cx - halo_sqrt(r * r - dm * dm)) + 1);
  }
  const HaloScreen &lo = scr[depth ? depth - 1 : 0];
  if (hb.x0 <= hb.x1) {
    if (lo.draw) lo.draw();
    if (clip) halo_clip_circle(CX + v_home.x, CX + v_home.y, CX * v_home.s);
  }
  hb.x0 = x0, hb.x1 = x1;
  halo_item_draw(cover);
  // Верхний экран в переходе — только в своём круге: что он рисует за краем (в покое этого не видно — край экрана),
  // при уменьшении вылезает поверх нижнего (строки списка под страницей WearLab №67, края карточек ленты)
  if (clip) hu_item_clip(cover.x, cover.y, cover.r);
  if (depth && scr[depth].draw) scr[depth].draw();
  if (clip) hu_item_clip(0, 0, 0);
}

#else  // прямоугольный экран

// Только внутри прямоугольника x0, y0 — x1, y1
static void box_draw(void (*draw)(), float x0, float y0, float x1, float y1) {
  if (!draw) return;
  const HaloBand band = hb;
  const int ax = max(band.x0, halo_floor(x0 + 0.5f)), bx = min(band.x1, halo_floor(x1 - 0.5f));
  const int ay = max(band.y, halo_floor(y0 + 0.5f)), by = min(band.y + band.rows - 1, halo_floor(y1 - 0.5f));
  if (ax > bx || ay > by) return;
  hb.buf = band.buf + (ay - band.y) * band.w, hb.y = ay, hb.rows = by - ay + 1;
  hb.x0 = ax, hb.x1 = bx;
  draw();
  hb = band;
}

// Экран с видом v — только внутри своей рамки: всё, что он рисует за её краем (в покое этого не видно — край экрана),
// при уменьшении вылезло бы наружу (карточки ленты за её концом)
static void page_draw(void (*draw)(), const HaloView &v) {
  const float cx = CX + v.x, cy = CY + v.y, hw = CX * v.s, hh = CY * v.s;
  box_draw(draw, cx - hw, cy - hh, cx + hw, cy + hh);
}

// Строки пачки целиком в высоте чёрной плашки c: дальняя от её середины строка внутри
static bool band_in_cover(const HaloItem &c) {
  return fmaxf(fabsf(hb.y + 0.5f - c.y), fabsf(hb.y + hb.rows - 0.5f - c.y)) < c.h / 2;
}

void halo_nav_draw() {
  const int x0 = hb.x0, x1 = hb.x1;
  if (has_face && face.draw) {
    // Под непрозрачной карточкой экрана (Zoom) циферблат — только левее и правее того, что она закрывает во всех строках
    const float hc = over_cover.shown && over_cover.alpha >= 0.999f && band_in_cover(over_cover)
                         ? over_cover.w / 2 - over_cover.r : -1;
    const int c0 = (int)(over_cover.x - hc) + 1, c1 = (int)(over_cover.x + hc) - 1;
    if (hc > 2 && c0 < c1) {
      if (c0 > x0) hb.x1 = min(x1, c0 - 1), face.draw(), halo_item_draw(over_cover);
      if (c1 < x1) hb.x0 = max(x0, c1 + 1), hb.x1 = x1, face.draw(), halo_item_draw(over_cover);
      hb.x0 = x0, hb.x1 = x1;
    } else {
      face.draw();
      halo_item_draw(over_cover);
    }
  }
  if (over_on && over.draw) over.draw();  // экран поверх циферблата
  // Под непрозрачным открытым экраном нижний не виден: в строках плашки — только левее неё (растущей из элемента — и
  // правее: она не у края)
  const bool grow = view == V_OPEN && grow_on;
  const HaloScreen &lo = scr[depth ? depth - 1 : 0];
  auto lower = [&]() {
    if (hb.x0 > hb.x1) return;
    if (clip) page_draw(lo.draw, v_home);
    else if (lo.draw) lo.draw();
  };
  if (hide_under && band_in_cover(cover)) {
    hb.x1 = min(x1, (int)(cover.x - cover.w / 2 + cover.r) + 1), lower();
    if (grow) hb.x0 = max(x0, (int)(cover.x + cover.w / 2 - cover.r) - 1), hb.x1 = x1, lower();
  } else {
    lower();
  }
  hb.x0 = x0, hb.x1 = x1;
  halo_item_draw(cover);
  if (depth && grow) box_draw(scr[depth].draw, grow_box[0], grow_box[1], grow_box[2], grow_box[3]);
  else if (depth && clip) page_draw(scr[depth].draw, v_top);
  else if (depth && scr[depth].draw) scr[depth].draw();
}
#endif
