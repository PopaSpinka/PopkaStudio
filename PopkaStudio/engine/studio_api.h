// studio_api.h — what the editor's page calls in the browser build of PopkaWatch (engine/build.sh includes it right
// after the sketch, so it sees the sketch's own screens: face_scr, face_scr_n in face.h).
//
// The page holds the project; on every change it sends all screens and their layers (studio_set). Fonts and icons come
// from studio.py, made by tools/halo_assets.py exactly as for the board, packed as HFNT / HIMS (studio.py: font_blob,
// icons_blob); the page hands them in (studio_font, studio_icons) and refers to them by the slot number it got back.
//
// One copy of the engine per screen on the editor's map: studio_pin(s) makes it show screen s at rest, without the
// navigation (as the navigation shows it once its transition is over: through the plain view). The copy for Try is
// not pinned: it runs the sketch as the board does — the face, a tap on a layer with a link, the transition, back.
// The page reads where each layer was drawn (studio_boxes), HaloWatch's place of each layer — what its dx, dy are
// counted from (studio_places: the Weather screen's icon and high/low stand by the temperature) — and of each kind
// (studio_bases: for a layer the engine has not drawn yet). The gallery: which screen a swipe up on the face opens
// (studio_main), and a pinned list scrolled to show a layer (studio_scroll). Try starts on the editor's screen: the
// page taps its way there (studio_tap).
#include <ArduinoJson.h>
#include <emscripten.h>

#include <vector>

#define ST_SCREENS 24  // room for screens and layers (studio.py: MAX_SCREENS, MAX_LAYERS)
#define ST_LAYERS 32
static FaceLayer st_layers[ST_SCREENS][ST_LAYERS];
static FaceRun st_run[ST_SCREENS][ST_LAYERS];
static FaceScreen st_screens[ST_SCREENS];
static std::vector<const HaloFont *> st_fonts;
static std::vector<const HaloImage *const *> st_icons;
static char st_text[ST_SCREENS][ST_LAYERS][3][40];  // the gallery's text: the title, the line under it, the icon
static char st_name[ST_SCREENS][40];                // the screens' names (an empty one shows its name)
static float st_boxes[ST_LAYERS * 4], st_places[ST_LAYERS * 2], st_bases[FL_SCLOCK * 2 + 2], st_radii[ST_LAYERS * 2];
static int st_pin = -1, st_page = 0;

void web_loop(HaloUpdate update, HaloDraw draw);  // halo_web.cpp

struct Reader {
  const uint8_t *p, *end;
  bool ok = true;
  template <class T> T get() {
    T v{};
    if (p + sizeof(T) > end) return ok = false, v;
    memcpy(&v, p, sizeof(T));
    p += sizeof(T);
    return v;
  }
  template <class T> T *copy(size_t n) {  // n items into their own memory (the blob is freed by the page)
    T *dst = (T *)malloc(n * sizeof(T) + 1);
    if (p + n * sizeof(T) > end) return ok = false, dst;
    memcpy(dst, p, n * sizeof(T));
    p += n * sizeof(T);
    return dst;
  }
};

// Pinned: screen st_pin at rest, on its page st_page (a screen with pages: the Weather screen's cities) — its layers
// through the plain view; no finger, no transitions
static void pinned_update(float dt) {
  settings_tick(dt);  // (as the board's update: Battery's words along the arc are made there)
  face_tick(dt);
#if HAS_SHADE
  shade_anim(dt);  // (the home's panel, shown while its layer is selected: the command shade open)
#endif
  if (st_pin >= 0 && st_pin < face_scr_n) {  // (its own count: the Weather screen — the cities, the Steps screen — 2)
    const int pg = min(st_page, screen_pages(face_scr[st_pin]) - 1);
    if (face_scr[st_pin].pager.x != pg) halo_spring_set(face_scr[st_pin].pager, pg);
  }
  screen_layout(st_pin, HaloView());
}
static void pinned_draw() {
  screen_draw(st_pin);
}

extern "C" {

// A font (HFNT) → its slot, -1 — not a font
EMSCRIPTEN_KEEPALIVE int studio_font(const uint8_t *blob, int len) {
  Reader r{blob, blob + len};
  if (r.get<uint32_t>() != 0x544E4648) return -1;  // "HFNT"
  HaloFont *f = new HaloFont();
  f->size = r.get<int16_t>(), f->line = r.get<int16_t>(), f->ascent = r.get<int16_t>(), f->count = r.get<uint16_t>();
  const uint32_t nbits = r.get<uint32_t>(), next = r.get<uint32_t>();
  f->codes = r.copy<uint32_t>(f->count);
  struct G {
    int16_t w, h, ox, oy, adv, pad;
    uint32_t bo, eo;
  };
  const G *g = r.copy<G>(f->count);
  const uint8_t *bits = r.copy<uint8_t>(nbits), *ext = r.copy<uint8_t>(next);
  if (!r.ok) return -1;
  HaloMask *m = new HaloMask[f->count];
  for (int i = 0; i < f->count; i++) m[i] = {g[i].w, g[i].h, g[i].ox, g[i].oy, g[i].adv, bits + g[i].bo, ext + g[i].eo};
  f->glyphs = m;
  st_fonts.push_back(f);
  return (int)st_fonts.size() - 1;
}

// Four weather icons (HIMS: the face's pill, the Weather screen's big ones or the hourly pill's), by WX_PARTLY ... →
// their slot, -1 — not icons
EMSCRIPTEN_KEEPALIVE int studio_icons(const uint8_t *blob, int len) {
  Reader r{blob, blob + len};
  if (r.get<uint32_t>() != 0x534D4948 || r.get<uint32_t>() != 4) return -1;  // "HIMS", 4 images
  const HaloImage **set = new const HaloImage *[4];
  for (int i = 0; i < 4; i++) {
    HaloImage *im = new HaloImage();
    im->w = r.get<int16_t>(), im->h = r.get<int16_t>();
    const uint32_t npx = r.get<uint32_t>(), nal = r.get<uint32_t>(), next = r.get<uint32_t>();
    im->px = r.copy<uint16_t>(npx), im->alpha = r.copy<uint8_t>(nal), im->ext = r.copy<uint8_t>(next);
    r.p += (4 - (16 + npx * 2 + nal + next) % 4) % 4;  // each image is padded to 4 bytes
    set[i] = im;
  }
  if (!r.ok) return -1;
  st_icons.push_back(set);
  return (int)st_icons.size() - 1;
}

// All screens, bottom to top layers: [[{"kind": FL_*, "dx", "dy", "k", "c", "c2", "sp", "font": slot, "icons": slot,
// "tap": screen or -1, "fx", "p", "hidden", "t", "u", "i" (the gallery: the title, the line under it, the icon), "c3",
// "font2": slot (steps: the ring, the shoe), "font3", "font4" (a clock of sclock.h)}, ...],
// ...]; the first — the watch face. A layer whose kind, font, spacing, size, radius or text changed is made anew (its
// letters in place); the lists are made from the layers again (kept where they were if the same layers make them);
// the screen is drawn anew.
EMSCRIPTEN_KEEPALIVE int studio_set(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return -1;
  JsonArray all = doc.as<JsonArray>();
  const bool first = face_scr != st_screens;
  const int ns = max(1, min((int)all.size(), ST_SCREENS));
  for (int s = 0; s < ns; s++) {
    JsonVariant sv = all[s];  // {"n": its name, "l": its layers} (or just the layers)
    JsonArray a = sv.is<JsonArray>() ? sv.as<JsonArray>() : sv["l"].as<JsonArray>();
    FaceScreen &S = st_screens[s];
    const char *name = sv.is<JsonObject>() ? (sv["n"] | "") : "";
    if (strncmp(st_name[s], name, sizeof(st_name[0]) - 1)) S.name_ok = false;  // (measured anew)
    strncpy(st_name[s], name, sizeof(st_name[0]) - 1);
    S.name = st_name[s];
    const int n = min((int)a.size(), ST_LAYERS), was = first || s >= face_scr_n ? 0 : S.n;
    S.layers = st_layers[s], S.run = st_run[s];
    for (int i = 0; i < n; i++) {
      JsonObject o = a[i];
      FaceLayer L{};
      L.kind = o["kind"] | 0;
      L.dx = o["dx"] | 0.0f, L.dy = o["dy"] | 0.0f, L.k = o["k"] | 1.0f;
      L.color = o["c"] | 0xFFFFFF, L.color2 = o["c2"] | 0xFFFFFF, L.spacing = o["sp"] | 0;
      const int fs = o["font"] | -1, is = o["icons"] | -1, f2 = o["font2"] | -1;
      L.font = fs >= 0 && fs < (int)st_fonts.size() ? st_fonts[fs] : nullptr;
      L.font2 = f2 >= 0 && f2 < (int)st_fonts.size() ? st_fonts[f2] : nullptr;
      const int f3 = o["font3"] | -1, f4 = o["font4"] | -1;  // (a clock of sclock.h: its third and fourth fonts)
      L.font3 = f3 >= 0 && f3 < (int)st_fonts.size() ? st_fonts[f3] : nullptr;
      L.font4 = f4 >= 0 && f4 < (int)st_fonts.size() ? st_fonts[f4] : nullptr;
      L.color3 = o["c3"] | 0xFFFFFF;
      L.p2 = o["p2"] | 0.0f, L.p3 = o["p3"] | 0.0f;
      L.icons = is >= 0 && is < (int)st_icons.size() ? st_icons[is] : nullptr;
      L.tap = o["tap"] | -1, L.fx = o["fx"] | (int)HALO_FX_FADE, L.p = o["p"] | 0.0f;
      L.hidden = o["hidden"] | false;
      bool texts = false;
      const char *in[3] = {o["t"] | "", o["u"] | "", o["i"] | ""};
      for (int k = 0; k < 3; k++) {
        char *buf = st_text[s][i][k];
        texts |= strncmp(buf, in[k], sizeof(st_text[0][0][0]) - 1) != 0;
        strncpy(buf, in[k], sizeof(st_text[0][0][0]) - 1);
      }
      L.text = st_text[s][i][0], L.sub = st_text[s][i][1], L.icon = st_text[s][i][2];
      const FaceLayer &old = S.layers[i];
      const bool fresh = i >= was || texts || face_layer_rebuild(old, L);
      S.layers[i] = L;
      if (fresh) face_layer_build(S, i);
    }
    S.n = n;
    gal_rebuild(S);
  }
  face_scr = st_screens, face_scr_n = ns;
  if (face_over >= ns) halo_nav_back();  // the screen open over the face is gone
  halo_damage_all();
  return ns;
}

// The Functions (power.h): Always On when idle (0/1), after idle s, its panel level (of 255); what BOOT and PWR do
// (FN_NOTHING, FN_SCREEN_OFF, FN_LOCK). A button that no longer locks — unlocked
EMSCRIPTEN_KEEPALIVE void studio_fn(int aod, int idle, int bright, int boot, int pwr, int tap, int raise, int btn,
                                    int home, int shift) {
  pw_aod_on = aod, pw_idle_s = idle, pw_aod_level = constrain(bright, 1, 255), pw_boot = boot, pw_pwr = pwr;
  pw_aod_home = home;    // (Back to watch face)
  pw_aod_shift = shift;  // (Burn-in shift)
  pw_wake_tap = tap, pw_wake_raise = raise, pw_wake_btn = btn;  // (Wake up)
  if (pw_locked && (pw_lock_btn == 1 ? boot : pwr) != FN_LOCK) pw_lock_set(false, 0), pw_wake(0);
}

// The clock (the editor's Time): 24 hours (1) or 12; shift — s the picked zone is ahead of the computer's
EMSCRIPTEN_KEEPALIVE void studio_clock(int h24, int shift) {
  fc_h24 = h24, st_time_shift = shift;
  fc_min = fc_hour = fc_yday = -1, wx_clock_h = -1, fc_check = true;  // the clock, the date and the arc — again
  const bool still = fc_still;
  fc_still = true, face_tick(0), fc_still = still;  // (at once, without rolling)
  wx_fill(false);
  halo_damage_all();
}

// Show screen s at rest (the map); -1 — run as the board does (Try)
// A pinned copy on page p of its screen (0 — the first; a screen without pages has only that)
static int st_page_asked;  // the page this copy is for (pinned_update holds it to its screen's pages)
EMSCRIPTEN_KEEPALIVE void studio_page(int p) {
  st_page_asked = p < 0 ? 0 : p;
  st_page = st_page_asked;
  halo_damage_all();
}
// How many pages a screen with the Weather screen's parts has, and the city of page p (weather.h)
EMSCRIPTEN_KEEPALIVE int studio_pages() {
  return wx_pages;
}
EMSCRIPTEN_KEEPALIVE const char *studio_city(int p) {
  return p >= 0 && p < wx_pages ? wx_city[p].name : "";
}

// The weather (the editor's cities and what studio.py got from Open-Meteo — the same the board gets):
// {"c": [{"n": name, "now", "feels", "hi", "lo", "k": kind, "t0", "hk": [kinds an hour]}]} — a city without numbers
// yet ("now" missing) shows "--"
EMSCRIPTEN_KEEPALIVE void studio_weather(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  JsonArrayConst cs = doc["c"].as<JsonArrayConst>();
  int n = 0;
  for (JsonObjectConst c : cs) {
    if (n == WX_MAX) break;
    WxCity &w = wx_city[n++];
    strlcpy(w.name, c["n"] | "", sizeof(w.name));
    w.now = c["now"] | (int)WX_NONE, w.feels = c["feels"] | (int)WX_NONE;
    w.hi = c["hi"] | (int)WX_NONE, w.lo = c["lo"] | (int)WX_NONE;
    w.kind = c["k"] | (int)WX_CLOUD, w.t0 = c["t0"] | 0u;
    JsonArrayConst hk = c["hk"].as<JsonArrayConst>(), ht = c["ht"].as<JsonArrayConst>();
    for (int j = 0; j < WX_AHEAD; j++) w.hk[j] = j < (int)hk.size() ? (uint8_t)(hk[j] | 0) : w.kind;
    for (int j = 0; j < WX_AHEAD; j++) w.ht[j] = j < (int)ht.size() && !ht[j].isNull() ? (int16_t)(ht[j] | 0) : WX_NONE;
  }
  wx_pages = n ? n : 1;
  st_page = st_page_asked;
  for (int k = 0; k < face_scr_n; k++)  // (fewer cities: a Weather screen on a page gone — on the last one)
    if (screen_paged(face_scr[k]) && face_scr[k].pager.x > wx_pages - 1) halo_spring_set(face_scr[k].pager, wx_pages - 1);
  face_weather(-1, false);
  halo_damage_all();
}

EMSCRIPTEN_KEEPALIVE void studio_pin(int s) {
  st_pin = s;
  if (s >= 0) web_loop(pinned_update, pinned_draw);
  halo_damage_all();
}

// Where each layer of screen s was drawn in the last frame: x0, y0, x1, y1 per layer (x1 < x0 — nowhere)
EMSCRIPTEN_KEEPALIVE float *studio_boxes(int s) {
  if (s >= 0 && s < face_scr_n)
    for (int i = 0; i < face_scr[s].n; i++) memcpy(st_boxes + i * 4, face_scr[s].run[i].box, sizeof(float) * 4);
  return st_boxes;
}

// How far from the screen's middle each layer of screen s reaches, its dx, dy left out: inner, outer per layer — curved
// text by its letters' pixels (a quarter lit and more), the rings by their stroke, the week and the forecast by their
// parts' boxes; NaN — not one of these, or not drawn yet. The editor snaps a curved layer's outer edge by it: right onto
// the screen's edge (nothing cut), or as far from it as another curved layer is
EMSCRIPTEN_KEEPALIVE float *studio_radii(int s) {
  for (int i = 0; i < ST_LAYERS * 2; i++) st_radii[i] = NAN;
  if (s < 0 || s >= face_scr_n) return st_radii;
  for (int i = 0; i < face_scr[s].n; i++) {
    const FaceLayer &L = face_scr[s].layers[i];
    const FaceRun &R = face_scr[s].run[i];
    float in = 1e9f, out = -1;
    if (L.kind == FL_CURVE && R.arc.bits) {
      const int stride = (R.arc.w + 1) / 2;
      for (int y = 0; y < R.arc.h; y++)
        for (int x = 0; x < R.arc.w; x++) {
          const uint8_t b = R.arc.bits[y * stride + x / 2], v = (x & 1) ? b & 15 : b >> 4;
          if (v < 4) continue;
          const float d = hypotf(R.tx + x + 0.5f - CX, R.ty + y + 0.5f - CX);
          in = fminf(in, d - 0.5f), out = fmaxf(out, d + 0.5f);
        }
    } else if (L.kind == FL_STEPRING || L.kind == FL_BATRING) {
      const float r = L.kind == FL_BATRING ? (L.p > 0 ? L.p : 224) : (L.p2 > 0 ? L.p2 : 224), w = 6 * L.k;
      in = r - w / 2, out = r + w / 2;
    } else if ((L.kind == FL_WEEKARC || L.kind == FL_FORECAST) && R.g) {
      for (int j = 0; j < (L.kind == FL_WEEKARC ? 8 : 15); j++) {
        const HaloItem &it = R.g[j];
        if (!it.shown || it.alpha <= 0 || it.b[2] < it.b[0]) continue;
        const float x0 = it.b[0] - L.dx - CX, y0 = it.b[1] - L.dy - CX, x1 = it.b[2] - L.dx - CX, y1 = it.b[3] - L.dy - CX;
        const float fx = fmaxf(fabsf(x0), fabsf(x1)), fy = fmaxf(fabsf(y0), fabsf(y1));
        const float nx = x0 > 0 ? x0 : x1 < 0 ? -x1 : 0, ny = y0 > 0 ? y0 : y1 < 0 ? -y1 : 0;
        out = fmaxf(out, hypotf(fx, fy)), in = fminf(in, hypotf(nx, ny));
      }
    }
    if (out >= 0) st_radii[i * 2] = in, st_radii[i * 2 + 1] = out;
  }
  return st_radii;
}

// HaloWatch's place of each layer of screen s in the last frame (where it stands less its dx, dy): x, y per layer;
// NaN — not drawn yet (its font is still on its way)
EMSCRIPTEN_KEEPALIVE float *studio_places(int s) {
  if (s >= 0 && s < face_scr_n)
    for (int i = 0; i < face_scr[s].n; i++) {
      const FaceRun &R = face_scr[s].run[i];
      const FaceLayer &L = face_scr[s].layers[i];
      st_places[i * 2] = R.ax - L.dx, st_places[i * 2 + 1] = R.ay - L.dy;
    }
  return st_places;
}

// HaloWatch's place (x, y) of each kind (FL_*): the face's — exactly; the Weather screen's — about (the icon and the
// high/low stand by the temperature, the clock along the edge by its text): for a layer just dropped on a screen
EMSCRIPTEN_KEEPALIVE float *studio_bases() {
  for (int k = 0; k < 3; k++) {
    FaceLayer L{};
    L.kind = k;
    st_bases[k * 2] = face_base_x(L), st_bases[k * 2 + 1] = face_base_y(L);
  }
  const float wx[11][2] = {{CX, CX - TT_R + 6},       {WX_CITY_X, WX_CITY_Y},  {CX - 108, WX_NOW_Y},     {CX, WX_NOW_Y},
                           {CX + 136, WX_NOW_Y - 18}, {WX_PILL_X, WX_PILL_Y},  {CX, CX + DOTS_R},
                           {CX, 64 + GL_HEAD_H / 2},  {CX, 136 + GL_ITEM_H / 2}, {CX, CX + EB_R - EB_FULL / 2},
                           {CX + 223, CX}};
  for (int k = 0; k < 11; k++) st_bases[(FL_ARC + k) * 2] = wx[k][0], st_bases[(FL_ARC + k) * 2 + 1] = wx[k][1];
  FaceLayer S{};
  S.kind = FL_STEPS;
  st_bases[FL_STEPS * 2] = face_base_x(S), st_bases[FL_STEPS * 2 + 1] = face_base_y(S);
  st_bases[FL_STARS * 2] = CX, st_bases[FL_STARS * 2 + 1] = CX;
  st_bases[FL_FEELS * 2] = CX, st_bases[FL_FEELS * 2 + 1] = WX_FEELS_Y;
  st_bases[FL_DISPLAY * 2] = st_bases[FL_DISPLAY * 2 + 1] = CX;
  st_bases[FL_BATTERY * 2] = st_bases[FL_BATTERY * 2 + 1] = CX;
  st_bases[FL_MUSIC * 2] = st_bases[FL_MUSIC * 2 + 1] = CX;
  st_bases[FL_RECORDER * 2] = st_bases[FL_RECORDER * 2 + 1] = CX;
  st_bases[FL_VIDEO * 2] = st_bases[FL_VIDEO * 2 + 1] = CX;
  st_bases[FL_SHADE * 2] = CX, st_bases[FL_SHADE * 2 + 1] = CY;
  st_bases[FL_SCLOCK * 2] = CX, st_bases[FL_SCLOCK * 2 + 1] = CY;  // (a clock of sclock.h: by the screen's middle)
  for (int k = FL_SNKDOTS; k <= FL_SNKISO; k++) st_bases[k * 2] = st_bases[k * 2 + 1] = CX;
#if HAS_STEPSCR
  const float st[4][2] = {{CX, SS_NUM_Y}, {CX, SS_CAP_Y}, {CX, SS_FOOT_Y}, {SS_CX, SS_CY}};  // the Steps screen's parts
  for (int k = 0; k < 4; k++) st_bases[(FL_STNUM + k) * 2] = st[k][0], st_bases[(FL_STNUM + k) * 2 + 1] = st[k][1];
#endif
  // The face's widgets (widgets.h): their middles at rest
  const float wg[7][2] = {{CX, CX}, {CX, CX}, {CX, CX - 190}, {CX, 372}, {CX, CX}, {CX, 226}, {CX, 189}};
  for (int k = 0; k < 7; k++) st_bases[(FL_CURVE + k) * 2] = wg[k][0], st_bases[(FL_CURVE + k) * 2 + 1] = wg[k][1];
  st_bases[FL_BATRING * 2] = st_bases[FL_BATRING * 2 + 1] = CX;
  st_bases[FL_PARROT * 2] = CX, st_bases[FL_PARROT * 2 + 1] = CX;
#ifndef HALO_ROUND
  // A rectangle's places (widgets_rect.h, wxscreen_rect.h): where each stands before it is moved
  const float wr[5][2] = {{CX, WR_WEEK_CAP_Y}, {CX, WR_INFO_Y}, {CX, WR_FC_BOX_Y + WR_FC_BOX_H / 2.0f}, {CX, WR_FRAME_CY}, {CX, WR_DOT_CY}};
  for (int k = 0; k < 5; k++) st_bases[(FL_WEEKARC + k) * 2] = wr[k][0], st_bases[(FL_WEEKARC + k) * 2 + 1] = wr[k][1];
  st_bases[FL_CURVE * 2 + 1] = CY, st_bases[FL_STARS * 2 + 1] = CY, st_bases[FL_PARROT * 2 + 1] = CY;
  st_bases[FL_FEELS * 2] = RW_TEXT_R, st_bases[FL_FEELS * 2 + 1] = RW_FEELS_Y;
#endif
  for (int k = FL_DEVCARD; k <= FL_CHARGE; k++) st_bases[k * 2] = st_bases[k * 2 + 1] = CX;
  return st_bases;
}

// The screen on top now: -1 — the face, -2 — a stub (a card without a link: studio_stub — its title); Try shows it
EMSCRIPTEN_KEEPALIVE int studio_open() {
  if (halo_nav_on_face()) return -1;
  if (halo_nav_over()) return face_over;
  const int d = halo_nav_depth();
  return nav_scr[d] >= 0 ? nav_scr[d] : -2;
}
EMSCRIPTEN_KEEPALIVE const char *studio_stub() {
  return gl_str(nav_stub[halo_nav_depth()]);
}

// Try starts where the editor is: as a tap on layer i of screen s, which is on top now — a layer of the face opens its
// screen over the face (as face_update), a card or the button of the gallery or of a screen on it — its screen on the
// stack in the next frame (as gal_update: gl_req)
EMSCRIPTEN_KEEPALIVE void studio_tap(int s, int i) {
  if (s < 0 || s >= face_scr_n || i < 0 || i >= face_scr[s].n) return;
  const FaceScreen &S = face_scr[s];
  const FaceLayer &L = S.layers[i];
  float x, y, w, h;
  if (s > 0) return (void)(gl_req = &L, gl_req_depth = halo_nav_depth());
  if (L.tap <= 0 || L.tap >= face_scr_n || !layer_rect(S, i, x, y, w, h)) return;
  face_over = L.tap;
  halo_nav_face_open(OVER, L.fx, x, y, w, h);
}

// The gallery: screen s is opened by a swipe up on the face (-1 — none). Changed — the board goes to the face
EMSCRIPTEN_KEEPALIVE void studio_main(int s) {
  if (s <= 0 || s >= face_scr_n) s = -1;
  if (s == nav_scr[0]) return;
  halo_nav_begin(HaloScreen{});  // the old one hidden, the face on screen
  nav_scr[0] = s;
  gallery_begin();
}

// A pinned screen's list scrolled so that layer i is in view (the editor selected it): a title or a card — in the
// middle as far as the list goes, the button at the bottom edge — the end; -1 — the top. As if the list stood there
EMSCRIPTEN_KEEPALIVE void studio_scroll(int s, int i) {
  if (s < 0 || s >= face_scr_n || !face_scr[s].gal || !face_scr[s].gal->list.n) return;
  GalRun &g = *face_scr[s].gal;
  HaloList &l = g.list;
  const int k = i >= 0 ? gal_item_of(g, i) : -1;
  const bool edge = i >= 0 && face_scr[s].layers[i].kind == FL_EDGE;
  float to = l.top;
  if (edge) to = halo_list_min(l);
  else if (k >= 0) to = CX - l.cum[k] - l.h[k] / 2;
#ifndef HALO_ROUND
  for (int c = 0; c < g.cells; c++)
    if (g.cell[c].layer == i) to = CY - g.cell[c].y - g.cell[c].h / 2;
#endif
  to = fminf(l.top, fmaxf(fminf(l.top, halo_list_min(l)), to));
  l.scroll = l.last = to;
  halo_list_update(l, 0, false);  // (a pinned copy has no finger) just the layout
  g.tt_k = 1 - halo_clamp01((l.top - l.scroll) / (36 * DP));
  halo_spring_set(g.eb_h, gal_eb_height(g));
  halo_spring_set(g.sc_pos, halo_list_progress(l));
  halo_damage_all();
}
}
