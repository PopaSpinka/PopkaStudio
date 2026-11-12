// Слой интерфейса Halo: палец (настоящий и искусственный), точные пружины, элементы с учётом изменений, страницы.
#include "halo_internal.h"

#pragma GCC optimize("O3", "fast-math")

// ---------- Пружины ----------
HaloSpringK halo_spring(float response, float damping) {
  const float dt = 1.0f / 60, w = 2 * HALO_PI / response, zw = damping * w, e = expf(-zw * dt);
  HaloSpringK k;
  if (damping < 0.999f) {  // с перелётом
    const float wd = w * sqrtf(1 - damping * damping), c = cosf(wd * dt), s = sinf(wd * dt);
    k = {e * (c + zw / wd * s), e * s / wd, -e * w * w / wd * s, e * (c - zw / wd * s)};
  } else {  // без перелёта (критическое затухание)
    k = {e * (1 + w * dt), e * dt, -e * w * w * dt, e * (1 - w * dt)};
  }
  return k;
}

void halo_spring_run(HaloSpring &s, const HaloSpringK &k, float dt) {
  int n = (int)(dt * 60 + 0.5f);  // кадр задержался — несколько точных шагов
  n = n < 1 ? 1 : n > 6 ? 6 : n;
  for (int i = 0; i < n; i++) {
    const float d = s.x - s.target, nd = k.m00 * d + k.m01 * s.v;
    s.v = k.m10 * d + k.m11 * s.v;
    s.x = s.target + nd;
  }
  // У цели — точно в цель: иначе «почти 1» тянет за собой масштабирование и перерисовку без конца
  if (fabsf(s.x - s.target) < 0.0015f && fabsf(s.v) < 0.02f) s.x = s.target, s.v = 0;
}

// ---------- Кривые ----------
void halo_ease_make(HaloEase &e, float x1, float y1, float x2, float y2) {
  for (int i = 0; i <= 64; i++) {  // для каждого t ищем параметр кривой u, где x(u) = t (делением пополам)
    const float t = i / 64.0f;
    float lo = 0, hi = 1, u = t;
    for (int k = 0; k < 30; k++) {
      u = (lo + hi) / 2;
      const float x = 3 * (1 - u) * (1 - u) * u * x1 + 3 * (1 - u) * u * u * x2 + u * u * u;
      if (x < t) lo = u;
      else hi = u;
    }
    e.v[i] = 3 * (1 - u) * (1 - u) * u * y1 + 3 * (1 - u) * u * u * y2 + u * u * u;
  }
  e.v[0] = 0, e.v[64] = 1;
}

float halo_rubber(float d, float k) {
  const float r = (1 - 1 / (fabsf(d) * k / HALO_W + 1)) * HALO_W;
  return d < 0 ? -r : r;
}

// ---------- Палец ----------
HaloFinger halo_finger;

static volatile bool syn_req, syn_lin_req;
static volatile float syn_arg[5];
static struct {
  bool on, lin;  // lin — с постоянной скоростью и отпускает на ходу (бросок), иначе — разгон и торможение
  float t, dur, x0, y0, x1, y1;
} syn;
static uint32_t syn_us;

static void syn_start(float x0, float y0, float x1, float y1, float dur, bool lin = false) {
  syn_arg[0] = x0, syn_arg[1] = y0, syn_arg[2] = x1, syn_arg[3] = y1, syn_arg[4] = dur;
  syn_lin_req = lin;
  syn_req = true;
}

// Палец трогается с места. Тач (CST9217) держит точку пальца, который стоит, и отпускает её, только когда палец
// сдвинулся на ~12 px, — весь этот путь приходит одним отчётом (замер: 1,2 с на 316,246, потом 318,233 за 10 мс и
// дальше шагами по 1–4 px; у нижнего края чип держит и движущийся палец — полсекунды на y = 461, потом −12 px). Как есть
// это рывок в начале каждой медленной протяжки и после каждой паузы в ней — в лентах, страницах, переходах. Поэтому
// скачок после остановки (палец стоял ≥ 3 отчётов, скачок > 6 px) берётся сразу только на 2 px, остальное — смещение,
// которое отдаётся плавно, S-кривой за HOLD_RELEASE_US: сначала медленно (экран трогается без толчка), потом быстрее —
// палец догоняет; скорость для броска считается без скачка. Быстрый свайп без остановки этим не задет
#define HOLD_REPORTS 3
#define HOLD_JUMP 6.0f
#define HOLD_KEEP 2.0f
#ifndef HOLD_RELEASE_US
#define HOLD_RELEASE_US 180000
#endif
struct HoldRelease {
  bool on;
  float x, y;   // смещение в начале отдачи (вычитается из отчётов с этого момента, уходит к 0 за HOLD_RELEASE_US)
  uint32_t t;   // когда начато: время отчёта со скачком
};
static struct {
  bool have;
  HaloTouchSample last;
  int still;                 // отчётов подряд в той же точке
  HoldRelease cur, prev;     // отдача сейчас и прошлая (для отчётов до начала нынешней)
} hold;

static float hold_left(const HoldRelease &r, uint32_t t) {  // какая доля смещения ещё не отдана к моменту t
  const float u = halo_clamp01((float)(int32_t)(t - r.t) * (1.0f / HOLD_RELEASE_US));
  return 1 - u * u * (3 - 2 * u);
}

// Смещение отчёта, пришедшего в момент t
static void hold_offset(uint32_t t, float &ox, float &oy) {
  ox = oy = 0;
  const HoldRelease *r = hold.cur.on && (int32_t)(t - hold.cur.t) >= 0   ? &hold.cur
                         : hold.prev.on && (int32_t)(t - hold.prev.t) >= 0 ? &hold.prev
                                                                           : nullptr;
  if (!r) return;
  const float k = hold_left(*r, t);
  ox = r->x * k, oy = r->y * k;
}

static void hold_track(const HaloTouchSample *s, int n) {
  if (!n) return (void)(hold = {});
  int i = n - 1;  // новые отчёты — от старых к новым
  while (i >= 0 && hold.have && (int32_t)(s[i].t_us - hold.last.t_us) <= 0) i--;
  for (; i >= 0; i--) {
    const HaloTouchSample &p = s[i];
    if (hold.have) {
      const float dx = p.x - hold.last.x, dy = p.y - hold.last.y, d = halo_sqrt(dx * dx + dy * dy);
      if (d == 0) {
        hold.still++;
      } else {
        if (hold.still >= HOLD_REPORTS && d > HOLD_JUMP) {  // отпустил после остановки: отдать заново, с тем, что не отдано
          const float k = (d - HOLD_KEEP) / d;
          float ox, oy;
          hold_offset(p.t_us, ox, oy);
          hold.prev = hold.cur, hold.cur = {true, ox + dx * k, oy + dy * k, p.t_us};
        }
        hold.still = 0;
      }
    }
    hold.last = p, hold.have = true;
  }
}

// Где палец: искусственный (по часам анимаций) или настоящий (по отчётам тача, скорость — по последним 60 мс)
static bool finger_now(float *x, float *y, float *vx, float *vy) {
  if (syn_req) {
    // Прошлый жест ещё идёт — сначала кадр «палец поднят», как у настоящего пальца: иначе новый жест — продолжение
    // старого (палец перепрыгивает, а хозяин жеста — прежний)
    if (syn.on) {
      syn.on = false;
      return false;
    }
    syn = {true, syn_lin_req, 0, syn_arg[4], syn_arg[0], syn_arg[1], syn_arg[2], syn_arg[3]};
    syn_req = false;
    syn_us = halo_us();
  }
  if (syn.on) {
    const uint32_t now = halo_us();
    syn.t += (int32_t)(now - syn_us) / 1e6f;
    syn_us = now;
    if (syn.t > syn.dur) {
      syn.on = false;
      return false;
    }
    const float u = syn.t / syn.dur, e = syn.lin ? u : u * u * (3 - 2 * u);  // ровно или с разгоном и торможением
    *x = syn.x0 + (syn.x1 - syn.x0) * e, *y = syn.y0 + (syn.y1 - syn.y0) * e;
    const float de = syn.lin ? 1 / syn.dur : 6 * u * (1 - u) / syn.dur;
    *vx = (syn.x1 - syn.x0) * de, *vy = (syn.y1 - syn.y0) * de;
    return true;
  }
  HaloTouchSample raw[8];
  const int n = halo_touch_samples(raw, 8);
  hold_track(raw, n);
  if (!n) return false;
  struct {
    float x, y;
    uint32_t t_us;
  } s[8];  // отчёты без скачка после остановки (hold_track): по ним — и место, и скорость
  for (int i = 0; i < n; i++) {
    float ox, oy;
    hold_offset(raw[i].t_us, ox, oy);
    s[i] = {raw[i].x - ox, raw[i].y - oy, raw[i].t_us};
  }
  int k = 0;
  while (k + 1 < n && s[0].t_us - s[k + 1].t_us <= 60000) k++;
  if (k) {
    const float span = (int32_t)(s[0].t_us - s[k].t_us) / 1e6f;
    *vx = (s[0].x - s[k].x) / span, *vy = (s[0].y - s[k].y) / span;
  } else {
    *vx = *vy = 0;
  }
  // Где палец был в момент hf_finger_t — он на одном расстоянии до показа кадра (движок, halo_frame.cpp). Отчёты тача
  // идут ~100 раз в секунду не в такт экрану; раньше брали последний и дотягивали по скорости до «сейчас» (не больше
  // 8 мс) — страница погоды под пальцем шла неровными шагами: 6 px ± 2,7. Теперь, как в Android: между двумя
  // отчётами — по прямой, после последнего — дотянуть по скорости (не больше 8 мс): ± 0,5 px, палец на 9 мс позже
  const uint32_t ft = hf_finger_t;
  int j = 0;  // s[j] — первый отчёт не позже ft (или самый старый)
  while (j + 1 < n && (int32_t)(s[j].t_us - ft) > 0) j++;
  if (j == 0) {
    const float ahead = fminf((int32_t)(ft - s[0].t_us) / 1e6f, 0.008f);
    *x = s[0].x + *vx * ahead, *y = s[0].y + *vy * ahead;
  } else if ((int32_t)(s[j].t_us - ft) > 0) {  // палец коснулся позже ft — первое касание как есть
    *x = s[j].x, *y = s[j].y;
  } else {
    const float u = (float)(int32_t)(ft - s[j].t_us) / (float)(int32_t)(s[j - 1].t_us - s[j].t_us);
    *x = s[j].x + (s[j - 1].x - s[j].x) * u, *y = s[j].y + (s[j - 1].y - s[j].y) * u;
  }
  if (halo_trace_on) halo_trace_put(TR_FINGER, ft, (uint16_t)constrain(*x * 64, 0.0f, 65535.0f), (uint16_t)constrain((int)*y, 0, 511));
  return true;
}

static volatile bool touch_on = true;
static bool raw_was, touch_blocked;
static bool swallow;  // касание разбудило плату (halo_power.cpp): не нажатие, пока палец не отпустят

void hu_swallow_touch() {
  swallow = true;
}

void halo_touch_on(bool on) {
  touch_on = on;
}

bool halo_touch_is_on() {
  return touch_on;
}

bool halo_finger_blocked() {
  const bool b = touch_blocked;
  touch_blocked = false;
  return b;
}

void halo_finger_update() {
  HaloFinger &f = halo_finger;
  float x, y, vx, vy;
  // Погашенный экран и выключенные касания (блокировка) пальца не видят; касание при выключенных — отметить
  const bool raw = finger_now(&x, &y, &vx, &vy);
  if (!raw) swallow = false;
  const bool down = raw && !swallow && halo_display_is_on() && touch_on;
  if (raw && !raw_was && !touch_on) touch_blocked = true;
  raw_was = raw;
  const bool was = f.down;
  f.pressed = down && !was;
  f.released = !down && was;
  f.tap = false;
  f.drag_start = false;
  if (f.pressed) {
    f.x0 = x, f.y0 = y, f.t0 = halo_ms();
    f.dragging = false, f.owner = nullptr;
    f.edge = y < 46 ? HALO_EDGE_TOP : y > HALO_H - 46 ? HALO_EDGE_BOTTOM : x < 36 ? HALO_EDGE_LEFT
             : x > HALO_W - 36 ? HALO_EDGE_RIGHT : HALO_EDGE_NONE;
  }
  if (down) {
    f.x = x, f.y = y, f.vx = vx, f.vy = vy;
    if (!f.dragging && (fabsf(x - f.x0) > 10 || fabsf(y - f.y0) > 10)) {
      f.dragging = f.drag_start = true;
      f.horiz = fabsf(x - f.x0) > fabsf(y - f.y0);
    }
  }
  if (f.released && !f.dragging && halo_ms() - f.t0 < 400) f.tap = true;
  if (!down && !f.released) f.owner = nullptr;
  f.down = down;
}

static void finger_commands() {
  halo_command("tap", [](const char *a) {
    float x = HALO_W / 2, y = HALO_H / 2;
    sscanf(a, "%f %f", &x, &y);
    syn_start(x, y, x, y, 0.08f);
  });
  halo_command("press", [](const char *a) {
    float x = HALO_W / 2, y = HALO_H / 2, ms = 800;
    sscanf(a, "%f %f %f", &x, &y, &ms);
    syn_start(x, y, x, y, ms / 1000);
  });
  halo_command("drag", [](const char *a) {
    float x0, y0, x1, y1, ms = 280;
    if (sscanf(a, "%f %f %f %f %f", &x0, &y0, &x1, &y1, &ms) < 4) return (void)Serial.println("drag X0 Y0 X1 Y1 [мс]");
    syn_start(x0, y0, x1, y1, ms / 1000);
  });
  // Бросок: палец идёт с постоянной скоростью и отпускает на ходу — как настоящий свайп (у drag к концу палец
  // тормозит и отпускает почти без скорости). fling X Y СКОРОСТЬ_PX_С [ПУТЬ_PX] [up|down|left|right]
  halo_command("fling", [](const char *a) {
    float x = HALO_W / 2, y = HALO_H / 2, v = 1000, len = 120;
    char dir[8] = "down";
    if (sscanf(a, "%f %f %f %f %7s", &x, &y, &v, &len, dir) < 3) return (void)Serial.println("fling X Y px/s [путь] [up|down|left|right]");
    const float dx = !strcmp(dir, "left") ? -len : !strcmp(dir, "right") ? len : 0;
    const float dy = !strcmp(dir, "up") ? -len : !strcmp(dir, "down") ? len : 0;
    syn_start(x, y, x + dx, y + dy, len / fmaxf(v, 1), true);
  });
  halo_command("swipe", [](const char *a) {
    const bool edge = strstr(a, "edge");
    float x0 = HALO_W / 2, y0 = HALO_H / 2, x1 = HALO_W / 2, y1 = HALO_H / 2;
    if (strstr(a, "up")) y0 = edge ? 456 : 360, y1 = edge ? 200 : 110;
    else if (strstr(a, "down")) y0 = edge ? 10 : 110, y1 = edge ? 300 : 360;
    else if (strstr(a, "left")) x0 = edge ? 456 : 360, x1 = 110;
    else if (strstr(a, "right")) x0 = edge ? 10 : 110, x1 = 360;  // от левого края — жест назад
    else return (void)Serial.println("swipe up|down|left|right [edge]");
    syn_start(x0, y0, x1, y1, 0.28f);
  });
}

// ---------- Элементы ----------
static inline uint32_t mix(uint32_t h, uint32_t v) {
  return (h ^ v) * 16777619u;
}
static inline uint32_t qf(float v, float k) {
  return (uint32_t)(int32_t)(v * k + (v >= 0 ? 0.5f : -0.5f));
}

static void bounds(const HaloItem &it, float b[4]) {
  const float s = it.scale;
  float hw = 0, hh = 0, cx = it.x, cy = it.y;
  switch (it.kind) {
    case HI_TEXT: {
      const float tw = (it.font && it.text ? halo_text_width(*it.font, it.text, it.spacing) : 0) * s;
      hw = tw / 2 + 2, hh = (it.font ? it.font->line : 0) * s / 2 + 2;
      cx = it.align == 0 ? it.x + tw / 2 : it.align == 2 ? it.x - tw / 2 : it.x;
      break;
    }
    case HI_MASK:
      if (it.mask) hw = it.mask->w * s / 2 + 1, hh = it.mask->h * s / 2 + 1;
      break;
    case HI_IMAGE:
      if (it.image) hw = it.image->w * s / 2 + 1, hh = it.image->h * s / 2 + 1;
      break;
    case HI_RRECT:
      hw = it.w * s / 2 + 1, hh = it.h * s / 2 + 1;
      break;
    case HI_CIRCLE:
      hw = hh = it.r * s + 1;
      break;
    case HI_RING:
      hw = hh = (it.r + it.w / 2) * s + 1;
      break;
    case HI_PIXELS:  // углы повёрнутой картинки с хвостом; искорки по всему экрану — весь экран
      if (it.pixels) {
        const HaloPixels &p = *it.pixels;
        if (it.tile) {
          b[0] = b[1] = 0, b[2] = HALO_W, b[3] = HALO_H;
          return;
        }
        const float c = cosf(it.a0) * s, n = sinf(it.a0) * s, t = p.tp > 0 ? max(0, (int)it.trail) : 0;
        b[0] = b[1] = 1e9f, b[2] = b[3] = -1e9f;
        for (int k = 0; k < 4; k++) {
          const float u = (k & 1 ? p.w : -t) - p.ax, v = (k & 2 ? p.h : 0) - p.ay;
          const float X = it.x + u * c - v * n, Y = it.y + u * n + v * c;
          b[0] = fminf(b[0], X), b[1] = fminf(b[1], Y), b[2] = fmaxf(b[2], X), b[3] = fmaxf(b[3], Y);
        }
        b[0] -= 1, b[1] -= 1, b[2] += 1, b[3] += 1;
        return;
      }
      break;
    case HI_EDGE:  // от верха кнопки до низа круга, по ширине — верх
      b[0] = it.x - it.w * s / 2 - 1, b[1] = it.y + (it.r - it.h) * s - 1;
      b[2] = it.x + it.w * s / 2 + 1, b[3] = it.y + it.r * s + 1;
      return;
    case HI_ARC: {  // средняя линия дуги (концы и точки 12, 3, 6, 9 часов внутри неё) + половина толщины
      const float r = it.r * s, e = it.w * s / 2 + 1;
      float x0 = fminf(sinf(it.a0), sinf(it.a1)), x1 = fmaxf(sinf(it.a0), sinf(it.a1));
      float y0 = fminf(-cosf(it.a0), -cosf(it.a1)), y1 = fmaxf(-cosf(it.a0), -cosf(it.a1));
      for (int k = 0; k < 4; k++) {
        float ak = k * (HALO_PI / 2);
        while (ak < it.a0) ak += 2 * HALO_PI;
        if (ak > it.a1) continue;
        if (k == 0) y0 = -1;
        if (k == 1) x1 = 1;
        if (k == 2) y1 = 1;
        if (k == 3) x0 = -1;
      }
      b[0] = it.x + x0 * r - e, b[1] = it.y + y0 * r - e, b[2] = it.x + x1 * r + e, b[3] = it.y + y1 * r + e;
      return;
    }
  }
  b[0] = cx - hw, b[1] = cy - hh, b[2] = cx + hw, b[3] = cy + hh;
}

// Пиксель-арт: сменился только кадр — перерисовать клетки, которые в новом кадре другие (подряд в строке — одной
// областью: её повёрнутая рамка). Обычно меняется несколько процентов клеток — и столько же экрана
static void pixels_damage(const HaloItem &it) {
  const HaloPixels &p = *it.pixels;
  const int rb = (p.w + 1) / 2, hide = it.hide | 1, trail = p.tp > 0 ? max(0, (int)it.trail) : 0;
  const uint8_t *A = p.cells + (it.drawn % p.frames) * rb * p.h, *B = p.cells + (it.frame % p.frames) * rb * p.h;
  const float c = cosf(it.a0) * it.scale, n = sinf(it.a0) * it.scale;
  auto cell = [&](const uint8_t *f, int j, int i) {
    const int b = f[j * rb + (i >> 1)], k = i & 1 ? b & 15 : b >> 4;
    return hide >> k & 1 ? 0 : k;
  };
  // Клетки [u0, u1) × [v0, v1) — их повёрнутый прямоугольник; за экраном — ничего
  auto rect = [&](int u0, int v0, int u1, int v1) {
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (int k = 0; k < 4; k++) {
      const float u = (k & 1 ? u1 : u0) - p.ax, v = (k >> 1 ? v1 : v0) - p.ay;
      const float X = it.x + u * c - v * n, Y = it.y + u * n + v * c;
      x0 = fminf(x0, X), y0 = fminf(y0, Y), x1 = fmaxf(x1, X), y1 = fmaxf(y1, Y);
    }
    if (x1 >= 0 && y1 >= 0 && x0 < HALO_W && y0 < HALO_H) halo_damage_f(x0, y0, x1, y1);
  };
  // Искорки по всему экрану: копии картинки, которые его касаются (углы экрана — назад в клетки)
  int ta0 = 0, ta1 = 0, tb0 = 0, tb1 = 0;
  if (it.tile) {
    const float s = fmaxf(it.scale, 1e-3f), cs = cosf(it.a0) / s, sn = sinf(it.a0) / s;
    float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f;
    for (int k = 0; k < 4; k++) {
      const float X = (k & 1 ? HALO_W : 0) - it.x, Y = (k & 2 ? HALO_H : 0) - it.y;
      const float u = X * cs + Y * sn + p.ax, v = Y * cs - X * sn + p.ay;
      u0 = fminf(u0, u), u1 = fmaxf(u1, u), v0 = fminf(v0, v), v1 = fmaxf(v1, v);
    }
    tb0 = halo_floor(v0 / p.h), tb1 = halo_floor(v1 / p.h);
    const int st = halo_pixels_stagger(p), smin = min(tb0 * st, tb1 * st), smax = max(tb0 * st, tb1 * st);
    ta0 = halo_floor((u0 + smin) / p.w), ta1 = halo_floor((u1 + smax) / p.w) + 1;  // (с запасом на сдвиг рядов)
  }
  const int half = p.h / 2, st = halo_pixels_stagger(p);  // (ряд копий tb — сдвинут на tb·st клеток влево)
  for (int j = 0; j < p.h; j++) {
    if (!memcmp(A + j * rb, B + j * rb, rb)) continue;
    bool body = false, spark = false;  // что сменилось в отрезке: сюжет, искорки
    for (int i = 0, i0 = -1; i <= p.w; i++) {
      const int a = i < p.w ? cell(A, j, i) : 0, b = i < p.w ? cell(B, j, i) : 0;
      const bool ch = a != b;
      if (ch) {
        if (i0 < 0) i0 = i, body = spark = false;
        body |= (a && !(p.apart >> a & 1)) || (b && !(p.apart >> b & 1));
        spark |= (p.apart >> a & 1) || (p.apart >> b & 1);
      }
      if (ch || i0 < 0) continue;
      rect(i0, j, i, j + 1);
      // хвост: столбцы [0, tp) — ещё и левее картинки, через каждые tp
      if (body && trail && i0 < p.tp)
        for (int k = 1; i0 - k * p.tp < 0 && min(i, (int)p.tp) - k * p.tp > -trail; k++)
          rect(max(i0 - k * p.tp, -trail), j, min(i, (int)p.tp) - k * p.tp, j + 1);
      // искорки во всех копиях на экране: на своих местах, а на месте сюжета — полвысотой выше
      if (spark && it.tile) {
        const int j2 = (j - half + p.h) % p.h, b0 = max(i0, (int)p.bx0), b1 = min(i, (int)p.bx1);
        const bool moved = j2 >= p.by0 && j2 < p.by1 && b0 < b1;
        for (int tb = tb0; tb <= tb1; tb++)
          for (int ta = ta0; ta <= ta1; ta++) {
            if (!ta && !tb) continue;
            const int du = ta * p.w - tb * st;
            rect(i0 + du, j + tb * p.h, i + du, j + 1 + tb * p.h);
            if (moved) rect(b0 + du, j2 + tb * p.h, b1 + du, j2 + 1 + tb * p.h);
          }
      }
      i0 = -1;
    }
  }
}

void halo_item_commit(HaloItem &it) {
  const bool vis = it.alpha > 0.004f;
  uint32_t h = 2166136261u;
  if (vis) {
    h = mix(h, it.kind), h = mix(h, qf(it.x, 16)), h = mix(h, qf(it.y, 16)), h = mix(h, qf(it.scale, 2048));
    h = mix(h, qf(it.alpha, 512)), h = mix(h, it.color), h = mix(h, it.color2), h = mix(h, it.align);
    h = mix(h, qf(it.w, 16)), h = mix(h, qf(it.h, 16)), h = mix(h, qf(it.r, 16)), h = mix(h, qf(it.v, 4096));
    h = mix(h, qf(it.a0, 4096)), h = mix(h, qf(it.a1, 4096)), h = mix(h, it.on_black);
    h = mix(h, (uint32_t)(uintptr_t)it.on);
    h = mix(h, (uint32_t)(uintptr_t)it.font), h = mix(h, (uint32_t)(uintptr_t)it.mask);
    // Маска, сделанная на ходу (halo_text_arc): новая — новые пиксели по новому адресу
    if (it.mask) h = mix(h, (uint32_t)(uintptr_t)it.mask->bits), h = mix(h, it.mask->w), h = mix(h, it.mask->h);
    h = mix(h, (uint32_t)(uintptr_t)it.image), h = mix(h, (uint32_t)(uintptr_t)it.pixels), h = mix(h, it.hide);
    if (it.pixels) h = mix(h, (uint16_t)it.trail), h = mix(h, it.tile);
    if (it.text)
      for (const char *p = it.text; *p; p++) h = mix(h, (uint8_t)*p);
  }
  if (vis == it.shown && (!vis || h == it.hash)) {  // ничего не изменилось — ничего не рисуем
    if (vis && it.pixels && it.frame != it.drawn) pixels_damage(it), it.drawn = it.frame;  // (кроме кадра пиксель-арта)
    return;
  }
  if (it.shown) halo_damage_f(it.b[0], it.b[1], it.b[2], it.b[3]);  // старое место
  it.shown = vis, it.hash = h, it.drawn = it.frame;
  if (!vis) return;
  bounds(it, it.b);
  halo_damage_f(it.b[0], it.b[1], it.b[2], it.b[3]);  // новое место
}

volatile bool hp_on;
uint32_t hp_cyc[2][HP_PARTS];

const HaloItem halo_black{};

// Сплошной цвет под текстом или маской (it.on), −1 — не сплошной: смешивать
static int32_t under(const HaloItem &it) {
  const HaloItem *o = it.on;
  if (!o || (it.kind != HI_TEXT && it.kind != HI_MASK)) return -1;
  if (o == &halo_black) return 0;
  if (!o->shown || !o->on_black) return -1;
  return halo_dim(o->color, (int)(o->alpha * 256 + 0.5f)) & 0xFFFFFF;
}

static void item_draw(const HaloItem &it, int32_t bg);

// Круг, по которому режутся элементы (cx, cy, r; r 0 — не режутся): верхний экран в переходе навигации. По ядрам —
// каждое рисует свою часть пачки и ставит его само
static float item_clip[2][3];

void hu_item_clip(float cx, float cy, float r) {
  float *c = item_clip[xPortGetCoreID()];
  c[0] = cx, c[1] = cy, c[2] = r;
}

// Элемент целиком в круге — false: рисовать как обычно. Целиком снаружи — не рисовать. Задевает край — каждая строка
// только между краями круга (пиксели с центром внутри); соседние строки, у которых края по ширине элемента те же, —
// одним вызовом
static bool clip_draw(const HaloItem &it, float *c) {
  const float cx = c[0], cy = c[1], r = c[2], r2 = r * r;
  if (it.kind == HI_CIRCLE || it.kind == HI_RING) {  // круг — по радиусу, не по углам рамки (диск страницы — во весь круг)
    const float dx = it.x - cx, dy = it.y - cy;
    const float ir = (it.kind == HI_RING ? it.r + it.w / 2 : it.r) * it.scale;
    if (halo_sqrt(dx * dx + dy * dy) + ir <= r + 1) return false;
  }
  const float fx = fmaxf(fabsf(it.b[0] - cx), fabsf(it.b[2] - cx)), fy = fmaxf(fabsf(it.b[1] - cy), fabsf(it.b[3] - cy));
  if (fx * fx + fy * fy <= r2) return false;
  const float nx = fmaxf(0, fmaxf(it.b[0] - cx, cx - it.b[2])), ny = fmaxf(0, fmaxf(it.b[1] - cy, cy - it.b[3]));
  if (nx * nx + ny * ny >= r2) return true;
  const HaloBand band = hb;
  const int y0 = max(band.y, halo_floor(it.b[1])), y1 = min(band.y + band.rows - 1, halo_floor(it.b[3]));
  const int ix0 = halo_floor(it.b[0]) - 1, ix1 = halo_floor(it.b[2]) + 1;  // где элемент может рисовать
  c[2] = 0;
  int run_y = y0, run_x0 = 0, run_x1 = -1;  // строки подряд с одними краями (x1 < x0 — пусто)
  for (int y = y0; y <= y1 + 1; y++) {
    int x0 = 0, x1 = -1;
    if (y <= y1) {
      const float dy = y + 0.5f - cy;
      if (dy * dy < r2) {
        const float h = halo_sqrt(r2 - dy * dy);
        x0 = max(max(band.x0, ix0), -halo_floor(h + 0.5f - cx)), x1 = min(min(band.x1, ix1), halo_floor(cx + h - 0.5f));
        if (x0 > x1) x0 = 0, x1 = -1;
      }
      if (x0 == run_x0 && x1 == run_x1) continue;
    }
    if (run_x0 <= run_x1) {
      hb.buf = band.buf + (run_y - band.y) * band.w, hb.y = run_y, hb.rows = y - run_y;
      hb.x0 = run_x0, hb.x1 = run_x1;
      halo_item_draw(it);
    }
    run_y = y, run_x0 = x0, run_x1 = x1;
  }
  c[2] = r, hb = band;
  return true;
}

void halo_item_draw(const HaloItem &it) {
  if (!it.shown || !halo_hits(it.b[0], it.b[1], it.b[2], it.b[3])) return;
  float *c = item_clip[xPortGetCoreID()];
  if (c[2] > 0 && clip_draw(it, c)) return;
  if (!hp_on) return item_draw(it, under(it));
  const uint32_t c0 = esp_cpu_get_cycle_count();
  item_draw(it, under(it));
  hp_cyc[xPortGetCoreID()][it.kind] += esp_cpu_get_cycle_count() - c0;
}

static void item_draw(const HaloItem &it, int32_t bg) {
  const float s = it.scale;
  if (it.on_black && it.alpha < 0.999f) {  // на чёрном: тусклый цвет вместо смешивания
    HaloItem d = it;
    const int k = (int)(it.alpha * 256 + 0.5f);
    d.color = halo_dim(it.color, k), d.color2 = halo_dim(it.color2, k), d.alpha = 1, d.on_black = false;
    if (it.kind != HI_IMAGE && it.kind != HI_PIXELS) return item_draw(d, bg);  // (у них цвета свои — гасят сами)
  }
  switch (it.kind) {
    case HI_TEXT:
      if (it.font && it.text)
        halo_text(*it.font, it.text, it.x, it.y - it.font->line * s / 2, it.color, it.alpha, it.align, s, it.spacing, bg);
      break;
    case HI_MASK:
      if (it.mask) halo_mask(*it.mask, it.x - it.mask->w * s / 2, it.y - it.mask->h * s / 2, it.color, it.alpha, s, bg);
      break;
    case HI_IMAGE:
      if (it.image) halo_image(*it.image, it.x - it.image->w * s / 2, it.y - it.image->h * s / 2, it.alpha, s);
      break;
    case HI_RRECT:
      halo_rrect(it.x - it.w * s / 2, it.y - it.h * s / 2, it.x + it.w * s / 2, it.y + it.h * s / 2, it.r * s, it.color,
                 it.alpha);
      break;
    case HI_CIRCLE:
      halo_circle(it.x, it.y, it.r * s, it.color, it.alpha);
      break;
    case HI_RING:
      halo_ring(it.x, it.y, it.r * s, it.w * s, it.v, it.color, it.color2, it.alpha);
      break;
    case HI_ARC:
      halo_arc(it.x, it.y, it.r * s, it.w * s / 2, it.a0, it.a1, it.color, it.alpha);
      break;
    case HI_EDGE:
      halo_edge(it.x, it.y, it.r * s, it.y + (it.r - it.h) * s, it.w * s / 2, it.v * s, 20 * s, 24 * s, it.color,
                it.alpha);
      break;
    case HI_PIXELS:
      if (it.pixels) halo_pixels(*it.pixels, it.drawn, it.x, it.y, s, it.a0, it.alpha, it.hide, it.on_black, it.trail, it.tile);
      break;
  }
}

// ---------- Страницы ----------
static HaloSpringK K_PAGER;

void halo_pager_begin(HaloPager &p, int count) {
  p.count = constrain(count, 1, 8);
  p.page = 0;
  halo_spring_set(p.off, 0);
  for (int i = 0; i < p.count; i++) p.dots[i].kind = HI_CIRCLE;
}

void halo_pager_go(HaloPager &p, int page) {
  p.page = constrain(page, 0, p.count - 1);
  p.off.target = p.page;
}

void halo_pager_update(HaloPager &p, float dt, bool enabled) {
  HaloFinger &f = halo_finger;
  // Горизонтальный жест не от верхнего и нижнего края — наш: страницы едут за пальцем (у крайних — с «резинкой»)
  if (enabled && f.down && f.dragging && f.horiz && f.edge != HALO_EDGE_TOP && f.edge != HALO_EDGE_BOTTOM &&
      halo_finger_claim(&p)) {
    p.drag = true;
    float o = p.page - (f.x - f.x0) / HALO_W;
    if (o < 0) o = -0.25f * (1 - 1 / (1 - o * 2));  // резинка: чем дальше, тем туже
    if (o > p.count - 1) o = p.count - 1 + 0.25f * (1 - 1 / (1 + (o - p.count + 1) * 2));
    p.off.x = o, p.off.target = o, p.off.v = -f.vx / HALO_W;
  } else if (p.drag) {  // отпустили: дальше по скорости или если ушли больше чем на треть
    p.drag = false;
    const float moved = p.off.x - p.page, v = p.off.v;
    int to = p.page;
    if (v > 0.6f || (moved > 0.33f && v > -0.6f)) to++;
    else if (v < -0.6f || (moved < -0.33f && v < 0.6f)) to--;
    halo_pager_go(p, to);
  }
  if (!p.drag) halo_spring_run(p.off, K_PAGER, dt);
  // Точки: текущая — белая и чуть больше, остальные — серые; между страницами — плавно
  const float gap = 14, x0 = HALO_W / 2 - (p.count - 1) * gap / 2;
  for (int i = 0; i < p.count; i++) {
    HaloItem &d = p.dots[i];
    const float near = halo_clamp01(1 - fabsf(p.off.x - i));
    d.x = x0 + i * gap, d.y = 448, d.r = 3.5f + 1.0f * near;
    const int k = (int)(90 + 165 * near);
    d.color = (k << 16) | (k << 8) | k;
    halo_item_commit(d);
  }
}

void halo_pager_draw(const HaloPager &p) {
  for (int i = 0; i < p.count; i++) halo_item_draw(p.dots[i]);
}

void hu_begin() {
  K_PAGER = halo_spring(0.1405f, 1.0f);  // доводка страниц (Horologist, жёсткость 2000)
  finger_commands();
  halo_trace_name(TR_FINGER, "finger", 'i');  // настоящий палец в кадре: время — момент, x × 64, y (trace.py сверяет)
}
