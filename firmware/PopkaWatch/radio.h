// radio.h — the Radio app (FL_RADIO), round board: PopkaStudio's own. The screens as the user picked them on 12 Nov 2026
// (web/mockups/radio5.html variant 1, radio2.html list variant B):
//
//   * the main screen — "Poster": the time, the station in big letters (two lines; a long name — smaller, three), the
//     song (a marquee), STATIONS and VOLUME — two words letter-spaced with an icon over each and a thin line between
//     (radio7.html variant 8, 13 Nov 2026: LIVE went, the name went up), Stop / Play as the bottom-edge button
//     (HI_EDGE, as the gallery's). Along
//     the left and the right edges an equalizer: 23 rays a side grow from the bezel inward — the low bands at the
//     bottom of an arc (+32°, above the button), the high ones at its top (−64°); the loud ones brighter, the loudest
//     nearly white. The levels are the sound being played (radio_net.h: Goertzel on each block the speaker takes),
//     30 times a second. A swipe left / right — the next / previous station of the folder; VOLUME (or a long press on
//     the button) — the volume screen (volume.h, the Music's);
//   * Browse — the catalog's folders as the gallery's cards (410 × 104: a round icon, the name, a line under it), and
//     a folder's stations the same way (the station's letters in its colour, its country and bitrate, a heart; the one
//     on, while the radio is on — an equalizer in its circle instead of its letters, as the folder on the dial). At the
//     end of both lists — the bottom-edge button with what plays (a tap — back to the main screen): the gallery's
//     (gallery.h, FL_EDGE), it grows from below as the list reaches its end (13 Nov 2026, the user's wish: it covered
//     the cards when it stood there always).
//
// A station picked in a folder puts that folder on the dial (what the swipes go through) and plays — the list stays,
// the station's circle shows it loading (the bars in a wave), then playing; tapped again while it plays — the main
// screen (halo_nav_back_to). Hearts are the favourites (NVS "radio": the studio's list is the start — RD_FAV_HASH,
// a new list from the editor replaces the board's). Browse and the stations are lib/Halo screens of their own
// (halo_nav_open: the Android 16 transition, back — a swipe from the left edge); the main screen is the app's (a
// section, as the Music). Their items live in PSRAM; the rays and the marquee are drawn by this file straight into the
// band. The sound and the catalog: radio_net.h (the board). The editor's page has no network and no sound: the stations
// play made-up songs and levels, a folder shows the favourites of its tag or country after a short "loading".
// Included from settings.h after recorder.h (sound.h and volume.h first).

#define RD_NAME 48
#define RD_URL 200
#define RD_TITLE 128
#define RD_RES 23          // a folder's stations (with the list's header: HALO_LIST_MAX)
#define RD_FAV_MAX 16      // favourites (NVS: ~5 KB)
#define RD_FOLDER_MAX 16
#define RD_AGENT "2.1"
#define RD_BANDS 23        // the equalizer's rays on a side (radio_net.h measures as many bands)
// The main screen (466 px): the middles of the lines
#define RP_TIME_Y 27.0f
// (radio7.html variant 8, 13 Nov 2026: no LIVE, the name raised; STATIONS and VOLUME — words with an icon over each)
#define RP_NAME_Y 146.0f   // the name's block (two lines of 52 px, or three of 32)
#define RP_NAME_W 290.0f   // (raised, its first line is between the top rays: ~200 px there at the caps' top)
#define RP_SONG_Y 241.0f
#define RP_SONG_W 280.0f
#define RP_BTN_IC_Y 291.0f // STATIONS and VOLUME: the icons …
#define RP_BTN_T_Y 316.0f  // … the words under them
                           // (STATIONS and VOLUME stand 65 px left and right of the middle: the editor's layers' dx)
#define RP_DIV_Y 307.0f
#define RP_EDGE_H 92.0f    // the bottom-edge button: its height
#define RP_EDGE_R 227.0f   // … its bottom follows the screen's circle 3 dp in
#define RP_A_TOP -64.0f    // the rays' arc, degrees from 3 o'clock (the right side; the left mirrors it)
#define RP_A_BOT 32.0f
#define RP_R0 224.0f       // the rays start at the bezel …
#define RP_LMAX 58.0f      // … and grow inward this much
#define RD_ORANGE 0xFF8A3D
#define RD_ON_ORANGE 0x2A1200
#define RD_RAY 0xFFB98A
#define RD_RAY_TOP 0xFFF1E4
#define RD_RAY_OFF 0x3A3A40
#define RD_CARD 0x2E3034   // the gallery's card
#define RD_TEXT 0xFFF8EE
#define RD_DIM 0x9E9E9E
#define RD_ERR 0xFF8A80
#define RD_PINK 0xFF8BCB

extern const HaloFont gl_ra_n52, gl_ra_n32, gl_ra_t24, gl_ra_t20, gl_ra_t14, gl_ra_ic24;  // ROUND_APP_FONTS in studio.py
static const char RD_PLAY[] = "\xEF\x81\x8B", RD_STOP[] = "\xEF\x81\x8D", RD_BARS[] = "\xEF\x83\x89", RD_HEART[] = "\xEF\x80\x84",
                  RD_VOL[] = "\xEF\x80\xA8";

struct RdSt {  // a station
  char name[RD_NAME], lab[24], url[RD_URL], uuid[40], cc[4], tag[20], mono[12];
  uint32_t col;
  uint16_t kbps;
};
struct RdDef {  // a favourite from the editor (radiodata.h)
  const char *name, *url, *uuid, *cc, *tag;
  int kbps;
  uint32_t col;
};
enum : uint8_t { RF_FAV, RF_TOP, RF_TAG, RF_CC };
struct RdFolder {  // a Browse folder (radiodata.h): the favourites, the most listened, a tag or a country
  const char *name;
  uint8_t kind;
  const char *arg;
  uint32_t col;
  uint8_t icon;  // RD_FIC
};
static const char *const RD_FIC[] = {"\xEF\x80\x84", "\xEF\x81\xAD", "\xEF\x80\x81", "\xEF\x84\xB0", "\xEF\x82\xAC"};  // heart, fire, note, microphone, globe
#include "radiodata.h"  // the project's favourites and folders (studio.py)

// ---------- Stations ----------
static const uint32_t RD_PAL[12] = {0xF2994A, 0x7CC47F, 0xE6007E, 0xF5C400, 0x3E7BDB, 0xE2483D,
                                    0x9B5CFF, 0x0E8C8A, 0xFF6A13, 0x5FA8A0, 0xC9A0FF, 0x2E6FE0};
static uint32_t rd_hash(const char *s) {
  uint32_t h = 2166136261u;
  while (*s) h = (h ^ (uint8_t)*s++) * 16777619u;
  return h;
}
static int rd_char_len(const char *s) {  // bytes of the UTF-8 letter at s
  const uint8_t c = *s;
  return c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
}
static void rd_copy(char *out, const char *s, size_t cap) {  // cut on a letter's boundary
  size_t n = strlen(s);
  if (n >= cap) {
    n = cap - 1;
    while (n && ((uint8_t)s[n] >> 6) == 2) n--;
  }
  memcpy(out, s, n), out[n] = 0;
}
// Fit to w px in f: cut with "…" (UTF-8 aware)
static void rd_fit(const HaloFont &f, const char *s, float w, char *out, size_t cap) {
  rd_copy(out, s, cap);
  if (halo_text_width(f, out) <= w) return;
  size_t n = strlen(out);
  while (n > 0) {
    do n--; while (n && ((uint8_t)out[n] >> 6) == 2);
    if (n + 4 > cap) continue;
    memcpy(out + n, "\xE2\x80\xA6", 4);  // "…"
    if (halo_text_width(f, out) <= w) return;
  }
  strlcpy(out, "\xE2\x80\xA6", cap);
}
static void rd_mono(char *out, size_t cap, const char *name) {  // the logo's letters: two words' first, or one short word
  const char *w[3];
  int n = 0;
  for (const char *p = name; *p && n < 3;) {
    while (*p == ' ' || *p == '-' || *p == '_' || *p == '.' || *p == '(') p++;
    if (!*p) break;
    w[n++] = p;
    while (*p && *p != ' ' && *p != '-' && *p != '_' && *p != '.' && *p != '(') p++;
  }
  out[0] = 0;
  if (!n) return (void)strlcpy(out, "R", cap);
  if (n >= 2 && n <= 3 && !strncasecmp(w[0], "radio ", 6)) w[0] = w[1], w[1] = n == 3 ? w[2] : nullptr, n--;  // ("Radio X" → X)
  if (n == 1 || !w[1]) {  // one word: short and in capitals — itself (FIP, KEXP); else its first two letters
    const char *p = w[0];
    int len = 0, caps = 1;
    while (p[len] && p[len] != ' ') caps &= !(p[len] >= 'a' && p[len] <= 'z'), len++;
    const int take = caps && len <= 4 ? len : rd_char_len(p) + rd_char_len(p + rd_char_len(p));
    rd_copy(out, p, min((size_t)take + 1, cap));
  } else {
    const int a = rd_char_len(w[0]), b = rd_char_len(w[1]);
    if ((size_t)(a + b) < cap) memcpy(out, w[0], a), memcpy(out + a, w[1], b), out[a + b] = 0;
  }
  for (char *p = out; *p; p++)
    if (*p >= 'a' && *p <= 'z') *p -= 32;
}
static void rd_station_make(RdSt &o, const char *name, const char *url, const char *uuid, const char *cc,
                            const char *tags, int kbps, uint32_t col) {
  memset(&o, 0, sizeof(o));
  while (*name == ' ' || *name == '\t' || *name == '\n') name++;
  rd_copy(o.name, *name ? name : "Radio", sizeof(o.name));
  for (int n = strlen(o.name); n > 0 && (o.name[n - 1] == ' ' || o.name[n - 1] == '\t' || o.name[n - 1] == '\n'); n--) o.name[n - 1] = 0;
  strlcpy(o.url, url, sizeof(o.url));
  strlcpy(o.uuid, uuid, sizeof(o.uuid));
  for (int i = 0; i < 2 && cc[i]; i++) o.cc[i] = toupper((uint8_t)cc[i]);
  const char *c = strchr(tags, ',');
  size_t tl = c ? (size_t)(c - tags) : strlen(tags);
  if (tl >= sizeof(o.tag)) tl = sizeof(o.tag) - 1;
  memcpy(o.tag, tags, tl), o.tag[tl] = 0;
  if (o.tag[0] >= 'a' && o.tag[0] <= 'z') o.tag[0] -= 32;
  o.kbps = kbps > 0 ? kbps : 128;
  o.col = col ? col : RD_PAL[rd_hash(o.name) % 12];
  rd_mono(o.mono, sizeof(o.mono), o.name);
  rd_fit(gl_ra_t14, o.name, 116, o.lab, sizeof(o.lab));
}
static uint32_t rd_light(uint32_t c) {  // a station's colour, halfway to white (the playing one's name)
  return (((c >> 16 & 255) + 255) / 2) << 16 | (((c >> 8 & 255) + 255) / 2) << 8 | ((c & 255) + 255) / 2;
}
static uint32_t rd_ink(uint32_t c) {  // letters on a station's colour: dark on light ones
  const int r = c >> 16, g = c >> 8 & 255, b = c & 255;
  return r * 299 + g * 587 + b * 114 > 160000 ? 0x1A1A1A : 0xFFFFFF;
}

#if !defined(__EMSCRIPTEN__) && defined(FN_WIFI) && FN_WIFI
#define RD_NET 1  // the board, with Wi-Fi: the real stream (radio_net.h)
#else
#define RD_NET 0  // the editor's page — or a board without Wi-Fi (the studio turns it on with the Radio)
#endif

#if !RD_NET
// ---------- The editor's page: no network, no sound — the same answers, made up ----------
enum { RN_OFF, RN_JOINING, RN_CONNECTING, RN_STREAMING, RN_NOWIFI, RN_FAILED };
static int rn_net, rn_fetch_st, rn_res_n, rn_kbps;
static bool rn_playing, rn_on;
static uint32_t rn_title_gen, rn_fetch_gen, rn_t0, rn_tf;
static float rn_band[RD_BANDS];
static RdSt rn_res_buf[RD_RES], *rn_res = rn_res_buf;
static char rn_title[RD_TITLE];
static int rn_demo, rn_fetch_folder;
static void rn_play(const char *) {
  rn_on = true, rn_playing = false, rn_net = RN_CONNECTING, rn_t0 = halo_ms(), rn_title[0] = 0, rn_title_gen++;
#ifndef __EMSCRIPTEN__
  rn_net = RN_NOWIFI;  // (a board without the Wi-Fi function)
#endif
}
static void rn_pause() { rn_on = false, rn_playing = false, rn_net = RN_OFF; }
static void rn_fetch(const char *, int folder) { rn_fetch_st = 1, rn_tf = halo_ms(), rn_fetch_folder = folder; }
static void rn_title_copy(char *out, size_t n) { strlcpy(out, rn_title, n); }
#else
#include "radio_net.h"
static void rn_fetch(const char *path, int) { rn_fetch(path); }
#endif

// ---------- The favourites, the dial (the folder the swipes go through) ----------
static RdSt *rd_fav, *rd_list;  // PSRAM: RD_FAV_MAX, RD_RES + 1
static int rd_fav_n, rd_list_n, rd_folder, rd_at = -1;  // rd_at — the playing one in rd_list
static RdSt rd_cur;
static bool rd_has_cur, rd_on;
static bool rd_fav_dirty;
static uint32_t rd_fav_changed;
static int rd_main_depth = 1;  // where the main screen is in lib/Halo's stack (Browse opens over it)
static float rd_clock;

static int rd_mod(int a, int n) { return n ? ((a % n) + n) % n : 0; }
static int rd_fav_find(const char *url) {
  for (int i = 0; i < rd_fav_n; i++)
    if (!strcmp(rd_fav[i].url, url)) return i;
  return -1;
}
static void rd_list_favs() {  // the dial = the favourites (and the playing one, if it is not one any more)
  memcpy(rd_list, rd_fav, rd_fav_n * sizeof(RdSt));
  rd_list_n = rd_fav_n;
  if (rd_has_cur && rd_fav_find(rd_cur.url) < 0 && rd_list_n < RD_RES + 1) rd_list[rd_list_n++] = rd_cur;
  rd_at = -1;
  for (int i = 0; i < rd_list_n && rd_has_cur; i++)
    if (!strcmp(rd_list[i].url, rd_cur.url)) rd_at = i;
}
static void rd_fav_load() {
  rd_fav_n = 0;
#ifndef __EMSCRIPTEN__
  Preferences p;
  p.begin("radio", true);
  const size_t bytes = p.getBytesLength("favs");
  if (p.getUInt("hash", 0) == RD_FAV_HASH && bytes && bytes % sizeof(RdSt) == 0 && bytes / sizeof(RdSt) <= RD_FAV_MAX)
    rd_fav_n = p.getBytes("favs", rd_fav, bytes) / sizeof(RdSt);
  p.end();
#endif
  if (rd_fav_n) return;
  for (int i = 0; i < RD_FAV_N && i < RD_FAV_MAX; i++) {
    const RdDef &d = RD_FAVDEF[i];
    if (d.url[0]) rd_station_make(rd_fav[rd_fav_n++], d.name, d.url, d.uuid, d.cc, d.tag, d.kbps, d.col);
  }
  rd_fav_dirty = true, rd_fav_changed = halo_ms();
}
static uint32_t rd_pop_at, rd_pop_url;  // a heart just turned on: its bounce
static void rd_fav_toggle(const RdSt &s) {
  const int i = rd_fav_find(s.url);
  if (i >= 0) memmove(rd_fav + i, rd_fav + i + 1, (rd_fav_n - i - 1) * sizeof(RdSt)), rd_fav_n--;
  else if (rd_fav_n < RD_FAV_MAX) rd_fav[rd_fav_n++] = s, rd_pop_url = rd_hash(s.url), rd_pop_at = halo_ms();
  rd_fav_dirty = true, rd_fav_changed = halo_ms();
  if (RD_FOLDER[rd_folder].kind == RF_FAV) rd_list_favs();
}

// ---------- The main screen's words ----------
// What its parts show (a part is a layer: radio_part_layout): the station's name (rp_name_gen +1 when it changes — the
// name parts wrap it again in their fonts), the song's line (rp_song_gen; rp_song_t — seconds since it changed: the
// marquee), the time
static char rp_name_s[RD_NAME] = "Radio", rp_song[RD_TITLE], rp_time[8], rp_edge_s[RD_NAME + 8];
static uint32_t rp_name_gen = 1, rp_song_gen = 1;
static float rp_song_t;
static void rp_name(const char *s) {
  rd_copy(rp_name_s, s, sizeof(rp_name_s));
  rp_name_gen++;
}
// s in lines of font f (max_lines; true — all of it): the longest runs of whole words that fit — the top line w1 px (the
// circle is narrower up there: a word that doesn't fit goes down — "BBC / World Service", not "BBC World / Service"),
// the others w px; a first word wider than w1 — alone on the top line ("SomaFM / Secret Agent"); a word wider than w —
// cut with "…". (A small w1 — always one word on top; w1 ≥ w — the longest top line that fits, as before)
static bool rp_wrap(const HaloFont &f, const char *s, float w1, float w, char (*out)[RD_NAME + 8], int max_lines, int &lines) {
  lines = 0;
  if (halo_text_width(f, s) <= w) return strlcpy(out[0], s, sizeof(out[0])), lines = 1, true;  // (one line: in the middle, w)
  const char *p = s;
  while (*p && lines < max_lines) {
    while (*p == ' ') p++;
    char *o = out[lines];
    o[0] = 0;
    const float lw = lines ? w : fminf(w1, w);
    const char *best = nullptr;
    for (const char *q = p;; q++) {
      if (*q == ' ' || !*q) {
        char tmp[RD_NAME + 8];
        const size_t n = min((size_t)(q - p), sizeof(tmp) - 1);
        memcpy(tmp, p, n), tmp[n] = 0;
        if (halo_text_width(f, tmp) <= lw) best = q, strlcpy(o, tmp, sizeof(out[0]));
        else break;
        if (!*q) break;
      }
    }
    if (!best && !lines) {  // (the first word wider than the top line: alone there, if it fits the others' width)
      char word[RD_NAME + 8];
      const size_t n = min(strcspn(p, " "), sizeof(word) - 1);
      memcpy(word, p, n), word[n] = 0;
      if (halo_text_width(f, word) <= w) best = p + n, strlcpy(o, word, sizeof(out[0]));
    }
    if (!best) {
      rd_fit(f, p, w, o, sizeof(out[0]));
      lines++;
      return false;
    }
    lines++, p = best;
  }
  while (*p == ' ') p++;
  return !*p;
}

// Play station i of the dial; dir — which way the name slides in (a swipe), 0 — none
static float rp_slide;  // the name's slide-in: ±1 → 0
static HaloSpring rp_slide_s;
static HaloSpringK K_RP_SLIDE;
static uint32_t rd_start_ms;  // a station started (halo_ms): the loading bars fade in over its letters from then
static void rd_tune(int i, int dir = 0) {
  if (i < 0 || i >= rd_list_n) return;
  const bool same = rd_has_cur && !strcmp(rd_cur.url, rd_list[i].url) && rd_on;
  rd_cur = rd_list[i], rd_has_cur = true, rd_on = true;
  if (RD_FOLDER[rd_folder].kind == RF_FAV && rd_list_n > rd_fav_n) rd_list_favs();  // (the left-over one goes)
  rd_at = -1;
  for (int k = 0; k < rd_list_n; k++)
    if (!strcmp(rd_list[k].url, rd_cur.url)) rd_at = k;
  rp_name(rd_cur.name);
  if (dir) rp_slide_s = {(float)dir, 0, 0};
  if (!same) {
#if HAS_MUSIC
    if (pl_playing) pl_set_playing(false);  // (one sound at a time)
#endif
#if RD_NET
    strlcpy(rn_click_uuid, rd_cur.uuid, sizeof(rn_click_uuid));
#endif
    rn_play(rd_cur.url);
    rd_start_ms = halo_ms();
  }
}
static void rd_toggle() {
  if (!rd_has_cur) return rd_list_n ? rd_tune(0) : void();
  rd_on = !rd_on;
  if (rd_on) {
#if HAS_MUSIC
    if (pl_playing) pl_set_playing(false);
#endif
    rn_play(rd_cur.url);
    rd_start_ms = halo_ms();
  } else {
    rn_pause();
  }
}
static void rd_other_sound() {  // the Music or the Recorder starts: the radio stops
  if (rd_on) rd_on = false, rn_pause();
}

// ---------- The screens' items (PSRAM) ----------
struct RdEdge {  // the bottom-edge button with what plays, at the end of Browse and of the stations
  HaloSpring h;  // its height now, px
  uint8_t mode;  // EB_NONE, EB_STILL, EB_GROWS (gallery.h)
  bool armed;
  Press press;
  HaloItem bg, t, eq[3];
};
struct RdUi {
  // the main screen: the rays' levels (smoothed) — what each equalizer part shows (its own items: RdPart)
  float lv[RD_BANDS];
  // Browse
  HaloList b_list;
  Press b_press[HALO_LIST_MAX];
  HaloItem b_time, b_head, b_card[RD_FOLDER_MAX], b_dot[RD_FOLDER_MAX], b_ic[RD_FOLDER_MAX], b_text[RD_FOLDER_MAX],
      b_sub[RD_FOLDER_MAX], b_eq[3];
  char b_sub_s[RD_FOLDER_MAX][32];
  RdEdge b_edge;
  // a folder's stations
  HaloList s_list;
  Press s_press[HALO_LIST_MAX];
  HaloItem s_time, s_head, s_card[RD_RES], s_logo[RD_RES], s_mono[RD_RES], s_name[RD_RES], s_sub[RD_RES], s_heart[RD_RES];
  HaloItem s_eq[3], s_msg, s_btn, s_btn_t, s_shim[3][4];
  RdEdge s_edge;
  char s_name_s[RD_RES][RD_NAME + 8], s_sub_s[RD_RES][32];
  RdSt s_rows[RD_RES];
  int s_n, s_folder, s_state;  // s_state: 0 — loading, 1 — rows, 2 — failed
  uint32_t s_gen;
  char s_path[220];             // the folder's question to the catalog (Retry asks it again)
};
static RdUi *ru;
static float rd_mid_n52, rd_mid_n32, rd_mid_t24, rd_mid_t20, rd_mid_t14, rd_mid_ic24;
static int rp_armed = -1, rb_armed = -1, rs_armed = -1, rs_heart = -1;
// The cards seen now (layout → draw): from, to (excl.). The items live in PSRAM and draw runs for every band of 16
// rows on both cores: going through all 23 stations' items there read ~25 KB of PSRAM 60 times a frame
static int rb_v0, rb_v1, rs_v0, rs_v1;
static bool rp_drag, rp_long_done, rs_btn_armed;
static float rp_fa = 1, rp_drag_x, rp_lv_t;
static int rp_owner;

// ---------- Build, every frame ----------
static void radio_build() {
  ru = (RdUi *)heap_caps_malloc(sizeof(RdUi), MALLOC_CAP_SPIRAM);
  rd_fav = (RdSt *)heap_caps_malloc(RD_FAV_MAX * sizeof(RdSt), MALLOC_CAP_SPIRAM);
  rd_list = (RdSt *)heap_caps_malloc((RD_RES + 1) * sizeof(RdSt), MALLOC_CAP_SPIRAM);
  if (!ru || !rd_fav || !rd_list) return (void)(ru = nullptr);
  memset(ru, 0, sizeof(RdUi)), memset(rd_fav, 0, RD_FAV_MAX * sizeof(RdSt)), memset(rd_list, 0, (RD_RES + 1) * sizeof(RdSt));
  K_RP_SLIDE = halo_spring(HALO_M3X_DEFAULT);
  rd_mid_n52 = halo_text_mid(gl_ra_n52, "Radio"), rd_mid_n32 = halo_text_mid(gl_ra_n32, "Radio");
  rd_mid_t24 = halo_text_mid(gl_ra_t24, "Radio"), rd_mid_t20 = halo_text_mid(gl_ra_t20, "Radio");
  rd_mid_t14 = halo_text_mid(gl_ra_t14, "LIVE"), rd_mid_ic24 = halo_text_mid(gl_ra_ic24, RD_HEART);
  vol_build();  // the speaker's volume (volume.h)
  rd_fav_load();
  rd_folder = 0;
  rd_list_favs();
  rp_name(rd_list_n ? rd_list[0].name : "Radio");
#if RD_NET
  rn_begin();
#endif
}
// From loop(): the favourites into flash 1.5 s after the last change (a flash write stalls the other core a moment)
static void radio_poll() {
#ifndef __EMSCRIPTEN__
  if (!rd_fav_dirty || halo_ms() - rd_fav_changed < 1500) return;
  rd_fav_dirty = false;
  Preferences p;
  p.begin("radio", false);
  p.putUInt("hash", RD_FAV_HASH);
  p.putBytes("favs", rd_fav, rd_fav_n * sizeof(RdSt));
  p.end();
  LOG("radio: %d favourites saved\n", rd_fav_n);
#endif
}
static void radio_tick(float dt) {  // every frame, whatever is on screen
  if (!ru) return;
  rd_clock = fmodf(rd_clock + dt, 1000);
#ifdef __EMSCRIPTEN__
  static const char *const SONGS[] = {"Moon Harbor \xE2\x80\x94 Slow Tide", "Kite Season \xE2\x80\x94 Paper Lanterns",
                                      "Lumen Drift \xE2\x80\x94 Under Glass", "Les Mar\xC3\xA9" "es \xE2\x80\x94 Caf\xC3\xA9 de minuit"};
  if (rn_on && !rn_playing && halo_ms() - rn_t0 > 900) rn_playing = true, rn_net = RN_STREAMING, rn_demo++;
  if (rn_playing && (!rn_title[0] || halo_ms() - rn_t0 > 12000)) {
    strlcpy(rn_title, SONGS[(rn_demo + (halo_ms() / 12000)) % 4], sizeof(rn_title)), rn_title_gen++, rn_t0 = halo_ms() - 900;
  }
  for (int i = 0; i < RD_BANDS; i++) {  // a made-up music: the low bands louder and on a beat
    const float b = i / (RD_BANDS - 1.0f), beat = 0.55f + 0.45f * fmaxf(0, sinf(rd_clock * 4.2f));
    rn_band[i] = rn_playing ? fminf(1, (0.9f - 0.55f * b) * (b < 0.3f ? beat : 1) *
                                           (0.35f + 0.65f * fabsf(sinf(rd_clock * 3.2f + i * 0.7f) * sinf(rd_clock * 1.8f + i * 0.29f))))
                            : 0;
  }
  if (rn_fetch_st == 1 && halo_ms() - rn_tf > 650) {  // a folder: the favourites of its tag or country (no network here)
    const RdFolder &f = RD_FOLDER[rn_fetch_folder];
    rn_res_n = 0;
    for (int i = 0; i < rd_fav_n && rn_res_n < RD_RES; i++)
      if (f.kind == RF_TOP || (f.kind == RF_TAG && !strcasecmp(rd_fav[i].tag, f.arg)) ||
          (f.kind == RF_CC && !strcasecmp(rd_fav[i].cc, f.arg)))
        rn_res[rn_res_n++] = rd_fav[i];
    rn_fetch_st = 2, rn_fetch_gen++;
  }
#endif
  // The rays: 30 times a second toward the bands the speaker plays — up quickly, down slowly
  if ((rp_lv_t += dt) >= 1 / 30.0f) {
    rp_lv_t = 0;
    for (int i = 0; i < RD_BANDS; i++) {
      const float t = rd_on && rn_playing ? rn_band[i] : 0;
      ru->lv[i] += (t - ru->lv[i]) * (t > ru->lv[i] ? 0.55f : 0.25f);
    }
  }
  halo_spring_run(rp_slide_s, K_RP_SLIDE, dt);
  rp_slide = rp_slide_s.x;
  // The song's line: the title, or how it goes (what the stream does)
  char s[RD_TITLE];
  if (!rd_has_cur) snprintf(s, sizeof(s), "%s", rd_list_n ? (rd_list[0].tag[0] ? rd_list[0].tag : "Radio") : "No stations");
  else if (!rd_on) strlcpy(s, "Stopped", sizeof(s));
  else if (rn_net == RN_NOWIFI) strlcpy(s, "No Wi-Fi", sizeof(s));
  else if (rn_net == RN_FAILED && !rn_playing) strlcpy(s, "Can't connect", sizeof(s));
  else if (!rn_playing) strlcpy(s, rn_net == RN_JOINING ? "Joining Wi-Fi\xE2\x80\xA6" : "Buffering\xE2\x80\xA6", sizeof(s));  // (connected, or filling its 2 s)
  else {
    rn_title_copy(s, sizeof(s));
    if (!s[0]) snprintf(s, sizeof(s), "%s%s%s", rd_cur.tag[0] ? rd_cur.tag : "Live", rd_cur.cc[0] ? " \xC2\xB7 " : "", rd_cur.cc);
  }
  if (strcmp(s, rp_song)) strlcpy(rp_song, s, sizeof(rp_song)), rp_song_gen++, rp_song_t = 0;
  rp_song_t = fminf(rp_song_t + dt, 1e6f);  // (the marquee: each song part by its own width — rp_marquee)
  // The time (the face's 12 or 24 hours) — once a second: localtime_r is ~70 µs, and this runs every frame, the radio's
  // screen open or not
  static time_t at;
  static bool h24;
  const time_t now = time(nullptr);
  if (now == at && h24 == fc_h24 && rp_time[0]) return;
  at = now, h24 = fc_h24;
  struct tm tm;
  localtime_r(&now, &tm);
  const int h = fc_h24 ? tm.tm_hour : (tm.tm_hour + 11) % 12 + 1;
  snprintf(rp_time, sizeof(rp_time), fc_h24 ? "%02d:%02d" : "%d:%02d", h, tm.tm_min);
}

// ---------- The main screen: its parts (a layer each: face.h FL_RTIME … FL_RPLAY; radio7.html variant 8) ----------
// The screen's own layer (FL_RADIO) holds what is the whole screen's: the volume screen over it (radio_layout: the
// parts fade through), the swipes, the taps — on the parts where they stand. Each part: its items and what it draws
// itself in PSRAM (internal memory is Wi-Fi's), its place — here, moved by the layer's dx, dy; its colours, fonts and
// sizes — the layer's (the editor's). Layer numbers: rtime, rname, rsong — font, color, spacing; rname — font2 (a long
// name, three lines), p — how wide; rsong — p: how wide; rbtn — font (the word), text, icon (gl_ic24), color, spacing,
// k (the icon's size), p2 — what it opens (0 — the stations, 1 — the volume); rline — color, p: how long; rrays —
// color (the rays), color2 (loud), color3 (the loudest), k (how long), p2 — sides (0 — both, 1 — the left, 2 — the
// right); rplay — color (the button), color2 (its word)
#define RP_BTN_Y ((RP_BTN_IC_Y + RP_BTN_T_Y) / 2)  // a button's place: between its icon and its word
#define RP_PLAY_Y (CX + 227 - RP_EDGE_H / 2)       // Play / Stop: the middle of the bottom-edge button
struct RdPart {
  HaloItem it[3];
  Press press;                  // a button, Play / Stop
  float mid;                    // the middle of its letters (its font: halo_text_mid)
  char line[3][RD_NAME + 8];    // the name: its lines …
  int lines;
  const HaloFont *f;            // … in this font (the layer's big or small one); the song: its font
  uint32_t gen;                 // … for this rp_name_gen; the song: its width for this rp_song_gen
  float tw;                     // the song: its text's width in its font
  float x, y, w, a;             // the song: the middle of its line, how wide, how seen
  float ww, ww1;                // the name: the widths it was wrapped in (its lines; its top line)
  float drawn[2][RD_BANDS];     // the rays: what the band shows now (each side)
  uint16_t pal[4][17];          // … their colours (soft, hot, top, off) by coverage, for this frame's view
  float dmg[4];                 // what it draws itself: where it was damaged last
  uint32_t dmg_h;
  HaloView v;                   // … the view it was laid out with (drawn with it)
};
static bool face_over_others(const FaceScreen &S, int i);  // (face.h: over another layer — blended, not on black)
static void rd_part_free(FaceRun &R) {
  if (R.rd) R.rd->~RdPart(), heap_caps_free(R.rd), R.rd = nullptr;
}
static void radio_part_build(const FaceScreen &S, int i) {
  FaceRun &R = S.run[i];
  void *m = heap_caps_calloc(1, sizeof(RdPart), MALLOC_CAP_SPIRAM);
  if (m) R.rd = new (m) RdPart();
}
// Each part's place at rest (what the layer's dx, dy are counted from; the editor's studio_bases)
static void radio_part_base(int k, float &x, float &y) {
  x = CX;
  y = k == FL_RTIME ? RP_TIME_Y : k == FL_RNAME ? RP_NAME_Y : k == FL_RSONG ? RP_SONG_Y : k == FL_RBTN ? RP_BTN_Y
    : k == FL_RLINE ? RP_DIV_Y : k == FL_RPLAY ? RP_PLAY_Y : CX;
}

// ---------- The main screen: the finger (on top and still) ----------
static const HaloScreen *rb_screen();
static int rp_armed_i = -1;  // the part pressed (its layer)
static void radio_enter() {
  rp_armed = rp_armed_i = -1;
  vol_enter();
}
static void radio_update(const FaceScreen &S, float dt) {
  if (!ru) return;
  HaloFinger &f = halo_finger;
  if (vol_update(dt)) return void(rp_armed = rp_armed_i = -1);  // the volume screen: "+", "−"; elsewhere or 3 s — closed
  // A swipe left / right — the next / previous station of the dial (lib/Halo keeps the left edge's for back)
  if (f.drag_start && f.horiz && rd_list_n > 1 && halo_finger_claim(&rp_owner)) rp_drag = true, rp_armed = rp_armed_i = -1;
  if (rp_drag) {
    if (f.down && f.owner == &rp_owner) {
      rp_drag_x = (f.x - f.x0) * 0.4f;
    } else {
      rp_drag = false;
      const float dx = f.x - f.x0;
      if (fabsf(dx) > 40 || fabsf(f.vx) > 500) {
        const int dir = (dx != 0 ? dx : -f.vx) < 0 ? 1 : -1, at = rd_at >= 0 ? rd_at : 0;
        rd_tune(rd_mod(at + dir, rd_list_n), dir);
        rp_slide_s.x = dir * fminf(1, fabsf(rp_drag_x) / 60 + 0.6f);
      } else {
        rp_slide_s = {rp_drag_x / 60, 0, 0};
      }
      rp_drag_x = 0;
    }
  }
  // A press: the topmost part under it — a button (its icon's top to under its word, 58 px either side of its middle;
  // what it opens — its p2) or Play / Stop (the bottom-edge button)
  if (f.pressed && !f.owner) {
    rp_armed = rp_armed_i = -1, rp_long_done = false;
    for (int i = S.n - 1; i >= 0 && rp_armed < 0; i--) {
      const FaceLayer &L = S.layers[i];
      const FaceRun &R = S.run[i];
      if (L.hidden || !R.rd) continue;
      if (L.kind == FL_RBTN && fabsf(f.x0 - R.ax) < 58 * fmaxf(1, L.k) && fabsf(f.y0 - R.ay) < 34 * fmaxf(1, L.k))
        rp_armed = L.p2 > 0.5f ? 2 : 0, rp_armed_i = i;
      else if (L.kind == FL_RPLAY && f.y0 > W - RP_EDGE_H - 6 && fabsf(f.x0 - CX) < 150) rp_armed = 1, rp_armed_i = i;
    }
  }
  if (f.dragging) rp_armed = rp_armed_i = -1;
  if (rp_armed == 1 && f.down && !rp_long_done && halo_ms() - f.t0 > 550) rp_long_done = true, rp_armed = rp_armed_i = -1, vol_open();  // (long — the volume)
  if (f.tap && !rp_long_done) {
    if (rp_armed == 0) {  // the project's Stations screen (the button's link: face.h opens it), else the built-in one
      rd_main_depth = halo_nav_depth();
      const FaceLayer &L = S.layers[rp_armed_i];
      if (L.tap > 0 && L.tap < face_scr_n) gl_req = &L, gl_req_depth = rd_main_depth;
      else halo_nav_open(*rb_screen());
    }
    else if (rp_armed == 1) rd_toggle();
    else if (rp_armed == 2) vol_open();
  }
  for (int i = 0; i < S.n; i++)
    if (S.run[i].rd && (S.layers[i].kind == FL_RBTN || S.layers[i].kind == FL_RPLAY))
      press_update(S.run[i].rd->press, i == rp_armed_i && f.down, dt);
  if (!f.down) rp_armed = rp_armed_i = -1;
}

// ---------- The main screen: layout and drawing ----------
static float rp_ang(int side, int i) {  // band i's angle (0 — the low end, at the bottom), radians from 3 o'clock
  const float a = (RP_A_BOT + (RP_A_TOP - RP_A_BOT) * i / (RD_BANDS - 1.0f)) * (HALO_PI / 180);
  return side ? HALO_PI - a : a;
}
static float rp_cos[2][RD_BANDS], rp_sin[2][RD_BANDS];  // (once: radio_layout's first frame)
static bool rp_trig;
// The screen's own layer: the volume screen over the main one (volume.h) — the parts fade through by rp_fa
static void radio_layout(const HaloView &view) {
  if (!ru) return;
  rp_fa = vol_layout(view);
  if (!rp_trig) {
    for (int s = 0; s < 2; s++)
      for (int i = 0; i < RD_BANDS; i++) rp_cos[s][i] = cosf(rp_ang(s, i)), rp_sin[s][i] = sinf(rp_ang(s, i));
    rp_trig = true;
  }
}
static void radio_draw() {}  // (its parts draw themselves; the volume screen goes over them: face.h)

// A part it draws itself (the song, the rays) changed: its old place and the new one redraw
static void rp_damage(RdPart &P, uint32_t h, float x0, float y0, float x1, float y1) {
  const HaloView &v = P.v;
  const float r[4] = {halo_view_x(v, x0), halo_view_y(v, y0), halo_view_x(v, x1), halo_view_y(v, y1)};
  for (float q : {v.x, v.y, v.s, v.a}) h = (h ^ (uint32_t)(int32_t)(q * 512)) * 16777619u;
  if (h == P.dmg_h && !memcmp(r, P.dmg, sizeof(r))) return;
  if (P.dmg_h) halo_damage_f(P.dmg[0], P.dmg[1], P.dmg[2], P.dmg[3]);
  halo_damage_f(r[0], r[1], r[2], r[3]);
  P.dmg_h = h, memcpy(P.dmg, r, sizeof(r));
}
static void rp_box(FaceRun &R, const HaloView &v, float x0, float y0, float x1, float y1) {  // (the editor's outline)
  R.box[0] = halo_view_x(v, x0), R.box[1] = halo_view_y(v, y0), R.box[2] = halo_view_x(v, x1), R.box[3] = halo_view_y(v, y1);
}
// The marquee: still for 4 s, then 64 dp/s until the line's copy 96 dp after it comes where it began, again
static float rp_marquee(float tw, float w) {
  if (tw <= w) return 0;
  const float t = fmodf(rp_song_t, 4 + (tw + 96 * DP) / (64 * DP));
  return t < 4 ? 0 : (t - 4) * 64 * DP;
}

static void radio_part_layout(const FaceScreen &S, int i, float a, const HaloView &view) {
  FaceRun &R = S.run[i];
  const FaceLayer &L = S.layers[i];
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!R.rd || !ru) return;
  RdPart &P = *R.rd;
  HaloView v = view;
  v.a *= rp_fa;  // (the volume screen over the main one: it fades through)
  float bx, by;
  radio_part_base(L.kind, bx, by);
  const float x = bx + L.dx, y = by + L.dy;
  R.ax = x, R.ay = y;
  const HaloItem *on = face_over_others(S, i) ? nullptr : &halo_black;  // (over another part — blended)
  switch (L.kind) {
    case FL_RTIME: {
      const HaloFont &f = L.font ? *L.font : gl_ra_t24;
      if (P.f != &f) P.f = &f, P.mid = halo_text_mid(f, "0");
      gl_text_mid(P.it[0], f, rp_time, P.mid, x, y, 1, L.color, a, on);
      P.it[0].spacing = L.spacing;
      halo_view_commit(P.it[0], v), box_add(R.box, P.it[0]);
      break;
    }
    case FL_RNAME: {  // two lines of its font, else three of its small one (font2); it slides in from a swipe's side
      const HaloFont &big = L.font ? *L.font : gl_ra_n52, &small = L.font2 ? *L.font2 : gl_ra_n32;
      const float w = L.p > 0 ? L.p : RP_NAME_W, w1 = L.p2 > 0 ? L.p2 : w;  // (p2: the top line's width)
      if (P.gen != rp_name_gen || (P.f != &big && P.f != &small) || P.ww != w || P.ww1 != w1) {
        P.gen = rp_name_gen, P.f = &big, P.ww = w, P.ww1 = w1;
        if (!rp_wrap(big, rp_name_s, w1, w, P.line, 2, P.lines)) {
          P.f = &small;
          // (the small one's top line: as narrow for its letters as the big one's — the same words go down)
          if (!rp_wrap(small, rp_name_s, w1 * small.size / big.size, w, P.line, 3, P.lines)) {  // (longer: the third line takes the rest, cut)
            const char *p = rp_name_s;
            for (int k = 0; k < 2; k++) p = strstr(p, P.line[k]) ? strstr(p, P.line[k]) + strlen(P.line[k]) : p;
            while (*p == ' ') p++;
            char rest[RD_NAME + 8];
            rd_copy(rest, p, sizeof(rest));
            rd_fit(small, rest, w, P.line[2], sizeof(P.line[2])), P.lines = 3;
          }
        }
        P.mid = halo_text_mid(*P.f, "Radio");
      }
      const float lh = P.f->size * (P.f == &big ? 0.98f : 1.125f), off = rp_drag_x + rp_slide * 60, na = 1 - fminf(1, fabsf(rp_slide) * 0.9f);
      for (int k = 0; k < 3; k++) {
        gl_text_mid(P.it[k], *P.f, P.line[k], P.mid, x + off, y + (k - (P.lines - 1) / 2.0f) * lh, 1, L.color,
                    k < P.lines ? a * na : 0, on);
        P.it[k].spacing = L.spacing;
        halo_view_commit(P.it[k], v), box_add(R.box, P.it[k]);
      }
      break;
    }
    case FL_RSONG: {  // drawn by radio_part_draw (a marquee, cut to its width): its place redraws when it changes
      const HaloFont &f = L.font ? *L.font : gl_ra_t20;
      const float w = L.p > 0 ? L.p : RP_SONG_W;
      if (P.gen != rp_song_gen || P.f != &f) P.gen = rp_song_gen, P.f = &f, P.tw = halo_text_width(f, rp_song, L.spacing), P.mid = halo_text_mid(f, "Radio");
      P.x = x, P.y = y, P.w = w, P.a = a, P.v = v;
      const float mq = v.a * a > 0.004f ? rp_marquee(P.tw, w) : 0, hh = f.line / 2.0f;  // (hidden: nothing moves)
      rp_damage(P, rd_hash(rp_song) ^ (uint32_t)(int32_t)(mq * 16) ^ L.color ^ (uint32_t)(int32_t)(a * 256) ^ rn_net * 7919u,
                x - w / 2, y - hh, x + w / 2, y + hh);
      if (a > 0.004f) rp_box(R, v, x - fminf(w, P.tw) / 2, y - hh * 0.6f, x + fminf(w, P.tw) / 2, y + hh * 0.6f);
      break;
    }
    case FL_RBTN: {  // its icon over its word; pressed — both shrink a little
      const HaloFont &f = L.font ? *L.font : gl_ra_t14;
      if (P.f != &f) P.f = &f, P.mid = halo_text_mid(f, "STATIONS");
      const float p = 1 - 0.08f * halo_clamp01(P.press.s.x), k = L.k > 0.05f ? L.k : 1;
      gl_text_mid(P.it[0], gl_ic24, L.icon && *L.icon ? L.icon : RD_BARS, gl_mid_icon, x, y - 12.5f * k, 0.8f * k * p, L.color,
                  L.icon && *L.icon ? a : 0, on);
      gl_text_mid(P.it[1], f, L.text ? L.text : "", P.mid, x, y + 12.5f * f.size / 14.0f, p, L.color, a, on);
      P.it[1].spacing = L.spacing;
      for (int j = 0; j < 2; j++) halo_view_commit(P.it[j], v), box_add(R.box, P.it[j]);
      break;
    }
    case FL_RLINE: {
      HaloItem &d = P.it[0];
      d.kind = HI_RRECT, d.x = x, d.y = y, d.w = 2, d.h = L.p > 0 ? L.p : 34, d.r = 1, d.scale = 1;
      d.color = L.color, d.alpha = a, d.on_black = !on ? false : true, d.on = nullptr;
      halo_view_commit(d, v), box_add(R.box, d);
      break;
    }
    case FL_RRAYS: {  // drawn by radio_part_draw: each changed ray — its box, the whole arcs when the view moves
      P.v = v, P.a = a;
      const int sides = (int)(L.p2 + 0.5f);
      const float len = RP_LMAX * (L.k > 0.05f ? L.k : 1);
      if (v.a * a < 0.004f) {  // hidden (Browse over it, the layer hidden): nothing drawn, nothing to redraw
        for (int sd = 0; sd < 2; sd++) memcpy(P.drawn[sd], ru->lv, sizeof(ru->lv));
        rp_damage(P, 1, 0, 0, 0, 0);
        break;
      }
      for (int sd = 0; sd < 2; sd++) {
        if ((sides == 1 && sd == 0) || (sides == 2 && sd == 1)) continue;  // (sd 0 — the right side, 1 — the left)
        for (int k = 0; k < RD_BANDS; k++) {
          const float lv = ru->lv[k];
          if (fabsf(lv - P.drawn[sd][k]) < 0.004f) continue;
          const float r_in = RP_R0 - 4 - fmaxf(lv, P.drawn[sd][k]) * len, c = rp_cos[sd][k], sn = rp_sin[sd][k];
          const float ax = CX + RP_R0 * c, ay = CX + RP_R0 * sn, qx = CX + r_in * c, qy = CX + r_in * sn;
          halo_damage_f(halo_view_x(v, fminf(ax, qx) - 4), halo_view_y(v, fminf(ay, qy) - 4), halo_view_x(v, fmaxf(ax, qx) + 4),
                        halo_view_y(v, fmaxf(ay, qy) + 4));
          P.drawn[sd][k] = lv;
        }
      }
      rp_damage(P, 2166136261u ^ L.color ^ (L.color2 * 3u) ^ (L.color3 * 7u) ^ (uint32_t)(len * 16) ^ (uint32_t)sides * 977u ^ (uint32_t)(a * 256),
                5, 25, W - 5, 360);  // (the whole arcs when the view, the colours or the length change)
      const uint32_t RC[4] = {L.color, L.color2, L.color3, RD_RAY_OFF};
      for (int c = 0; c < 4; c++)
        for (int k = 0; k <= 16; k++) P.pal[c][k] = halo_565(halo_dim(RC[c], (int)(k * 16 * v.a * a)));
      rp_box(R, v, sides == 2 ? CX : 5, 25, sides == 1 ? CX : W - 5, 360);
      break;
    }
    case FL_RPLAY: {  // Stop / Play — the bottom-edge button (the gallery's: pressed — the top corners squeeze)
      const float ep = halo_clamp01(P.press.s.x), ey = W - RP_EDGE_H * 0.6f;
      const bool playing = rd_on && rd_has_cur;
      HaloItem &e = P.it[0];
      e.kind = HI_EDGE, e.x = CX, e.y = CX, e.r = 227, e.h = RP_EDGE_H, e.w = 300, e.v = 48 + (32 - 48) * ep, e.scale = 1;
      e.color = L.color, e.alpha = a, e.on_black = true, e.on = nullptr;
      gl_text_mid(P.it[1], gl_ra_ic24, playing ? RD_STOP : RD_PLAY, rd_mid_ic24, CX - 30, ey, 0.85f, L.color2, a, &e);
      gl_text_mid(P.it[2], gl_ra_t24, playing ? "Stop" : "Play", rd_mid_t24, CX + 14, ey, 1, L.color2, a, &e);
      for (int j = 0; j < 3; j++) halo_view_commit(P.it[j], v);
      box_add(R.box, e);
      R.ax = CX, R.ay = RP_PLAY_Y;
      break;
    }
  }
}
// One ray: a capsule from (ax, ay) to (bx, by), half-width h, straight into this task's part of the band; its edge
// smoothed by the distance (≈ (d² + h²) / 2h near the edge: no root per pixel)
static void HALO_HOT rp_ray(float ax, float ay, float bx, float by, float h, const uint16_t *pal) {
  const float x0 = fminf(ax, bx) - h - 1, x1 = fmaxf(ax, bx) + h + 1, y0 = fminf(ay, by) - h - 1, y1 = fmaxf(ay, by) + h + 1;
  if (!halo_hits(x0, y0, x1, y1)) return;
  const float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy, il2 = l2 > 0.01f ? 1 / l2 : 0;
  const float ady = fabsf(dy), slope = ady > 1 ? dx / dy : 0, span = ady > 1 ? (h + 1.5f) * (fabsf(dx) + ady) / ady : 1e9f;
  const float hi = (h - 0.5f) * (h - 0.5f), ho = (h + 0.5f) * (h + 0.5f), i2h = 1 / (2 * h);
  const int ya = max((int)y0, hb.y), yb = min((int)y1, hb.y + hb.rows - 1);
  for (int y = ya; y <= yb; y++) {
    const float py = y + 0.5f - ay, xc = ax + py * slope;
    const int xa = max(max((int)x0, hb.x0), (int)(xc - span)), xb = min(min((int)x1, hb.x1), (int)(xc + span) + 1);
    uint16_t *row = halo_row(y);
    for (int x = xa; x <= xb; x++) {
      const float px = x + 0.5f - ax;
      float t = (px * dx + py * dy) * il2;
      t = t < 0 ? 0 : t > 1 ? 1 : t;
      const float ex = px - t * dx, ey = py - t * dy, d2 = ex * ex + ey * ey;
      if (d2 >= ho) continue;
      if (d2 <= hi) { row[x] = pal[16]; continue; }
      const float cov = h + 0.5f - (d2 + h * h) * i2h;
      const int k = (int)(cov * 16);
      if (k > 0) row[x] = pal[k > 16 ? 16 : k];
    }
  }
}
static void rp_rays_draw(const FaceLayer &L, const RdPart &P) {
  const HaloView &v = P.v;
  if (v.a * P.a < 0.004f) return;
  const bool playing = rd_on && rn_playing;
  const int sides = (int)(L.p2 + 0.5f);
  const float h = 3 * v.s, len = RP_LMAX * (L.k > 0.05f ? L.k : 1);
  for (int s = 0; s < 2; s++) {
    if ((sides == 1 && s == 0) || (sides == 2 && s == 1)) continue;  // (s 0 — the right side, 1 — the left)
    for (int i = 0; i < RD_BANDS; i++) {
      const float lv = P.drawn[s][i], r_in = RP_R0 - 4 - lv * len, c = rp_cos[s][i], sn = rp_sin[s][i];
      const uint16_t *pal = !playing ? P.pal[3] : lv > 0.75f ? P.pal[2] : lv > 0.5f ? P.pal[1] : P.pal[0];
      rp_ray(halo_view_x(v, CX + RP_R0 * c), halo_view_y(v, CX + RP_R0 * sn), halo_view_x(v, CX + r_in * c),
             halo_view_y(v, CX + r_in * sn), h, pal);
    }
  }
}
static void rp_text_clip(const HaloFont &f, const char *s, float x, float top, float sc, uint32_t color, float xa, float xb) {
  const int sx0 = hb.x0, sx1 = hb.x1;
  hb.x0 = max(hb.x0, (int)xa), hb.x1 = min(hb.x1, (int)xb);
  if (hb.x0 <= hb.x1) halo_text(f, s, x, top, color, 1, 0, sc, 0, 0);
  hb.x0 = sx0, hb.x1 = sx1;
}
static void rp_fade_col(int x, float k, int y0, int y1) {  // the marquee's soft edge: a column toward black
  if (x < hb.x0 || x > hb.x1) return;
  for (int y = max(y0, hb.y); y <= min(y1, hb.y + hb.rows - 1); y++) halo_blend(halo_row(y) + x, 0, 0, 0, (int)((1 - k) * 256));
}
static void rp_song_draw(const FaceLayer &L, const RdPart &P) {
  const HaloView &v = P.v;
  const float al = v.a * P.a;
  if (al < 0.004f || !P.f) return;
  const bool err = rd_on && (rn_net == RN_NOWIFI || (rn_net == RN_FAILED && !rn_playing));
  const uint32_t c = halo_dim(err ? RD_ERR : L.color, (int)(al * 256));
  const float top = halo_view_y(v, P.y) - P.mid * v.s;
  const float xa = halo_view_x(v, P.x - P.w / 2), xb = halo_view_x(v, P.x + P.w / 2);
  if (P.tw > P.w) {  // two copies 96 dp apart, the edges fade over 16 px
    const float x0 = halo_view_x(v, P.x - P.w / 2 - rp_marquee(P.tw, P.w));
    rp_text_clip(*P.f, rp_song, x0, top, v.s, c, xa, xb);
    rp_text_clip(*P.f, rp_song, x0 + (P.tw + 96 * DP) * v.s, top, v.s, c, xa, xb);
    const int y0 = (int)top, y1 = (int)(top + P.f->line * v.s);
    for (int e = 0; e < 16; e++) rp_fade_col((int)xa + e, e / 16.0f, y0, y1), rp_fade_col((int)xb - 1 - e, e / 16.0f, y0, y1);
  } else {
    halo_text(*P.f, rp_song, halo_view_x(v, P.x), top, c, 1, 1, v.s, L.spacing, 0);
  }
}
static void radio_part_draw(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (!R.rd || !ru) return;
  const FaceLayer &L = S.layers[i];
  const RdPart &P = *R.rd;
  if (L.kind == FL_RRAYS) return rp_rays_draw(L, P);
  if (L.kind == FL_RSONG) return rp_song_draw(L, P);
  const int n = L.kind == FL_RNAME || L.kind == FL_RPLAY ? 3 : L.kind == FL_RBTN ? 2 : 1;
  for (int j = 0; j < n; j++) halo_item_draw(P.it[j]);
}

// ---------- Eq bars (a station on): three, on a colour ----------
// Still (stopped, can't connect), live (the music: each its own beat), loading (joining, connecting, buffering: a wave —
// one bar after another rises and falls, 13 Nov 2026, the user's wish: the same bars, an animation of their own)
enum : uint8_t { RD_EQ_STILL, RD_EQ_LIVE, RD_EQ_LOAD };
static uint8_t rd_eq_mode() {
  if (!rd_on) return RD_EQ_STILL;
  if (rn_playing) return RD_EQ_LIVE;
  return rn_net == RN_NOWIFI || rn_net == RN_FAILED ? RD_EQ_STILL : RD_EQ_LOAD;
}
// How much the bars are seen over a card's letters or icon: they fade in over 300 ms since the station started
static float rd_eq_fade() {
  return halo_clamp01((halo_ms() - rd_start_ms) / 300.0f);
}
static void rd_eq(HaloItem *b, float x, float y, float h, float w, uint32_t c, float a, const HaloView &v, uint8_t mode) {
  for (int i = 0; i < 3; i++) {
    const float wave = sinf(rd_clock * 7 - i * 1.3f);
    const float k = mode == RD_EQ_LIVE ? 0.35f + 0.65f * fabsf(sinf(rd_clock * (6.3f + i * 2.1f) + i * 1.7f))
                  : mode == RD_EQ_LOAD ? 0.3f + 0.7f * (wave > 0 ? wave : 0) : 0.3f;
    HaloItem &it = b[i];
    it.kind = HI_RRECT, it.x = x + (i - 1) * w * 1.7f, it.y = y + h / 2 - h * k / 2, it.w = w, it.h = h * k, it.r = w / 2;
    it.scale = 1, it.color = c, it.alpha = a, it.on_black = false, it.on = nullptr;
    halo_view_commit(it, v);
  }
}
// The same bars in a card's round icon (64 px) — the playing station's (or folder's) in place of its letters: in its
// middle, 24 px tall at most, standing on a line 12 px below the middle (13 Nov 2026, the user's wish: the heart stays
// on the right of every card)
static void rd_eq_circle(HaloItem *b, float cx, float cy, float s, uint32_t c, float a, const HaloView &v, uint8_t mode) {
  rd_eq(b, cx, cy, 24 * s, 5 * s, c, a, v, mode);
}
// The time at the top of a list: it goes up and fades over the first 36 dp of scrolling (as the gallery's)
static void rd_list_time(HaloItem &it, const HaloList &l, const HaloView &v) {
  const float k = 1 - halo_clamp01((l.top - l.scroll) / (36 * DP));
  gl_text_mid(it, gl_ra_t24, rp_time, rd_mid_t24, CX, RP_TIME_Y - 24 * (1 - k), 0.7f + 0.3f * k, RD_TEXT, k, &halo_black);
  halo_view_commit(it, v);
}
// A gallery card: the plate (pressed — less round)
static void rd_card(HaloItem &bg, float cy, float s, float p, float a, uint32_t col, const HaloView &v) {
  bg.kind = HI_RRECT, bg.x = CX, bg.y = cy, bg.w = 410, bg.h = 104, bg.r = 52 + (36 - 52) * p, bg.scale = s;
  bg.color = col, bg.alpha = a, bg.on_black = true, bg.on = nullptr;
  halo_view_commit(bg, v);
}

// ---------- The bottom-edge button with what plays: the gallery's (gallery.h: gal_rebuild, gal_update, FL_EDGE) ----------
// By the number of cards, as there: one or none — no button (and none before anything has played); two — full height,
// still; three and more — it grows as the list reaches its end: its height follows the scroll (full in the last 92 px),
// a jump of more than 16 dp without scrolling goes by a 400/1 spring; under 30 dp its background fades, under 38 dp its
// text. The list's last card ends 8 px above it (without it — at 420, as Browse always did)
static uint8_t rd_edge_mode(int cards) {
  return !rd_has_cur || cards <= 1 ? EB_NONE : cards == 2 ? EB_STILL : EB_GROWS;
}
static float rd_list_end(uint8_t mode) {
  return mode == EB_NONE ? 420 : CX + RP_EDGE_R - RP_EDGE_H - 8;
}
static float rd_edge_height(const RdEdge &e, const HaloList &l) {
  if (e.mode == EB_NONE) return 0;
  if (e.mode == EB_STILL) return RP_EDGE_H;
  return halo_clamp01(1 - (l.scroll - halo_list_min(l)) / RP_EDGE_H) * RP_EDGE_H;
}
// After halo_list_init: still — the list stands where it ends (and doesn't scroll); the button's height as the list is
static void rd_edge_reset(RdEdge &e, HaloList &l, uint8_t mode) {
  e.mode = mode, e.armed = false;
  if (l.n && mode == EB_STILL && halo_list_min(l) < l.top) l.top = halo_list_min(l), halo_list_reset(l);
  halo_spring_set(e.h, rd_edge_height(e, l));
  halo_spring_set(e.press.s, 0), e.press.going_down = false;
}
static void rd_edge_update(RdEdge &e, const HaloList &l, float dt) {
  const HaloFinger &f = halo_finger;
  e.h.target = rd_edge_height(e, l);
  const bool scrolling = f.owner == &l || l.flinging || l.back_on;
  if (e.mode != EB_GROWS || scrolling || fabsf(e.h.target - e.h.x) <= 16 * DP) e.h.x = e.h.target, e.h.v = 0;
  halo_spring_run(e.h, K_EB, dt);
}
static bool rd_edge_hit(const RdEdge &e, const HaloFinger &f) {  // (it is over the list: it wins the press)
  return e.mode != EB_NONE && e.h.x > 60 && f.y0 > CX + RP_EDGE_R - e.h.x && fabsf(f.x0 - CX) < 150;
}
static void rd_edge_layout(RdEdge &e, const HaloView &v) {
  const float eh = e.h.x, ep = halo_clamp01(e.press.s.x), top = CX + RP_EDGE_R - eh;
  const float a_bg = eh > 1 ? halo_clamp01((eh - 4 * DP) / (26 * DP)) : 0, a_t = eh > 1 ? halo_clamp01((eh - 30 * DP) / (8 * DP)) : 0;
  HaloItem &b = e.bg;  // pressed — the top corners squeeze
  b.kind = HI_EDGE, b.x = CX, b.y = CX, b.r = RP_EDGE_R, b.h = eh, b.w = 300, b.v = 48 + (32 - 48) * ep, b.scale = 1;
  b.color = RD_ORANGE, b.alpha = a_bg, b.on_black = true, b.on = nullptr;
  halo_view_commit(b, v);
  const float ey = top + fminf(eh, RP_EDGE_H) * 0.465f;  // (full height: where the main screen's Stop has its text)
  rd_fit(gl_ra_t24, rd_has_cur ? rd_cur.name : "", 186, rp_edge_s, sizeof(rp_edge_s));
  const float tw = halo_text_width(gl_ra_t24, rp_edge_s);
  gl_text_mid(e.t, gl_ra_t24, rp_edge_s, rd_mid_t24, CX + 14, ey, 1, RD_ON_ORANGE, a_t, &b);
  halo_view_commit(e.t, v);
  rd_eq(e.eq, CX + 14 - tw / 2 - 20, ey - 9, 18, 4, RD_ON_ORANGE, a_t, v, rd_eq_mode());
}
static void rd_edge_draw(const RdEdge &e) {
  halo_item_draw(e.bg), halo_item_draw(e.t);
  for (const auto &q : e.eq) halo_item_draw(q);
}

// ---------- Browse: the catalog's folders, the gallery's cards ----------
static void rb_enter() {
  float hs[HALO_LIST_MAX];
  hs[0] = 64;
  const int n = min(RD_FOLDER_N, min(RD_FOLDER_MAX, HALO_LIST_MAX - 1));
  for (int i = 0; i < n; i++) hs[i + 1] = 104;
  const uint8_t mode = rd_edge_mode(n);
  halo_list_init(ru->b_list, hs, n + 1, 64, rd_list_end(mode), true);
  rd_edge_reset(ru->b_edge, ru->b_list, mode);
  rb_armed = -1;
}
static const HaloScreen *rs_screen();
// The Stations screen's own layer (FL_RBROWSE) while it is the project's screen: its link opens the Folder screen
// (nullptr — the built-in Browse of a project from before 13 Nov 2026: the built-in stations screen)
static const FaceLayer *rb_link;
static void rs_open(int folder) {
  RdUi &u = *ru;
  u.s_folder = folder, u.s_n = 0, u.s_state = 0;
  const RdFolder &f = RD_FOLDER[folder];
  if (f.kind == RF_FAV) {
    u.s_n = min(rd_fav_n, RD_RES), u.s_state = 1;
    memcpy(u.s_rows, rd_fav, u.s_n * sizeof(RdSt));
  } else {
    char arg[60] = "";
    const char *src = f.arg;  // (%-encoded). Plain http only: HTTPS needs ~47 KB of internal memory the board lacks
    for (int o = 0; *src && o < (int)sizeof(arg) - 4; src++) {
      const uint8_t ch = *src;
      if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.') arg[o++] = ch;
      else o += snprintf(arg + o, sizeof(arg) - o, "%%%02X", ch);
    }
    snprintf(u.s_path, sizeof(u.s_path), "/json/stations/search?codec=MP3&is_https=false&hidebroken=true&order=clickcount&reverse=true&limit=%d%s%s",
             RD_RES + 6, f.kind == RF_TAG ? "&tag=" : f.kind == RF_CC ? "&countrycode=" : "", f.kind == RF_TOP ? "" : arg);
    u.s_gen = rn_fetch_gen;
    rn_fetch(u.s_path, folder);
  }
  if (rb_link && rb_link->tap > 0 && rb_link->tap < face_scr_n) gl_req = rb_link, gl_req_depth = halo_nav_depth();
  else halo_nav_open(*rs_screen());
}
static void rb_update(float dt) {
  RdUi &u = *ru;
  HaloFinger &f = halo_finger;
  HaloList &l = u.b_list;
  halo_list_update(l, dt);
  RdEdge &e = u.b_edge;
  rd_edge_update(e, l, dt);
  if (f.pressed) {
    const int i = halo_list_hit(l, f.y0);
    rb_armed = i > 0 && fabsf(f.x0 - CX) < 205 * l.fx[i].s ? i : -1;
    e.armed = rd_edge_hit(e, f);
    if (e.armed) rb_armed = -1;
  }
  if (f.dragging) rb_armed = -1, e.armed = false;
  if (f.tap && e.armed) halo_nav_back_to(rd_main_depth);
  else if (f.tap && rb_armed > 0) rs_open(rb_armed - 1);
  for (int i = 0; i < l.n; i++) press_update(u.b_press[i], i == rb_armed && f.down, dt);
  press_update(e.press, e.armed && f.down, dt);
  if (!f.down) rb_armed = -1, e.armed = false;
}
// Opening over another screen, a list is not drawn while it would still be blended over it — lib/Halo's first frame
// of the transition (a ≈ 0.14, its black circle under it not opaque yet: halo_nav.cpp, cover). Then every pixel of the
// cards was blended over the main screen: ~10 ms more on the right core, and that frame missed its pass while the
// stream played (13 Nov 2026). From the second frame (a ≈ 0.37) it lies on the black circle — dimmed colours, cheap.
// A frame of 14 % less is not seen
static HaloView rd_list_view(const HaloView &v) {
  HaloView o = v;
  if (o.over && o.a < 0.25f) o.a = 0;
  return o;
}
static void rb_layout(const HaloView &view) {
  const HaloView v = rd_list_view(view);
  RdUi &u = *ru;
  HaloList &l = u.b_list;
  rd_list_time(u.b_time, l, v);
  const HaloListFx &hd = l.fx[0];
  gl_text_mid(u.b_head, gl_ra_n32, "Radio", rd_mid_n32, CX, hd.top + 32 * hd.s, hd.s, RD_TEXT, hd.a_bg, &halo_black);
  halo_view_commit(u.b_head, v);
  bool eq = false;
  rb_v0 = RD_FOLDER_MAX, rb_v1 = 0;
  for (int i = 0; i < RD_FOLDER_N && i < RD_FOLDER_MAX && i + 1 < l.n; i++) {
    const RdFolder &fd = RD_FOLDER[i];
    const HaloListFx &fx = l.fx[i + 1];
    const bool vis = halo_list_item_visible(l, i + 1), cur = i == rd_folder;
    if (vis) rb_v0 = min(rb_v0, i), rb_v1 = i + 1;
    const float s = fx.s, x0 = CX - 205 * s, y0 = fx.top, cy = y0 + 52 * s, ac = vis ? fx.a_c : 0;
    rd_card(u.b_card[i], cy, s, halo_clamp01(u.b_press[i + 1].s.x), vis ? fx.a_bg : 0, RD_CARD, v);
    HaloItem &dc = u.b_dot[i];
    dc.kind = HI_CIRCLE, dc.x = x0 + 56 * s, dc.y = cy, dc.r = 32, dc.scale = s, dc.color = fd.col, dc.alpha = ac;
    dc.on_black = true, dc.on = nullptr;
    const bool eqc = cur && rd_on;  // the folder on the dial, the radio on: the equalizer in its circle, not its icon
    const float ef = eqc ? rd_eq_fade() : 0;
    gl_text_mid(u.b_ic[i], gl_ra_ic24, RD_FIC[fd.icon % 5], rd_mid_ic24, x0 + 56 * s, cy, s * 1.08f, 0x1B1B1F,
                ac * (1 - ef), &dc);
    gl_text_base(u.b_text[i], gl_ra_t24, fd.name, x0 + 104 * s, y0 + 46 * s, s, RD_TEXT, ac, &u.b_card[i]);
    const char *sub = fd.kind == RF_TOP ? "most listened" : fd.kind == RF_TAG ? "genre" : fd.kind == RF_CC ? "country" : nullptr;
    if (cur) snprintf(u.b_sub_s[i], sizeof(u.b_sub_s[i]), "on the dial%s", rd_on && rn_playing ? " \xC2\xB7 playing" : rd_eq_mode() == RD_EQ_LOAD ? " \xC2\xB7 buffering" : "");
    else if (!sub) snprintf(u.b_sub_s[i], sizeof(u.b_sub_s[i]), "%d station%s", rd_fav_n, rd_fav_n == 1 ? "" : "s");
    else strlcpy(u.b_sub_s[i], sub, sizeof(u.b_sub_s[i]));
    gl_text_base(u.b_sub[i], gl_ra_t20, u.b_sub_s[i], x0 + 104 * s, y0 + 80 * s, s, RD_DIM, ac, &u.b_card[i]);
    for (HaloItem *it : {&dc, &u.b_ic[i], &u.b_text[i], &u.b_sub[i]}) halo_view_commit(*it, v);
    if (eqc) eq = true, rd_eq_circle(u.b_eq, x0 + 56 * s, cy, s, 0x1B1B1F, ac * ef, v, rd_eq_mode());
  }
  if (!eq)
    for (auto &e : u.b_eq) e.alpha = 0, halo_view_commit(e, v);
  rd_edge_layout(u.b_edge, v);
}
static void rb_draw() {
  RdUi &u = *ru;
  halo_item_draw(u.b_time), halo_item_draw(u.b_head);
  for (int i = rb_v0; i < rb_v1; i++)  // (only the cards seen: the others are hidden)
    halo_item_draw(u.b_card[i]), halo_item_draw(u.b_dot[i]), halo_item_draw(u.b_ic[i]), halo_item_draw(u.b_text[i]),
        halo_item_draw(u.b_sub[i]);
  for (const auto &e : u.b_eq) halo_item_draw(e);
  rd_edge_draw(u.b_edge);
}
static const HaloScreen *rb_screen() {
  // Not opaque: the cards lie on black (no background of its own) — so lib/Halo puts its black circle under it as it
  // opens and doesn't draw the main screen there. Opaque, the main screen (the rays, the big name) showed through the
  // fading-in cards and was blended under every pixel: 13–17 ms a half for the first 12 frames (13 Nov 2026)
  static const HaloScreen S = {rb_enter, [](float dt) { rb_link = nullptr, rb_update(dt); }, rb_layout, rb_draw, false, HALO_HOME_BOTH};
  return &S;
}

// ---------- A folder's stations, the gallery's cards; the bottom-edge button — what plays ----------
static void rs_rows_init() {
  RdUi &u = *ru;
  float hs[HALO_LIST_MAX];
  hs[0] = 64;
  const int n = u.s_state == 1 ? u.s_n : 0;
  for (int i = 0; i < n; i++) hs[i + 1] = 104;
  const uint8_t mode = rd_edge_mode(n);
  halo_list_init(u.s_list, hs, n + 1, 64, rd_list_end(mode), true);
  rd_edge_reset(u.s_edge, u.s_list, mode);
  for (int i = 0; i < n; i++) {
    const RdSt &s = u.s_rows[i];
    rd_fit(gl_ra_t24, s.name, 226, u.s_name_s[i], sizeof(u.s_name_s[i]));
    snprintf(u.s_sub_s[i], sizeof(u.s_sub_s[i]), "%s%sMP3 %d", s.cc, s.cc[0] ? " \xC2\xB7 " : "", s.kbps);
  }
}
static void rs_enter() {
  rs_armed = rs_heart = -1, rs_btn_armed = false;
  rs_rows_init();
}
static void rs_take_fetch() {  // the catalog answered: its stations into the rows
  RdUi &u = *ru;
  if (u.s_state != 0 || rn_fetch_gen == u.s_gen || rn_fetch_st == 1) return;
  u.s_gen = rn_fetch_gen;
  if (rn_fetch_st == 2) {
    u.s_n = min((int)rn_res_n, RD_RES);
    memcpy(u.s_rows, rn_res, u.s_n * sizeof(RdSt));
    u.s_state = 1;
  } else {
    u.s_state = 2;
  }
  rs_rows_init();
}
// A station of the folder tapped: the folder onto the dial, it plays — the list stays (its circle shows it loading, then
// playing); the one already playing tapped again — the main screen (13 Nov 2026, the user's wish; before: always back)
static void rs_pick(int i) {
  RdUi &u = *ru;
  if (rd_has_cur && rd_on && !strcmp(rd_cur.url, u.s_rows[i].url)) {
    if (rn_playing) halo_nav_back_to(rd_main_depth);  // (still loading: nothing)
    return;
  }
  rd_folder = u.s_folder;
  if (RD_FOLDER[rd_folder].kind == RF_FAV) {
    rd_list_favs();
    for (int k = 0; k < rd_list_n; k++)
      if (!strcmp(rd_list[k].url, u.s_rows[i].url)) {
        rd_tune(k);
        break;
      }
  } else {
    rd_list_n = u.s_n;
    memcpy(rd_list, u.s_rows, u.s_n * sizeof(RdSt));
    rd_tune(i);
  }
}
static void rs_update(float dt) {
  RdUi &u = *ru;
  HaloFinger &f = halo_finger;
  HaloList &l = u.s_list;
  rs_take_fetch();
  halo_list_update(l, dt);
  RdEdge &e = u.s_edge;
  rd_edge_update(e, l, dt);
  if (f.pressed) {
    rs_armed = rs_heart = -1, rs_btn_armed = false;
    e.armed = rd_edge_hit(e, f);
    if (e.armed) (void)0;  // (the button is over the list: it wins the press)
    else if (u.s_state == 2 && fabsf(f.y0 - 274) < 26 && fabsf(f.x0 - CX) < 62) rs_btn_armed = true;
    else {
      const int i = halo_list_hit(l, f.y0);
      const float s = i >= 0 ? l.fx[i].s : 1;
      if (i > 0 && fabsf(f.x0 - CX) < 205 * s) {
        if (f.x0 > CX + (205 - 84) * s) rs_heart = i - 1;
        else rs_armed = i;
      }
    }
  }
  if (f.dragging) rs_armed = rs_heart = -1, rs_btn_armed = e.armed = false;
  if (f.tap) {
    if (e.armed) halo_nav_back_to(rd_main_depth);
    else if (rs_btn_armed) {  // Retry: ask the catalog once more
      u.s_state = 0, u.s_gen = rn_fetch_gen;
      rn_fetch(u.s_path, u.s_folder);
      rs_rows_init();
    } else if (rs_heart >= 0 && rs_heart < u.s_n) rd_fav_toggle(u.s_rows[rs_heart]);
    else if (rs_armed > 0 && rs_armed - 1 < u.s_n) rs_pick(rs_armed - 1);
  }
  for (int i = 0; i < l.n; i++) press_update(u.s_press[i], i == rs_armed && f.down, dt);
  press_update(e.press, e.armed && f.down, dt);
  if (!f.down) rs_armed = rs_heart = -1, rs_btn_armed = e.armed = false;
}
static void rs_layout(const HaloView &view) {
  const HaloView v = rd_list_view(view);
  RdUi &u = *ru;
  HaloList &l = u.s_list;
  const RdFolder &fd = RD_FOLDER[u.s_folder];
  rd_list_time(u.s_time, l, v);
  const HaloListFx &hd = l.fx[0];
  gl_text_mid(u.s_head, gl_ra_n32, fd.name, rd_mid_n32, CX, hd.top + 32 * hd.s, hd.s, RD_TEXT, hd.a_bg, &halo_black);
  halo_view_commit(u.s_head, v);
  const int n = u.s_state == 1 ? u.s_n : 0;
  bool eq = false;
  rs_v0 = RD_RES, rs_v1 = 0;
  for (int i = 0; i < RD_RES; i++) {
    HaloItem *its[6] = {&u.s_card[i], &u.s_logo[i], &u.s_mono[i], &u.s_name[i], &u.s_sub[i], &u.s_heart[i]};
    // Not seen — hidden, and nothing else: the URLs (PSRAM) against the playing one and the favourites — only for the
    // cards seen (23 × 17 comparisons a frame were)
    if (i >= n || i + 1 >= l.n || !halo_list_item_visible(l, i + 1)) {
      for (HaloItem *it : its) it->alpha = 0, halo_view_commit(*it, v);
      continue;
    }
    rs_v0 = min(rs_v0, i), rs_v1 = i + 1;
    const RdSt &s = u.s_rows[i];
    const HaloListFx &fx = l.fx[i + 1];
    const float sc = fx.s, x0 = CX - 205 * sc, y0 = fx.top, cy = y0 + 52 * sc, ac = fx.a_c;
    const bool cur = rd_has_cur && !strcmp(rd_cur.url, s.url), fav = rd_fav_find(s.url) >= 0;
    rd_card(u.s_card[i], cy, sc, halo_clamp01(u.s_press[i + 1].s.x), fx.a_bg, RD_CARD, v);
    HaloItem &lg = u.s_logo[i];
    lg.kind = HI_CIRCLE, lg.x = x0 + 56 * sc, lg.y = cy, lg.r = 32, lg.scale = sc, lg.color = s.col, lg.alpha = ac;
    lg.on_black = true, lg.on = nullptr;
    const int ml = strlen(s.mono);
    const bool big = ml <= 2 || (ml <= 4 && (uint8_t)s.mono[0] >= 0x80);  // (two letters, Latin or not)
    const bool eqc = cur && rd_on;  // the station on, the radio on: the equalizer in its circle, not its letters
    const float ef = eqc ? rd_eq_fade() : 0;
    gl_text_mid(u.s_mono[i], big ? gl_ra_t24 : gl_ra_t14, s.mono, big ? rd_mid_t24 : rd_mid_t14, x0 + 56 * sc, cy, sc,
                rd_ink(s.col), ac * (1 - ef), &lg);
    gl_text_base(u.s_name[i], gl_ra_t24, u.s_name_s[i], x0 + 104 * sc, y0 + 46 * sc, sc, cur ? rd_light(s.col) : RD_TEXT, ac, &u.s_card[i]);
    gl_text_base(u.s_sub[i], gl_ra_t20, u.s_sub_s[i], x0 + 104 * sc, y0 + 80 * sc, sc, RD_DIM, ac, &u.s_card[i]);
    const bool pop = fav && rd_pop_url == rd_hash(s.url) && halo_ms() - rd_pop_at < 450;
    const float bounce = pop ? 1 + 0.4f * sinf((halo_ms() - rd_pop_at) / 450.0f * HALO_PI) : 1;
    gl_text_mid(u.s_heart[i], gl_ra_ic24, RD_HEART, rd_mid_ic24, x0 + 362 * sc, cy, sc * 1.08f * bounce,
                fav ? RD_PINK : 0x55555B, ac, &u.s_card[i]);  // (the heart on every card: the playing one's too)
    for (HaloItem *it : its) halo_view_commit(*it, v);
    if (eqc) eq = true, rd_eq_circle(u.s_eq, x0 + 56 * sc, cy, sc, rd_ink(s.col), ac * ef, v, rd_eq_mode());
  }
  if (!eq)
    for (auto &e : u.s_eq) e.alpha = 0, halo_view_commit(e, v);
  // Loading: three grey cards, breathing; failed: a word and Retry; empty: a word
  const float br = 0.55f + 0.45f * sinf(rd_clock * 5);
  for (int r = 0; r < 3; r++) {
    const float cy = 128 + 52 + r * 112, x0 = CX - 205;
    for (int k = 0; k < 4; k++) {
      HaloItem &it = u.s_shim[r][k];
      it.kind = k == 1 ? HI_CIRCLE : HI_RRECT, it.scale = 1, it.on_black = true, it.on = nullptr;
      it.color = k ? 0x3A3C41 : 0x1F2023, it.alpha = u.s_state == 0 ? (k ? br : 1) : 0;
      if (!k) it.x = CX, it.y = cy, it.w = 410, it.h = 104, it.r = 52;
      else if (k == 1) it.x = x0 + 56, it.y = cy, it.r = 32;
      else it.x = x0 + 104 + (k == 2 ? 90 - r * 10 : 55), it.y = cy + (k == 2 ? -12 : 16), it.w = k == 2 ? 180 - r * 20 : 110, it.h = k == 2 ? 16 : 12, it.r = it.h / 2;
      halo_view_commit(it, v);
    }
  }
  const char *msg = u.s_state == 2 ? (rn_net == RN_NOWIFI ? "No Wi-Fi" : "No connection")
                    : u.s_state == 1 && !u.s_n ? (fd.kind == RF_FAV ? "No favorites yet" : "Nothing here") : "";
  gl_text_mid(u.s_msg, gl_ra_t24, msg, rd_mid_t24, CX, 214, 1, RD_DIM, *msg ? 1 : 0, &halo_black);
  halo_view_commit(u.s_msg, v);
  HaloItem &rb = u.s_btn;
  rb.kind = HI_RRECT, rb.x = CX, rb.y = 274, rb.w = 124, rb.h = 52, rb.r = 26, rb.scale = 1, rb.color = RD_CARD;
  rb.alpha = u.s_state == 2 ? 1 : 0, rb.on_black = true, rb.on = nullptr;
  gl_text_mid(u.s_btn_t, gl_ra_t20, "Retry", rd_mid_t20, CX, 274, 1, RD_TEXT, rb.alpha, &rb);
  halo_view_commit(rb, v), halo_view_commit(u.s_btn_t, v);
  rd_edge_layout(u.s_edge, v);  // what plays (a tap — back to the main screen)
}
static void rs_draw() {
  RdUi &u = *ru;
  halo_item_draw(u.s_time), halo_item_draw(u.s_head);
  for (int i = rs_v0; i < rs_v1; i++)  // (only the cards seen: the others are hidden)
    halo_item_draw(u.s_card[i]), halo_item_draw(u.s_logo[i]), halo_item_draw(u.s_mono[i]), halo_item_draw(u.s_name[i]),
        halo_item_draw(u.s_sub[i]), halo_item_draw(u.s_heart[i]);
  for (const auto &e : u.s_eq) halo_item_draw(e);
  if (u.s_state == 0)  // (loading — the grey cards; otherwise hidden)
    for (auto &r : u.s_shim)
      for (auto &it : r) halo_item_draw(it);
  if (u.s_state != 1 || !u.s_n) halo_item_draw(u.s_msg), halo_item_draw(u.s_btn), halo_item_draw(u.s_btn_t);
  rd_edge_draw(u.s_edge);
}
static const HaloScreen *rs_screen() {
  static const HaloScreen S = {rs_enter, rs_update, rs_layout, rs_draw, false, HALO_HOME_BOTH};  // (as Browse's)
  return &S;
}

// ---------- The Radio's other screens as the project's own (FL_RBROWSE, FL_RFOLDER, FL_RVOLUME: settings.h) ----------
// The Stations screen (the catalog's folders) and the Folder screen (a folder's stations) — Browse and the stations
// above, run by their layer; the editor's map shows them at rest without opening them (no enter there): their lists
// made at the first layout — the Folder screen shows the favourites
static void rb_update_on(const FaceScreen &S, float dt) {
  rb_link = nullptr;
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_RBROWSE) rb_link = &S.layers[i];
  rb_update(dt);
}
static void rb_layout_on(const HaloView &v) {
  if (!ru) return;
  if (!ru->b_list.n) rb_enter();
  rb_layout(v);
}
static void rs_layout_on(const HaloView &v) {
  if (!ru) return;
  if (!ru->s_list.n) {  // (not opened from a folder: the favourites)
    RdUi &u = *ru;
    u.s_folder = 0, u.s_n = min(rd_fav_n, RD_RES), u.s_state = 1;
    memcpy(u.s_rows, rd_fav, u.s_n * sizeof(RdSt));
    rs_rows_init();
  }
  rs_layout(v);
}
// The volume screen as a screen: opened (Try's way to it) — up, back by itself when it closes; on the map (never
// opened) — shown up as it is
static bool rv_live;
static void rv_enter() {
  vol_enter(), vol_open();
  rv_live = true;
}
static void rv_update(float dt) {
  if (rv_live && !vol_update(dt)) rv_live = false, halo_nav_back();
}
static void rv_layout(const HaloView &v) {
  if (rv_live) return (void)vol_layout(v);
  const HaloSpring keep = vol_screen;  // (the map: shown up; the main screen's own volume screen left as it was)
  halo_spring_set(vol_screen, 1);
  vol_layout(v);
  vol_screen = keep;
}
