// radio_rect.h — the Radio on a rectangular screen: variant 10 of web/mockups/radio-touch.html, "two knobs, as on a
// radio set" (the user's pick, 15 Nov 2026, after variant 4 of radio-rect.html felt small under the finger). One screen
// in place of the round board's three (the main one, the catalog's folders, a folder's stations) and its volume screen.
// Every target is at least 10 mm (1024 x 600 at 7 inches: 6.67 px a mm), the knobs sit where two thumbs are:
//
//   * along the top — the catalog's folders as band keys (92 x 62, a lamp lit on the one looked at); a tap opens that
//     folder (the favourites at once, the others from Radio Browser); more than ten — the row scrolls under the finger;
//   * the station that plays (or played last) big, its song under it (a marquee; trouble in red); the time at the left,
//     the heart at the right (the station into the favourites);
//   * the tuning window: the folder's stations on a scale — round logos over the ticks (ten a station), the names under
//     them, an orange needle in the middle; the ticks dance to the sound (the lows in the middle), the one that plays
//     has an orange ring and the bars in its circle. Drag the window (or tap a logo) — it settles on a station and that
//     one plays (its folder goes onto the dial, as a station picked in the round board's folder);
//   * at the bottom left the volume knob (turn it: 270 degrees for 0..10, an orange arc round it), at the bottom right
//     the tuning knob (36 degrees a station: the window moves with it, let go — the station under the needle plays);
//     between them previous, Stop / Play, next — through the dial.
//
// The stations, the favourites, the dial, the stream, the song's line, the time and the sound's levels are radio.h's;
// only the screen is this board's. Its items and the rows of the folder looked at are in PSRAM: the P4's internal
// memory is Wi-Fi's and the speaker's (with Wi-Fi it runs short — settings_load takes two drawing tiles, not three).
// Included from radio.h in place of the round board's screens.
//
// The 7″ board held tall (HALO_P4_TALL, 600 x 1024: variant 10A of web/mockups/rect-tall.html) — the same parts stacked:
// the band keys in two rows of five (more than ten — both rows scroll together), the station and its song, the tuning
// window across (three stations in it), previous / Stop / next, then the two knobs (the volume left, the tuning right).
//
// The square (HALO_SQUARE, 480 x 480: variant 1B of web/mockups/square-radio.html) — no knobs: one row of band keys (it
// scrolls), the station and its song, a tall tuning window to drag, then one row of big buttons — volume −, previous,
// Stop / Play, next, volume + — and the volume as a line under them.

#if defined(HALO_WATCH)  // the rectangular watch (variant 3A of web/mockups/watch206-media.html): the poster is the main view
// (below: rt_poster_*); STATIONS opens this — the square's tuner by the watch's size: one row of band keys clear of the
// glass's round corners, the station and its song, the tuning window, volume − / previous / Stop / next / volume +, the
// volume's line. A station picked here (or a tap on the station's name) — back to the poster
#define RT_PILLS 1
#define RT_KEY_X0 14.0f
#define RT_KEY_P 74.0f
#define RT_KEY_W 68.0f
#define RT_KEY_H 46.0f
#define RT_KEY_Y 52.0f
#define RT_KEY_ROWS 1
#define RT_KEY_PY 0.0f
#define RT_KEY_COLS 1
#define RT_KEY_WY 2.0f
#define RT_KEYS_X1 396.0f
#define RT_KEYS_PAD 4.0f
#define RT_NAME_Y 132.0f
#define RT_NAME_W 330.0f
#define RT_SONG_Y 164.0f
#define RT_SONG_W 362.0f
#define RT_TIME_X 26.0f
#define RT_TIME_AL 0
#define RT_HEART_X 382.0f
#define RT_HEART_R 22.0f
#define RT_TOP_Y 98.0f
#define RT_MID_Y0 80.0f
#define RT_MID_Y1 180.0f
#define RT_WIN_X0 14.0f
#define RT_WIN_X1 396.0f
#define RT_WIN_Y0 186.0f
#define RT_WIN_Y1 344.0f
#define RT_LOGO_Y 236.0f
#define RT_STEP 130.0f
#define RT_TICK 13.0f
#define RT_TICK_Y 304.0f
#define RT_LAB_Y 328.0f
#define RT_FADE 54.0f
#define RT_DANCE 16.0f
#define RT_NEEDLE_Y0 270.0f
#define RT_NEEDLE_Y1 318.0f
#define RT_BTN_Y 402.0f
#define RT_PREV_X 132.0f
#define RT_PLAY_X 205.0f
#define RT_NEXT_X 278.0f
#define RT_SIDE_R 28.0f
#define RT_PLAY_R 38.0f
#define RT_VOLM_X 52.0f
#define RT_VOLP_X 358.0f
#define RT_VBTN_W 72.0f
#define RT_VBTN_H 58.0f
#define RT_VLINE_Y 458.0f
#define RT_KEY_IN 3.0f
#define RT_LOGO_R0 17.0f
#define RT_LOGO_R1 7.0f
#define RT_KNOB_Y RT_BTN_Y
#define RT_KNOB_R 0.0f
#define RT_VOLK_X 0.0f
#define RT_TUNK_X 0.0f
// The poster: the middles of its lines (the name: one line, or two)
#define PT_TIME_Y 30.0f
#define PT_NAME_Y 160.0f
#define PT_NAME_W 340.0f
#define PT_SONG_Y 248.0f
#define PT_SONG_W 320.0f
#define PT_IC_Y 298.0f
#define PT_WORD_Y 326.0f
#define PT_WORD_DX 64.0f
#define PT_BTN_Y 424.0f
#define PT_BTN_W 220.0f
#define PT_BTN_H 84.0f
#define PT_RAY_Y0 80.0f     // the rays: the top one's middle, the bottom one's (the low bands at the bottom)
#define PT_RAY_Y1 396.0f
#define PT_RAY_X 6.0f       // they start this far in from the side
#define PT_RAY_L0 10.0f     // their length: quiet, and how much more at the loudest
#define PT_RAY_L1 44.0f
extern const HaloFont gl_ra_n52, gl_ra_n32, gl_ra_t24, gl_ra_t20, gl_ra_t14, gl_ra_ic24;  // (studio.py: the round's radio fonts)
#elif defined(HALO_SQUARE)
#define RT_PILLS 1
#define RT_KEY_X0 12.0f
#define RT_KEY_P 76.0f
#define RT_KEY_W 70.0f
#define RT_KEY_H 48.0f
#define RT_KEY_Y 34.0f
#define RT_KEY_ROWS 1
#define RT_KEY_PY 0.0f
#define RT_KEY_COLS 1
#define RT_KEY_WY 2.0f
#define RT_KEYS_X1 468.0f
#define RT_KEYS_PAD 4.0f
#define RT_NAME_Y 122.0f
#define RT_NAME_W 448.0f
#define RT_SONG_Y 152.0f
#define RT_SONG_W 420.0f
#define RT_TIME_X 16.0f
#define RT_TIME_AL 0
#define RT_HEART_X 440.0f
#define RT_HEART_R 24.0f
#define RT_TOP_Y 88.0f
#define RT_MID_Y0 66.0f
#define RT_MID_Y1 166.0f
#define RT_WIN_X0 16.0f
#define RT_WIN_X1 464.0f
#define RT_WIN_Y0 174.0f
#define RT_WIN_Y1 340.0f
#define RT_LOGO_Y 229.0f
#define RT_STEP 150.0f
#define RT_TICK 15.0f
#define RT_TICK_Y 298.0f
#define RT_LAB_Y 322.0f
#define RT_FADE 60.0f
#define RT_DANCE 18.0f
#define RT_NEEDLE_Y0 264.0f
#define RT_NEEDLE_Y1 313.0f
#define RT_BTN_Y 400.0f    // the row: volume −, previous, Stop / Play, next, volume +
#define RT_PREV_X 150.0f
#define RT_PLAY_X 240.0f
#define RT_NEXT_X 330.0f
#define RT_SIDE_R 32.0f
#define RT_PLAY_R 42.0f
#define RT_VOLM_X 56.0f    // the volume's pills: their middles, their size
#define RT_VOLP_X 424.0f
#define RT_VBTN_W 80.0f
#define RT_VBTN_H 64.0f
#define RT_VLINE_Y 455.0f  // the volume's line
#define RT_KEY_IN 3.0f
#define RT_LOGO_R0 18.0f
#define RT_LOGO_R1 8.0f
#define RT_KNOB_Y RT_BTN_Y  // (no knobs here)
#define RT_KNOB_R 0.0f
#define RT_VOLK_X 0.0f
#define RT_TUNK_X 0.0f
#elif defined(HALO_P4_TALL)
#define RT_KEY_X0 12.0f    // the band keys: the first one's left edge, the pitch, the size
#define RT_KEY_P 116.0f
#define RT_KEY_W 108.0f
#define RT_KEY_H 66.0f
#define RT_KEY_Y 45.0f     // … the first row's middle
#define RT_KEY_ROWS 2      // … rows, their pitch
#define RT_KEY_PY 76.0f
#define RT_KEY_COLS 5      // … keys in a row at least
#define RT_KEY_WY 2.0f     // … the word under the key's middle
#define RT_KEYS_X1 588.0f  // … the rows' right end
#define RT_KEYS_PAD 2.0f   // … the keys take the finger this far under them (the heart is near)
#define RT_NAME_Y 250.0f
#define RT_NAME_W 536.0f
#define RT_SONG_Y 298.0f
#define RT_SONG_W 536.0f
#define RT_TIME_X 32.0f
#define RT_TIME_AL 0       // (from its left end)
#define RT_HEART_X 536.0f
#define RT_HEART_R 34.0f
#define RT_TOP_Y 190.0f    // the time and the heart
#define RT_MID_Y0 152.0f   // the band of the time, the heart, the station and the song
#define RT_MID_Y1 320.0f
#define RT_WIN_X0 16.0f    // the tuning window
#define RT_WIN_X1 584.0f
#define RT_WIN_Y0 340.0f
#define RT_WIN_Y1 516.0f
#define RT_LOGO_Y 396.0f   // … the logos' middles
#define RT_STEP 180.0f     // … between two stations
#define RT_TICK 18.0f      // … between two ticks
#define RT_TICK_Y 472.0f   // … the ticks stand on this line
#define RT_LAB_Y 496.0f    // … the names under them
#define RT_FADE 90.0f      // … everything fades out over this much at its ends
#define RT_DANCE 22.0f     // … how much a tick grows with the sound
#define RT_NEEDLE_Y0 442.0f  // … the needle: from under the logos to over the names
#define RT_NEEDLE_Y1 484.0f
#define RT_BTN_Y 610.0f    // previous, Stop / Play, next
#define RT_PREV_X 170.0f
#define RT_PLAY_X 300.0f
#define RT_NEXT_X 430.0f
#define RT_KNOB_Y 830.0f   // the knobs
#define RT_KNOB_R 84.0f
#define RT_VOLK_X 132.0f
#define RT_TUNK_X 468.0f
#else
#define RT_KEY_X0 12.0f    // the band keys: the first one's left edge, the pitch, the size
#define RT_KEY_P 100.0f
#define RT_KEY_W 92.0f
#define RT_KEY_H 62.0f
#define RT_KEY_Y 41.0f     // … their middle
#define RT_KEY_ROWS 1      // … one row: all of them
#define RT_KEY_PY 0.0f
#define RT_KEY_COLS 1
#define RT_KEY_WY 5.0f
#define RT_KEYS_X1 1012.0f // … the row's right end
#define RT_KEYS_PAD 8.0f
#define RT_NAME_Y 116.0f
#define RT_NAME_W 760.0f
#define RT_SONG_Y 160.0f
#define RT_SONG_W 760.0f
#define RT_TIME_X 70.0f
#define RT_TIME_AL 1
#define RT_HEART_X 930.0f
#define RT_HEART_R 32.0f
#define RT_TOP_Y 122.0f    // the time and the heart
#define RT_MID_Y0 80.0f
#define RT_MID_Y1 190.0f
#define RT_WIN_X0 36.0f    // the tuning window
#define RT_WIN_X1 988.0f
#define RT_WIN_Y0 192.0f
#define RT_WIN_Y1 360.0f
#define RT_LOGO_Y 240.0f   // … the logos' middles
#define RT_STEP 180.0f     // … between two stations
#define RT_TICK 18.0f      // … between two ticks
#define RT_TICK_Y 318.0f   // … the ticks stand on this line (the bottom of a 16-row band: the dance touches 3 of them)
#define RT_LAB_Y 340.0f    // … the names under them
#define RT_FADE 120.0f     // … everything fades out over this much at its ends
#define RT_DANCE 22.0f     // … how much a tick grows with the sound
#define RT_NEEDLE_Y0 286.0f
#define RT_NEEDLE_Y1 354.0f
#define RT_BTN_Y RT_KNOB_Y // previous, Stop / Play, next: between the knobs
#define RT_PREV_X 378.0f
#define RT_PLAY_X 512.0f
#define RT_NEXT_X 646.0f
#define RT_KNOB_Y 470.0f   // the knobs
#define RT_KNOB_R 84.0f
#define RT_VOLK_X 150.0f
#define RT_TUNK_X 874.0f
#endif
#define RT_KEYS_Y0 (RT_KEY_Y - RT_KEY_H / 2)  // the keys' band: its top, its bottom
#define RT_KEYS_Y1 (RT_KEY_Y + (RT_KEY_ROWS - 1) * RT_KEY_PY + RT_KEY_H / 2)
#define RT_DEG 36.0f       // the tuning knob: degrees a station
#ifndef RT_KEY_IN
#define RT_KEY_IN 10.0f    // a key's word: this much narrower than the key, at most
#endif
#ifndef RT_LOGO_R0
#define RT_LOGO_R0 24.0f   // a logo away from the needle, and how much bigger it is under it
#define RT_LOGO_R1 6.0f
#endif
#ifndef RT_SIDE_R
#define RT_SIDE_R 40.0f
#define RT_PLAY_R 58.0f
#endif
#define RT_TICKS 64        // the ticks laid out at most (~55 are seen)
#define RT_LOGOS 8         // the logos seen at most
#define RT_DOTS 16         // a knob's grip dots
#define RT_KEY_ON 0x3A3D42 // the key looked at
#define RT_LED_OFF 0x1F2023
#define RT_WIN_EDGE 0x26272B
#define RT_KNOB_IN 0x26282C
#define RT_DOT 0x4A4C52
#define RT_TICK_MAJ 0x5A5B61
#define RT_TICK_MIN 0x303136
#define RT_HEART_OFF 0x55555B
static const char RT_PREV[] = "\xEF\x81\x88", RT_NEXT[] = "\xEF\x81\x91";  // Font Awesome: backward-step, forward-step

enum : int8_t { RT_NONE = -1, RT_KEYS, RT_WINDOW, RT_HEART, RT_PREV_B, RT_PLAY_B, RT_NEXT_B, RT_VOLK, RT_TUNK, RT_VOLM, RT_VOLP,
                RT_BACK, RT_STATIONS, RT_VOLUME };  // (the last three: the watch's poster and its way back)
struct RtDmg {  // what this file draws itself (the song, the needle's mark, the notches): where it was damaged last
  float r[4];
  uint32_t h;
};
struct RtUi {
  // The folder looked at: its stations (the favourites — a copy of them, kept up to date)
  RdSt rows[RD_RES];
  int n, view, state;           // state: 0 — loading, 1 — rows, 2 — failed
  uint32_t gen, fav_seen, rows_gen;
  int park;                     // where the window stands when the station that plays is not in this folder
  char path[220];
  // The band keys: their words; the row's scroll (more than ten folders)
  char key_s[RD_FOLDER_MAX][24];
  float key_off, key_off0, key_most;
  bool key_drag;
  // The window: its place (stations, 0 — the first under the needle), dragged / turned by the finger, or by a spring
  HaloSpring sc;
  float sc0;
  bool sc_drag, win_drag;       // (the window moved: by a drag of it, or by the tuning knob)
  int lv0, lv1;                 // the logos' items in use
  // A knob turned: the angle the finger went round it (degrees, clockwise), its last angle; the volume's place, 0..1
  float turn, turn_last, vol0, vol_f;
  bool turn_on;
  // The name: cut to fit, its scale; the song: its width
  char name_s[RD_NAME + 8];
  float name_k;
  uint32_t name_gen, song_gen;
  float song_w;
  RtDmg song_dmg, mark_dmg, notch_dmg[2], key_dmg, tick_dmg;
  HaloItem key[RD_FOLDER_MAX], led[RD_FOLDER_MAX], kword[RD_FOLDER_MAX];
  HaloItem time, name, heart_bg, heart, msg, win_side[4], win_corner[4], needle;
  HaloItem logo[RT_LOGOS], mono[RT_LOGOS], lab[RT_LOGOS], ring, eq[3];
  HaloItem arc_track, arc_lvl, body[2], inner[2], dot[2][RT_DOTS], klab[2];
  HaloItem prev_bg, prev, play, play_ic, next_bg, next;
  Press p_heart, p_prev, p_play, p_next;
#ifdef RT_PILLS
  HaloItem vbtn[2], vsign[3], vline[2];  // the volume's pills (−, +), their signs (−, + across, + up), its line (track, level)
  Press p_vol[2];
#endif
#ifdef HALO_WATCH  // the poster
  bool poster;
  uint32_t back_at;             // a station picked in the tuner: back to the poster then
  HaloItem pt_time, pt_name[2], pt_ic[2], pt_word[2], pt_div, pt_btn, pt_btn_ic, pt_btn_t;
  Press p_st, p_vo, p_pb;
  char pt_line[2][RD_NAME + 8];
  const HaloFont *pt_f;
  int pt_lines;
  uint32_t pt_gen, pt_song_gen;
  float pt_song_w;
  RtDmg pt_song_dmg, pt_ray_dmg[2];
#endif
};
static RtUi *rt_u;  // (PSRAM: rt_build)
static HaloView rt_v;  // the view laid out with (what this file draws itself uses it)
// The ticks: drawn in one pass (rt_ticks_draw), not items — ~55 items in PSRAM went through for every job of a frame
struct RtTick {
  float x, w, top, a;  // the middle, the width, the top (the bottom — RT_TICK_Y), how seen
  uint32_t c;
};
static RtTick rt_tk[RT_TICKS];
static int rt_tk_n;
static HaloSpringK K_RT_SCALE;
static float rt_mid_n48, rt_mid_t24, rt_mid_t18, rt_mid_t15, rt_mid_t12, rt_mid_n24, rt_mid_ic;
static float rt_notch[2];  // the knobs' angles laid out (degrees from 12 o'clock): their notches
static int8_t rt_armed = RT_NONE;
static int rt_owner_keys, rt_owner_win, rt_owner_knob;  // (their addresses: who took the finger)

// ---------- The folder looked at, its stations ----------
static uint32_t rt_fav_sig() {
  return rd_fav_changed * 31u + rd_fav_n;
}
static int rt_ci = -1;  // the station that plays in the rows (−1 — not there), for rp_name_gen and the rows' generation
static uint32_t rt_ci_key = 1;
static int rt_cur_idx() {
  RtUi &u = *rt_u;
  const uint32_t key = rp_name_gen * 2654435761u ^ u.rows_gen ^ (rd_has_cur ? 0x80000000u : 0);
  if (key == rt_ci_key) return rt_ci;
  rt_ci_key = key, rt_ci = -1;
  for (int i = 0; rd_has_cur && i < u.n; i++)
    if (!strcmp(u.rows[i].url, rd_cur.url)) rt_ci = i;
  return rt_ci;
}
static int rt_target() {  // where the window settles: on the station that plays, else where it was left
  RtUi &u = *rt_u;
  const int c = rt_cur_idx();
  return c >= 0 ? c : min(u.park, max(0, u.n - 1));
}
// The band keys: how many in a row (a row — all of them; tall — rows of five, more than ten — half of them in each)
static int rt_key_cols() {
  const int nf = min(RD_FOLDER_N, RD_FOLDER_MAX);
  return max(RT_KEY_COLS, (nf + RT_KEY_ROWS - 1) / RT_KEY_ROWS);
}
static void rt_key_show(int i) {  // (more than ten folders: the one opened in sight)
  RtUi &u = *rt_u;
  const float l = (RT_KEY_ROWS > 1 ? i % rt_key_cols() : i) * RT_KEY_P, r = l + RT_KEY_W, room = RT_KEYS_X1 - RT_KEY_X0;
  if (r - u.key_off > room) u.key_off = r - room;
  if (l < u.key_off) u.key_off = l;
}
static void rt_rows_changed() {  // new rows: the window stands on its station at once (no slide from another folder's)
  RtUi &u = *rt_u;
  u.rows_gen++, u.sc_drag = u.win_drag = false;
  const float t = rt_target();
  u.sc = {t, 0, t};
}
static void rt_rows_fav() {
  RtUi &u = *rt_u;
  u.n = min(rd_fav_n, RD_RES), u.state = 1, u.fav_seen = rt_fav_sig();
  memcpy(u.rows, rd_fav, u.n * sizeof(RdSt));
}
static void rt_open(int folder) {
  RtUi &u = *rt_u;
  u.view = folder, u.park = 0;
  const RdFolder &f = RD_FOLDER[folder];
  if (f.kind == RF_FAV) {
    rt_rows_fav();
  } else {
    u.n = 0, u.state = 0;
    rd_folder_path(f, u.path, sizeof(u.path));
    u.gen = rn_fetch_gen;
    rn_fetch(u.path, folder);
  }
  rt_rows_changed();
  rt_key_show(folder);
}
static void rt_take_fetch() {  // the catalog answered: its stations into the rows
  RtUi &u = *rt_u;
  if (u.state != 0 || rn_fetch_gen == u.gen || rn_fetch_st == 1) return;
  u.gen = rn_fetch_gen;
  if (rn_fetch_st == 2) {
    u.n = min((int)rn_res_n, RD_RES);
    memcpy(u.rows, rn_res, u.n * sizeof(RdSt));
    u.state = 1;
  } else {
    u.state = 2;
  }
  rt_rows_changed();
}
// Station i of the rows plays: the folder onto the dial (prev / next go through it), as a station picked in the round
// board's folder; the one already playing — nothing
static void rt_pick(int i, int dir) {
  RtUi &u = *rt_u;
  if (u.state != 1 || i < 0 || i >= u.n) return;
#ifdef HALO_WATCH
  if (!u.poster) u.back_at = halo_ms() + 700;  // (the watch: picked in the tuner — the poster shows it in a moment)
#endif
  if (rd_has_cur && rd_on && !strcmp(rd_cur.url, u.rows[i].url)) return;
  rd_folder = u.view;
  if (RD_FOLDER[rd_folder].kind == RF_FAV) {
    rd_list_favs();
    for (int k = 0; k < rd_list_n; k++)
      if (!strcmp(rd_list[k].url, u.rows[i].url)) return rd_tune(k, dir);
  } else {
    rd_list_n = u.n;
    memcpy(rd_list, u.rows, u.n * sizeof(RdSt));
    rd_tune(i, dir);
  }
}
static void rt_settle() {  // the window let go: the station nearest the needle plays
  RtUi &u = *rt_u;
  u.sc_drag = false;
  if (u.state != 1 || !u.n) return;
  const int i = (int)lroundf(fminf(u.n - 1.0f, fmaxf(0, u.sc.x))), ci = rt_cur_idx();
  u.park = i;
  rt_pick(i, ci < 0 || i > ci ? 1 : -1);
}
static const RdSt *rt_heart_st() {  // what the heart is for: the station that plays, else the one under the needle
  RtUi &u = *rt_u;
  if (rd_has_cur) return &rd_cur;
  const int i = lroundf(u.sc.x);
  return u.state == 1 && i >= 0 && i < u.n ? &u.rows[i] : nullptr;
}

// From radio_build: the folder on the dial looked at, the keys' words, the fonts' middles
static void rt_build() {
  void *m = heap_caps_calloc(1, sizeof(RtUi), MALLOC_CAP_SPIRAM);
  if (!m) return;
  rt_u = new (m) RtUi();
  RtUi &u = *rt_u;
  K_RT_SCALE = halo_spring(0.32f, 0.86f);
  rt_mid_n48 = halo_text_mid(gl_rr_n48, "Radio"), rt_mid_t24 = halo_text_mid(gl_rr_t24, "RP");
  rt_mid_t18 = halo_text_mid(gl_rr_t18, "Radio"), rt_mid_t15 = halo_text_mid(gl_rr_t15, "VOLUME");
  rt_mid_t12 = halo_text_mid(gl_rr_t12, "JAZZ"), rt_mid_n24 = halo_text_mid(gl_r_n24, "0");
  rt_mid_ic = halo_text_mid(gl_rr_ic48, RD_PLAY);
#ifdef HALO_WATCH
  rd_mid_n52 = halo_text_mid(gl_ra_n52, "Radio"), rd_mid_n32 = halo_text_mid(gl_ra_n32, "Radio");
  rd_mid_t24 = halo_text_mid(gl_ra_t24, "Radio"), rd_mid_t20 = halo_text_mid(gl_ra_t20, "Radio");
  rd_mid_t14 = halo_text_mid(gl_ra_t14, "STATIONS"), rd_mid_ic24 = halo_text_mid(gl_ra_ic24, RD_BARS);
  u.poster = true;
#endif
  const int nf = min(RD_FOLDER_N, RD_FOLDER_MAX);
  for (int i = 0; i < nf; i++) {  // a key's word: the folder's first word in capitals, cut to the key
    char w[RD_NAME];
    rd_copy(w, RD_FOLDER[i].name, sizeof(w));
    w[strcspn(w, " ")] = 0;
    for (char *p = w; *p; p++)
      if (*p >= 'a' && *p <= 'z') *p -= 32;
    rd_copy(u.key_s[i], w, sizeof(u.key_s[i]));
    while (halo_text_width(gl_rr_t12, u.key_s[i], 1) > RT_KEY_W - RT_KEY_IN && strlen(u.key_s[i]) > 1) {
      size_t n = strlen(u.key_s[i]);
      do n--; while (n && ((uint8_t)u.key_s[i][n] >> 6) == 2);
      u.key_s[i][n] = 0;
    }
  }
  u.key_most = fmaxf(0, rt_key_cols() * RT_KEY_P - (RT_KEY_P - RT_KEY_W) - (RT_KEYS_X1 - RT_KEY_X0));
  u.vol_f = -1;
  rt_open(rd_folder);
}

// ---------- The finger (on top and still) ----------
#ifdef HALO_WATCH
static void rt_poster_update(float dt);
static void rt_mode(bool poster) {  // the poster or the tuner: everything redrawn (the other's items are not laid out)
  if (rt_u->poster != poster) rt_u->poster = poster, rt_u->back_at = 0, halo_damage_all();
}
#endif
static void radio_enter() {
  if (!rt_u) return;
  RtUi &u = *rt_u;
  rt_armed = RT_NONE, u.sc_drag = u.win_drag = u.key_drag = u.turn_on = false, u.vol_f = -1;
#ifdef HALO_WATCH
  rt_mode(true), vol_enter();
#endif
  if (RD_FOLDER[u.view].kind == RF_FAV && u.fav_seen != rt_fav_sig()) rt_rows_fav(), rt_rows_changed();
}
static bool rt_in(float x, float y, float cx, float cy, float r) {
  return (x - cx) * (x - cx) + (y - cy) * (y - cy) < r * r;
}
static float rt_ang(float x, float y, float cx, float cy) {  // degrees from 12 o'clock, clockwise
  return atan2f(x - cx, cy - y) * (180 / HALO_PI);
}
static void rt_turn(const HaloFinger &f, float cx) {  // how far round the knob the finger went (near its middle — not counted)
  RtUi &u = *rt_u;
  if (rt_in(f.x, f.y, cx, RT_KNOB_Y, 22)) return;
  const float a = rt_ang(f.x, f.y, cx, RT_KNOB_Y);
  if (!u.turn_on) u.turn_on = true, u.turn_last = a;
  float d = a - u.turn_last;
  d -= 360 * lroundf(d / 360);
  u.turn += d, u.turn_last = a;
}
static void radio_update(const FaceScreen &, float dt) {
  if (!ru || !rt_u) return;
  RtUi &u = *rt_u;
  HaloFinger &f = halo_finger;
  rt_take_fetch();
#ifdef HALO_WATCH
  if (u.back_at && (int32_t)(halo_ms() - u.back_at) >= 0) rt_mode(true);
  if (u.poster) return rt_poster_update(dt);
#endif
  if (RD_FOLDER[u.view].kind == RF_FAV && u.fav_seen != rt_fav_sig()) {  // (a heart changed: the favourites' rows)
    const float at = u.sc.x;
    rt_rows_fav();
    u.rows_gen++;
    u.sc.x = fminf(at, fmaxf(0, u.n - 1.0f));
  }
  if (f.pressed && !f.owner) {
    rt_armed = RT_NONE;
    const float x = f.x0, y = f.y0;
    if (y < RT_KEYS_Y1 + RT_KEYS_PAD) rt_armed = RT_KEYS;
    else if (rt_in(x, y, RT_HEART_X, RT_TOP_Y, 46)) rt_armed = RT_HEART;
#ifdef HALO_WATCH
    else if (y >= RT_MID_Y0 && y <= RT_MID_Y1) rt_armed = RT_BACK;  // (the time, the station, its song: back to the poster)
#endif
    else if (x >= RT_WIN_X0 && x <= RT_WIN_X1 && y >= RT_WIN_Y0 - 6 && y <= RT_WIN_Y1 + 6) rt_armed = RT_WINDOW;
    else if (rt_in(x, y, RT_PLAY_X, RT_BTN_Y, RT_PLAY_R + 8)) rt_armed = RT_PLAY_B;
    else if (rt_in(x, y, RT_PREV_X, RT_BTN_Y, RT_SIDE_R + 8)) rt_armed = RT_PREV_B;
    else if (rt_in(x, y, RT_NEXT_X, RT_BTN_Y, RT_SIDE_R + 8)) rt_armed = RT_NEXT_B;
#ifdef RT_PILLS
    else if (fabsf(y - RT_BTN_Y) < RT_VBTN_H / 2 + 8 && fabsf(x - RT_VOLM_X) < RT_VBTN_W / 2 + 6) rt_armed = RT_VOLM;
    else if (fabsf(y - RT_BTN_Y) < RT_VBTN_H / 2 + 8 && fabsf(x - RT_VOLP_X) < RT_VBTN_W / 2 + 6) rt_armed = RT_VOLP;
#else
    else if (rt_in(x, y, RT_VOLK_X, RT_KNOB_Y, RT_KNOB_R + 14) || rt_in(x, y, RT_TUNK_X, RT_KNOB_Y, RT_KNOB_R + 14)) {
      if (halo_finger_claim(&rt_owner_knob)) {  // (taken at once: a knob turns from the first move, the back swipe stays off)
        rt_armed = x < CX ? RT_VOLK : RT_TUNK;
        u.turn = 0, u.turn_on = false, u.vol0 = vol_lvl, u.sc0 = u.sc.x;
      }
    }
#endif
  }
  // The keys' row scrolls (more than ten folders); the window moves under the finger
  if (f.drag_start && f.horiz) {
    if (rt_armed == RT_KEYS && u.key_most > 0 && halo_finger_claim(&rt_owner_keys)) u.key_drag = true, u.key_off0 = u.key_off;
    else if (rt_armed == RT_WINDOW && u.state == 1 && u.n > 0 && halo_finger_claim(&rt_owner_win)) u.sc_drag = u.win_drag = true, u.sc0 = u.sc.x;
  }
  if (u.key_drag) {
    u.key_off = fminf(u.key_most, fmaxf(0, u.key_off0 - (f.x - f.x0)));
    if (!(f.down && f.owner == &rt_owner_keys)) u.key_drag = false;
  }
  const float hi = u.n - 1.0f;
  auto band = [hi](float x) { return x < 0 ? x / 3 : x > hi ? hi + (x - hi) / 3 : x; };  // (past the ends — a rubber band)
  if (u.win_drag) {
    if (f.down && f.owner == &rt_owner_win) u.sc.x = u.sc.target = band(u.sc0 - (f.x - f.x0) / RT_STEP), u.sc.v = 0;
    else u.win_drag = false, u.sc.v = -f.vx / RT_STEP, rt_settle();
  }
  // The knobs: the volume follows the turn (270 degrees for 0..10); the tuning knob moves the window, let go — it settles
  if (rt_armed == RT_VOLK || rt_armed == RT_TUNK) {
    if (f.down && f.owner == &rt_owner_knob) {
      rt_turn(f, rt_armed == RT_VOLK ? RT_VOLK_X : RT_TUNK_X);
      if (rt_armed == RT_VOLK) {
        u.vol_f = halo_clamp01(u.vol0 + u.turn / 270);
        const int level = (int)lroundf(u.vol_f * 10);
        if (level != vol_level) vol_set(level);
      } else if (u.state == 1 && u.n > 0 && fabsf(u.turn) > 4) {
        u.sc_drag = true, u.sc.x = u.sc.target = band(u.sc0 + u.turn / RT_DEG), u.sc.v = 0;
      }
    } else {
      if (rt_armed == RT_TUNK && u.sc_drag) rt_settle();
      u.vol_f = -1, u.turn_on = false, rt_armed = RT_NONE;
    }
  }
  if (f.dragging && rt_armed != RT_VOLK && rt_armed != RT_TUNK) rt_armed = RT_NONE;
  if (f.tap) {
    switch (rt_armed) {
      case RT_KEYS: {
        const float x = f.x0 - RT_KEY_X0 + u.key_off;
#if RT_KEY_ROWS > 1  // (the row nearest the finger)
        const int row = max(0, min(RT_KEY_ROWS - 1, (int)((f.y0 - RT_KEY_Y + RT_KEY_PY / 2) / RT_KEY_PY))), cols = rt_key_cols();
        const int col = (int)(x / RT_KEY_P), i = row * cols + col;
        if (x >= 0 && col < cols && i < min(RD_FOLDER_N, RD_FOLDER_MAX) && fmodf(x, RT_KEY_P) <= RT_KEY_W + 4 && (i != u.view || u.state == 2)) rt_open(i);
#else
        const int i = (int)(x / RT_KEY_P);
        if (x >= 0 && i < min(RD_FOLDER_N, RD_FOLDER_MAX) && fmodf(x, RT_KEY_P) <= RT_KEY_W + 4 && (i != u.view || u.state == 2)) rt_open(i);
#endif
        break;
      }
      case RT_WINDOW: {
        if (u.state == 2) { rt_open(u.view); break; }  // (failed — a tap asks once more)
        const int i = (int)lroundf(u.sc.x + (f.x0 - CX) / RT_STEP);
        if (u.state == 1 && i >= 0 && i < u.n && fabsf(CX + (i - u.sc.x) * RT_STEP - f.x0) < 76) {
          const int ci = rt_cur_idx();
          u.park = i;
          if (i == ci && !rd_on) rd_toggle();  // (stopped: its logo plays it again)
          else rt_pick(i, ci < 0 || i > ci ? 1 : -1);
        }
        break;
      }
      case RT_PLAY_B:  // nothing played yet: the station under the needle
        if (!rd_has_cur && u.state == 1 && u.n) rt_pick((int)lroundf(u.sc.x), 0);
        else rd_toggle();
        break;
      case RT_PREV_B:
      case RT_NEXT_B:
        if (rd_list_n) {
          const int dir = rt_armed == RT_NEXT_B ? 1 : -1;
          rd_tune(rd_mod((rd_at >= 0 ? rd_at : 0) + (rd_has_cur ? dir : 0), rd_list_n), dir);
        }
        break;
      case RT_HEART:
        if (const RdSt *s = rt_heart_st()) rd_fav_toggle(*s);
        break;
      case RT_VOLM:
      case RT_VOLP:
        vol_set(vol_level + (rt_armed == RT_VOLP ? 1 : -1));
        break;
#ifdef HALO_WATCH
      case RT_BACK:
        rt_mode(true);
        break;
#endif
      default:
        break;
    }
  }
  press_update(u.p_play, rt_armed == RT_PLAY_B && f.down, dt);
  press_update(u.p_prev, rt_armed == RT_PREV_B && f.down, dt);
  press_update(u.p_next, rt_armed == RT_NEXT_B && f.down, dt);
  press_update(u.p_heart, rt_armed == RT_HEART && f.down, dt);
#ifdef RT_PILLS
  press_update(u.p_vol[0], rt_armed == RT_VOLM && f.down, dt);
  press_update(u.p_vol[1], rt_armed == RT_VOLP && f.down, dt);
#endif
  if (!u.sc_drag) {
    u.sc.target = rt_target();
    halo_spring_run(u.sc, K_RT_SCALE, dt);
  }
  if (!f.down && rt_armed != RT_VOLK && rt_armed != RT_TUNK) rt_armed = RT_NONE;
}

// ---------- Layout ----------
// What this file draws itself changed: its old place and the new one redraw
static void rt_damage(RtDmg &d, uint32_t h, float x0, float y0, float x1, float y1) {
  const HaloView &v = rt_v;
  const float r[4] = {halo_view_x(v, x0), halo_view_y(v, y0), halo_view_x(v, x1), halo_view_y(v, y1)};
  for (float q : {v.x, v.y, v.s, v.a}) h = (h ^ (uint32_t)(int32_t)(q * 512)) * 16777619u;
  h |= 1;  // (0 — never drawn; compared as kept: an even hash stored as h | 1 redrew its place every frame — 15 Nov 2026)
  if (h == d.h && !memcmp(r, d.r, sizeof(r))) return;
  if (d.h) halo_damage_f(d.r[0], d.r[1], d.r[2], d.r[3]);
  halo_damage_f(r[0], r[1], r[2], r[3]);
  d.h = h, memcpy(d.r, r, sizeof(r));
}
static float rt_edge(float x) {  // the window fades out at its ends
  return halo_clamp01(fminf(x - RT_WIN_X0, RT_WIN_X1 - x) / RT_FADE);
}
static uint32_t rt_mix(uint32_t a, uint32_t b, float t) {
  const int k = (int)(halo_clamp01(t) * 256);
  uint32_t o = 0;
  for (int sh = 0; sh <= 16; sh += 8) {
    const int ca = a >> sh & 255, cb = b >> sh & 255;
    o |= (uint32_t)(ca + (((cb - ca) * k) >> 8)) << sh;
  }
  return o;
}
// A dancing tick's colour: the radio's orange, lighter when loud, nearly white at the loudest (the rays' colours)
static uint32_t rt_ray_col(float l) {
  return l < 0.8f ? rt_mix(RD_ORANGE, RD_RAY, (l - 0.05f) / 0.75f) : rt_mix(RD_RAY, RD_RAY_TOP, (l - 0.8f) / 0.2f);
}
static void rt_rrect(HaloItem &it, float x, float y, float w, float h, float r, uint32_t c, float a, const HaloView &v, bool on_black = true) {
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = 1;
  it.color = c, it.alpha = a, it.on_black = on_black, it.on = nullptr;
  halo_view_commit(it, v);
}
static void rt_circle(HaloItem &it, float x, float y, float r, uint32_t c, float a, const HaloView &v, bool on_black = true) {
  it.kind = HI_CIRCLE, it.x = x, it.y = y, it.r = r, it.scale = 1, it.color = c, it.alpha = a, it.on_black = on_black, it.on = nullptr;
  halo_view_commit(it, v);
}
static void rt_icon(HaloItem &it, const char *ic, float x, float y, float sc, uint32_t c, float a, const HaloItem *on, const HaloView &v) {
  gl_text_mid(it, gl_rr_ic48, ic, rt_mid_ic, x, y, sc, c, a, on);
  halo_view_commit(it, v);
}
static float rt_marquee(float tw, float w) {  // still for 4 s, then 80 px/s until the copy 120 px after it comes round
  if (tw <= w) return 0;
  const float t = fmodf(rp_song_t, 4 + (tw + 120) / 80);
  return t < 4 ? 0 : (t - 4) * 80;
}
static bool rt_song_err() {
  return rd_on && (rn_net == RN_NOWIFI || (rn_net == RN_FAILED && !rn_playing));
}
// A knob: the body, its inner disc, the grip dots turning with it (the notch is drawn by radio_draw: rt_notch)
static void rt_knob(int k, float cx, float ang, const char *word, const HaloView &v) {
  RtUi &u = *rt_u;
  rt_circle(u.body[k], cx, RT_KNOB_Y, RT_KNOB_R, RD_CARD, 1, v);  // (a disc under the inner one: cheaper than a ring round it)
  rt_circle(u.inner[k], cx, RT_KNOB_Y, RT_KNOB_R - 12, RT_KNOB_IN, 1, v);
  for (int i = 0; i < RT_DOTS; i++) {
    const float a = (ang + i * (360.0f / RT_DOTS)) * (HALO_PI / 180);
    rt_circle(u.dot[k][i], cx + (RT_KNOB_R - 6) * sinf(a), RT_KNOB_Y - (RT_KNOB_R - 6) * cosf(a), 2, RT_DOT, 1, v, false);
  }
  gl_text_mid(u.klab[k], gl_rr_t15, word, rt_mid_t15, cx, RT_KNOB_Y + RT_KNOB_R + (k ? 22 : 30), 1, RD_DIM, 1, &halo_black);
  u.klab[k].spacing = 2;
  halo_view_commit(u.klab[k], v);
  rt_notch[k] = ang;
}


#ifdef HALO_WATCH
static void rt_poster_layout(const HaloView &v);
#endif
static void radio_layout(const HaloView &v0) {
  if (!ru || !rt_u) return;
  RtUi &u = *rt_u;
#ifdef HALO_WATCH
  HaloView v = v0;
  v.a *= vol_layout(v0);  // (the volume screen over the radio: it fades through)
  rt_v = v;
  if (u.poster) return rt_poster_layout(v);
#else
  const HaloView &v = v0;
  rt_v = v;
#endif
  // The band keys: a lamp lit on the one looked at
  const int nf = min(RD_FOLDER_N, RD_FOLDER_MAX), cols = rt_key_cols();
  for (int i = 0; i < nf; i++) {
    const float x = RT_KEY_X0 + i % cols * RT_KEY_P - u.key_off, cx = x + RT_KEY_W / 2, y = RT_KEY_Y + i / cols * RT_KEY_PY;
    const bool sel = i == u.view, seen = x < RT_KEYS_X1 && x + RT_KEY_W > RT_KEY_X0;
    rt_rrect(u.key[i], cx, y, RT_KEY_W, RT_KEY_H, 14, sel ? RT_KEY_ON : RD_CARD, seen ? 1 : 0, v);
    rt_circle(u.led[i], x + (RT_KEY_W - 14), y - (RT_KEY_H / 2 - 12), 4, sel ? RD_ORANGE : RT_LED_OFF, seen ? 1 : 0, v, false);
    gl_text_mid(u.kword[i], gl_rr_t12, u.key_s[i], rt_mid_t12, cx, y + RT_KEY_WY, 1, sel ? RD_TEXT : RD_DIM, seen ? 1 : 0, &u.key[i]);
    u.kword[i].spacing = 1;
    halo_view_commit(u.kword[i], v);
  }
  rt_damage(u.key_dmg, (u.key_off > 0.5f ? 1 : 0) | (u.key_off < u.key_most - 0.5f ? 2 : 0), RT_KEY_X0 - 2, RT_KEYS_Y0 - 3, RT_KEYS_X1 + 2, RT_KEYS_Y1 + 3);
  // The station, big (a long one smaller, longer still — cut); it slides in from a skip's side (rd_tune: rp_slide)
  if (u.name_gen != rp_name_gen) {
    u.name_gen = rp_name_gen;
    const float w = halo_text_width(gl_rr_n48, rp_name_s);
    u.name_k = w > RT_NAME_W ? fmaxf(0.72f, RT_NAME_W / w) : 1;
    rd_fit(gl_rr_n48, rp_name_s, RT_NAME_W / u.name_k, u.name_s, sizeof(u.name_s));
  }
  gl_text_mid(u.name, gl_rr_n48, u.name_s, rt_mid_n48, CX + rp_slide * 60, RT_NAME_Y, u.name_k, RD_TEXT,
              1 - fminf(1, fabsf(rp_slide) * 0.9f), &halo_black);
  halo_view_commit(u.name, v);
  // The song: drawn by radio_draw (a marquee, cut to its width) — its place redraws when it changes
  if (u.song_gen != rp_song_gen) u.song_gen = rp_song_gen, u.song_w = halo_text_width(gl_rr_t24, rp_song);
  const float mq = v.a > 0.004f ? rt_marquee(u.song_w, RT_SONG_W) : 0, hh = gl_rr_t24.line / 2.0f;
  rt_damage(u.song_dmg, rd_hash(rp_song) ^ (uint32_t)(int32_t)(mq * 16) ^ (rt_song_err() ? 0x5bd1e995u : 0),
            CX - RT_SONG_W / 2, RT_SONG_Y - hh, CX + RT_SONG_W / 2, RT_SONG_Y + hh);
  // The time; the heart (the station that plays, else the one under the needle — into the favourites)
  rg_text(u.time, gl_r_n24, rp_time, rt_mid_n24, RT_TIME_AL, RT_TIME_X, RT_TOP_Y, RD_DIM, 1, &halo_black);
  halo_view_commit(u.time, v);
  {
    const RdSt *hs = rt_heart_st();
    const bool fav = hs && rd_fav_find(hs->url) >= 0;
    const bool pop = fav && rd_pop_url == rd_hash(hs->url) && halo_ms() - rd_pop_at < 450;
    const float bounce = pop ? 1 + 0.4f * sinf((halo_ms() - rd_pop_at) / 450.0f * HALO_PI) : 1, p = halo_clamp01(u.p_heart.s.x);
    rt_circle(u.heart_bg, RT_HEART_X, RT_TOP_Y, RT_HEART_R * (1 - 0.06f * p), RD_CARD, 1, v);
    rt_icon(u.heart, RD_HEART, RT_HEART_X, RT_TOP_Y + 1, 0.67f * bounce, fav ? RD_PINK : RT_HEART_OFF, hs ? 1 : 0.4f, bounce == 1 ? &u.heart_bg : nullptr, v);
  }
  // The tuning window: its frame — a thin outline, black inside (a fill was ~2× the pixels of the window to redraw
  // every frame while the ticks dance: 2–5 frames missed a second at a station's change; 15 Nov 2026) …
  {
    const float R = 24, x0 = RT_WIN_X0 + 1, x1 = RT_WIN_X1 - 1, y0 = RT_WIN_Y0 + 1, y1 = RT_WIN_Y1 - 1;
    rt_rrect(u.win_side[0], (x0 + x1) / 2, y0, x1 - x0 - 2 * R, 2, 1, RT_WIN_EDGE, 1, v);
    rt_rrect(u.win_side[1], (x0 + x1) / 2, y1, x1 - x0 - 2 * R, 2, 1, RT_WIN_EDGE, 1, v);
    rt_rrect(u.win_side[2], x0, (y0 + y1) / 2, 2, y1 - y0 - 2 * R, 1, RT_WIN_EDGE, 1, v);
    rt_rrect(u.win_side[3], x1, (y0 + y1) / 2, 2, y1 - y0 - 2 * R, 1, RT_WIN_EDGE, 1, v);
    const float cxs[4] = {x0 + R, x1 - R, x1 - R, x0 + R}, cys[4] = {y0 + R, y0 + R, y1 - R, y1 - R};
    for (int c = 0; c < 4; c++) {  // (the corners: quarter arcs, from 9, 12, 3, 6 o'clock)
      HaloItem &a = u.win_corner[c];
      a.kind = HI_ARC, a.x = cxs[c], a.y = cys[c], a.r = R, a.w = 2, a.scale = 1;
      a.a0 = (c == 0 ? 270 : (c - 1) * 90) * (HALO_PI / 180), a.a1 = a.a0 + HALO_PI / 2;
      a.color = RT_WIN_EDGE, a.alpha = 1, a.on_black = true, a.on = nullptr;
      halo_view_commit(a, v);
    }
  }
  // … the ticks, dancing to the sound (the lows in the middle) …
  const int n = u.state == 1 ? u.n : 0;
  const float sc = u.sc.x, uq = sc * 10;
  const bool sound = rd_on && rn_playing;
  const int lo = -12, hi = (n > 0 ? (n - 1) * 10 : 0) + 12;
  const int qa = max(lo, (int)floorf(uq - (CX - RT_WIN_X0) / RT_TICK)), qb = min(hi, (int)ceilf(uq + (RT_WIN_X1 - CX) / RT_TICK));
  int k = 0;
  uint32_t th = 2166136261u;
  for (int q = qa; q <= qb && k < RT_TICKS; q++) {
    const float x = CX + (q - uq) * RT_TICK;
    if (x < RT_WIN_X0 + 4 || x > RT_WIN_X1 - 4) continue;
    const bool maj = n > 0 && q >= 0 && q <= (n - 1) * 10 && q % 10 == 0;  // (a station's)
    const int b = (int)lroundf(halo_clamp01(fabsf(x - CX) / ((RT_WIN_X1 - RT_WIN_X0) / 2)) * (RD_BANDS - 1));
    const float l = sound ? ru->lv[b] : 0, h = (maj ? 20 : 10) + l * RT_DANCE;
    RtTick &t = rt_tk[k++];
    t = {x, maj ? 3.0f : 2.0f, RT_TICK_Y - h, rt_edge(x), l > 0.05f ? rt_ray_col(l) : maj ? RT_TICK_MAJ : RT_TICK_MIN};
    for (float q2 : {x * 8, h * 8, t.a * 255}) th = (th ^ (uint32_t)(int32_t)q2) * 16777619u;
    th = (th ^ t.c) * 16777619u;
  }
  rt_tk_n = k;
  rt_damage(u.tick_dmg, th, RT_WIN_X0 + 2, RT_TICK_Y - 20 - RT_DANCE - 2, RT_WIN_X1 - 2, RT_TICK_Y + 1);
  // … the logos and the names under the ticks: away from the needle smaller and dimmer; the one that plays — a ring and
  // the bars in its circle (they fade in over its letters as it starts)
  const int ci = rt_cur_idx();
  bool ring = false, eq = false;
  int j = 0;
  for (int i = max(0, (int)floorf(sc - 3)); n && i <= min(n - 1, (int)ceilf(sc + 3)) && j < RT_LOGOS; i++) {
    const float x = CX + (i - sc) * RT_STEP, d = fabsf(i - sc);
    if (x < RT_WIN_X0 - 30 || x > RT_WIN_X1 + 30) continue;
    const RdSt &o = u.rows[i];
    const float r = RT_LOGO_R0 + RT_LOGO_R1 * halo_clamp01(1 - d), al = (1 - 0.5f * halo_clamp01(d)) * rt_edge(x);
    HaloItem &lg = u.logo[j];
    rt_circle(lg, x, RT_LOGO_Y, r, o.col, al, v);
    const bool cur = i == ci, eqc = cur && rd_on, near = d < 0.5f;
    const float ef = eqc ? rd_eq_fade() : 0;
    const int ml = strlen(o.mono);
    const bool big = ml <= 2 || (ml <= 4 && (uint8_t)o.mono[0] >= 0x80);  // (two letters, Latin or not)
    gl_text_mid(u.mono[j], big ? gl_rr_t24 : gl_rr_t18, o.mono, big ? rt_mid_t24 : rt_mid_t18, x, RT_LOGO_Y, r / (big ? 38 : 34), rd_ink(o.col),
                al * (1 - ef), &lg);
    halo_view_commit(u.mono[j], v);
    if (eqc) eq = true, rd_eq_circle(u.eq, x, RT_LOGO_Y, r / 32, rd_ink(o.col), al * ef, v, rd_eq_mode());
    if (cur && near) {
      HaloItem &rg = u.ring;
      rg.kind = HI_RING, rg.x = x, rg.y = RT_LOGO_Y, rg.r = r + 7, rg.w = 3, rg.v = 1, rg.scale = 1;
      rg.color = rg.color2 = RD_ORANGE, rg.alpha = al, rg.on_black = true, rg.on = nullptr;
      halo_view_commit(rg, v), ring = true;
    }
    gl_text_mid(u.lab[j], gl_rr_t18, o.lab, rt_mid_t18, x, RT_LAB_Y, 1, near ? RD_TEXT : RD_DIM, al, &halo_black);
    halo_view_commit(u.lab[j], v);
    j++;
  }
  u.lv0 = 0, u.lv1 = j;
  for (int q = j; q < RT_LOGOS; q++)
    for (HaloItem *it : {&u.logo[q], &u.mono[q], &u.lab[q]}) it->alpha = 0, halo_view_commit(*it, v);
  if (!ring) u.ring.alpha = 0, halo_view_commit(u.ring, v);
  if (!eq)
    for (auto &e : u.eq) e.alpha = 0, halo_view_commit(e, v);
  // … or a word: loading (breathing), failed (a tap — again), nothing there; the needle (its mark — radio_draw)
  const RdFolder &fd = RD_FOLDER[u.view];
  const char *msg = u.state == 0 ? "Loading\xE2\x80\xA6"
                  : u.state == 2 ? (rn_net == RN_NOWIFI ? "No Wi-Fi \xC2\xB7 tap to retry" : "No connection \xC2\xB7 tap to retry")
                  : !u.n ? (fd.kind == RF_FAV ? "No favorites yet" : "Nothing here") : "";
  const float br = u.state == 0 ? 0.55f + 0.45f * sinf(rd_clock * 5) : 1;
  gl_text_mid(u.msg, gl_rr_t24, msg, rt_mid_t24, CX, RT_LOGO_Y + 30, 1, RD_DIM, *msg ? br : 0, &halo_black);
  halo_view_commit(u.msg, v);
  rt_rrect(u.needle, CX, (RT_NEEDLE_Y0 + RT_NEEDLE_Y1) / 2.0f, 3, RT_NEEDLE_Y1 - RT_NEEDLE_Y0, 1.5f, RD_ORANGE, 1, v);
  rt_damage(u.mark_dmg, 7, CX - 14, RT_WIN_Y0 + 2, CX + 14, RT_WIN_Y0 + 20);
#ifdef RT_PILLS
  // The volume: two pills (−, + drawn as bars) and its line under the row
  for (int i = 0; i < 2; i++) {
    const float cx = i ? RT_VOLP_X : RT_VOLM_X, p = halo_clamp01(u.p_vol[i].s.x);
    rt_rrect(u.vbtn[i], cx, RT_BTN_Y, RT_VBTN_W * (1 - 0.05f * p), RT_VBTN_H * (1 - 0.05f * p), RT_VBTN_H / 2 * (1 - 0.05f * p), RD_CARD, 1, v);
    rt_rrect(u.vsign[i], cx, RT_BTN_Y, 22, 4, 2, RD_TEXT, (i ? vol_level < 10 : vol_level > 0) ? 1 : 0.35f, v, false);
  }
  rt_rrect(u.vsign[2], RT_VOLP_X, RT_BTN_Y, 4, 22, 2, RD_TEXT, vol_level < 10 ? 1 : 0.35f, v, false);
  const float lv = halo_clamp01(vol_lvl), lw = RT_WIN_X1 - RT_WIN_X0;
  rt_rrect(u.vline[0], (RT_WIN_X0 + RT_WIN_X1) / 2, RT_VLINE_Y, lw, 6, 3, RD_RAY_OFF, 1, v);
  rt_rrect(u.vline[1], RT_WIN_X0 + fmaxf(6, lw * lv) / 2, RT_VLINE_Y, fmaxf(6, lw * lv), 6, 3, RD_ORANGE, lv > 0.005f ? 1 : 0, v);
#else
  // The knobs: the volume (its range's arc round it, how far — orange) and the tuning (36 degrees a station)
  const float vf = u.vol_f >= 0 ? u.vol_f : halo_clamp01(vol_lvl);
  const float a0 = 225 * (HALO_PI / 180), A = 270 * (HALO_PI / 180);
  HaloItem &at = u.arc_track, &al = u.arc_lvl;
  at.kind = al.kind = HI_ARC, at.x = al.x = RT_VOLK_X, at.y = al.y = RT_KNOB_Y, at.r = al.r = RT_KNOB_R + 14, at.w = al.w = 6;
  // (not over each other: the track only where the level is not — two 270° arcs over each other were 1.4 ms of a full frame)
  at.scale = al.scale = 1, al.a0 = a0, al.a1 = at.a0 = a0 + A * vf, at.a1 = a0 + A;
  at.color = RD_RAY_OFF, al.color = RD_ORANGE, at.alpha = vf < 0.99f ? 1 : 0, al.alpha = vf > 0.01f ? 1 : 0, at.on_black = al.on_black = true;
  at.on = al.on = nullptr;
  halo_view_commit(at, v), halo_view_commit(al, v);
  rt_knob(0, RT_VOLK_X, -135 + 270 * vf, "VOLUME", v);
  rt_knob(1, RT_TUNK_X, fmodf(sc * RT_DEG, 360), "TUNING", v);
  rt_damage(u.notch_dmg[0], (uint32_t)(int32_t)(rt_notch[0] * 16) * 2654435761u, RT_VOLK_X - RT_KNOB_R, RT_KNOB_Y - RT_KNOB_R, RT_VOLK_X + RT_KNOB_R, RT_KNOB_Y + RT_KNOB_R);
  rt_damage(u.notch_dmg[1], (uint32_t)(int32_t)(rt_notch[1] * 16) * 2654435761u, RT_TUNK_X - RT_KNOB_R, RT_KNOB_Y - RT_KNOB_R, RT_TUNK_X + RT_KNOB_R, RT_KNOB_Y + RT_KNOB_R);
#endif
  // Previous, Stop / Play, next
  const bool playing = rd_on && rd_has_cur;
  const float pp = halo_clamp01(u.p_prev.s.x), pn = halo_clamp01(u.p_next.s.x), pl = halo_clamp01(u.p_play.s.x);
  const float skip_a = rd_list_n > 1 ? 1 : 0.35f;
  rt_circle(u.prev_bg, RT_PREV_X, RT_BTN_Y, RT_SIDE_R * (1 - 0.06f * pp), RD_CARD, 1, v);
  rt_icon(u.prev, RT_PREV, RT_PREV_X, RT_BTN_Y, 0.83f, RD_TEXT, skip_a, &u.prev_bg, v);
  rt_circle(u.next_bg, RT_NEXT_X, RT_BTN_Y, RT_SIDE_R * (1 - 0.06f * pn), RD_CARD, 1, v);
  rt_icon(u.next, RT_NEXT, RT_NEXT_X, RT_BTN_Y, 0.83f, RD_TEXT, skip_a, &u.next_bg, v);
  rt_circle(u.play, RT_PLAY_X, RT_BTN_Y, RT_PLAY_R * (1 - 0.06f * pl), RD_ORANGE, 1, v);
  rt_icon(u.play_ic, playing ? RD_STOP : RD_PLAY, RT_PLAY_X + (playing ? 0 : 3), RT_BTN_Y, 1.08f, RD_ON_ORANGE, 1, &u.play, v);
}

// ---------- Drawing ----------
static void rt_fade_col(int x, float k, int y0, int y1) {  // a column toward black (k: 0 — black, 1 — as it is)
  if (x < hb.x0 || x > hb.x1) return;
  for (int y = max(y0, hb.y); y <= min(y1, hb.y + hb.rows - 1); y++) halo_blend(halo_row(y) + x, 0, 0, 0, (int)((1 - k) * 256));
}
static void rt_text_clip(const HaloFont &f, const char *s, float x, float top, float sc, uint32_t color, float xa, float xb) {
  const int sx0 = hb.x0, sx1 = hb.x1;
  hb.x0 = max(hb.x0, (int)xa), hb.x1 = min(hb.x1, (int)xb);
  if (hb.x0 <= hb.x1) halo_text(f, s, x, top, color, 1, 0, sc, 0, 0);
  hb.x0 = sx0, hb.x1 = sx1;
}
static inline void rt_put(uint16_t *p, uint32_t c, float cov) {  // a pixel of colour c, cov of it over what is there
  if (cov >= 0.98f) *p = halo_565(c);
  else if (cov > 0.02f) halo_blend(p, c >> 19 & 31, c >> 10 & 63, c >> 3 & 31, (int)(cov * 256));
}
// A capsule from a to b, half-width h, blended over what is there (a knob's notch)
static void HALO_HOT rt_capsule(float ax, float ay, float bx, float by, float h, uint32_t c) {
  const float x0 = fminf(ax, bx) - h - 1, x1 = fmaxf(ax, bx) + h + 1, y0 = fminf(ay, by) - h - 1, y1 = fmaxf(ay, by) + h + 1;
  if (!halo_hits(x0, y0, x1, y1)) return;
  const float dx = bx - ax, dy = by - ay, il2 = 1 / fmaxf(0.01f, dx * dx + dy * dy);
  const int ya = max((int)y0, hb.y), yb = min((int)y1, hb.y + hb.rows - 1), xa = max((int)x0, hb.x0), xb = min((int)x1, hb.x1);
  for (int y = ya; y <= yb; y++) {
    uint16_t *row = halo_row(y);
    const float py = y + 0.5f - ay;
    for (int x = xa; x <= xb; x++) {
      const float px = x + 0.5f - ax, t = halo_clamp01((px * dx + py * dy) * il2), ex = px - t * dx, ey = py - t * dy;
      const float d2 = ex * ex + ey * ey;
      if (d2 >= (h + 0.5f) * (h + 0.5f)) continue;
      rt_put(row + x, c, h + 0.5f - halo_sqrt(d2));
    }
  }
}
// The needle's mark at the window's top: a small triangle pointing down
static void HALO_HOT rt_mark(float cx, float y0, float y1, float hw, uint32_t c) {
  if (!halo_hits(cx - hw - 1, y0, cx + hw + 1, y1)) return;
  for (int y = max((int)y0, hb.y); y <= min((int)y1, hb.y + hb.rows - 1); y++) {
    const float w = hw * (y1 - (y + 0.5f)) / (y1 - y0);
    if (w <= 0) continue;
    uint16_t *row = halo_row(y);
    for (int x = max((int)(cx - w - 1), hb.x0); x <= min((int)(cx + w + 1), hb.x1); x++) rt_put(row + x, c, halo_clamp01(w + 0.5f - fabsf(x + 0.5f - cx)));
  }
}
static void rt_song_draw() {
  RtUi &u = *rt_u;
  const HaloView &v = rt_v;
  if (v.a < 0.004f) return;
  const uint32_t c = halo_dim(rt_song_err() ? RD_ERR : RD_DIM, (int)(v.a * 256));
  const float top = halo_view_y(v, RT_SONG_Y) - rt_mid_t24 * v.s;
  if (u.song_w > RT_SONG_W) {  // two copies 120 px apart, the ends fade over 24 px
    const float xa = halo_view_x(v, CX - RT_SONG_W / 2), xb = halo_view_x(v, CX + RT_SONG_W / 2);
    const float x0 = halo_view_x(v, CX - RT_SONG_W / 2 - rt_marquee(u.song_w, RT_SONG_W));
    rt_text_clip(gl_rr_t24, rp_song, x0, top, v.s, c, xa, xb);
    rt_text_clip(gl_rr_t24, rp_song, x0 + (u.song_w + 120) * v.s, top, v.s, c, xa, xb);
    const int y0 = (int)top, y1 = (int)(top + gl_rr_t24.line * v.s);
    for (int e = 0; e < 24; e++) rt_fade_col((int)xa + e, e / 24.0f, y0, y1), rt_fade_col((int)xb - 1 - e, e / 24.0f, y0, y1);
  } else {
    halo_text(gl_rr_t24, rp_song, halo_view_x(v, CX), top, c, 1, 1, v.s, 0, 0);
  }
}
// Is a part of the screen (laid out at x0..x1, y0..y1) in this job's band: the items are in PSRAM, and every job went
// through all ~200 of them (32–36 jobs a frame while the ticks dance) — each group only where it is
static bool rt_here(float x0, float y0, float x1, float y1) {
  const HaloView &v = rt_v;
  return halo_hits(halo_view_x(v, x0), halo_view_y(v, y0), halo_view_x(v, x1), halo_view_y(v, y1));
}
static void rt_draw_keys(RtUi &u, const HaloView &v) {  // the band keys (more than ten scroll: cut to their row, they fade out)
  const int nf = min(RD_FOLDER_N, RD_FOLDER_MAX);
  const int sx0 = hb.x0, sx1 = hb.x1, xa = (int)halo_view_x(v, RT_KEY_X0 - 2), xb = (int)halo_view_x(v, RT_KEYS_X1 + 2);
  hb.x0 = max(hb.x0, xa), hb.x1 = min(hb.x1, xb);
  if (hb.x0 <= hb.x1)
    for (int i = 0; i < nf; i++) halo_item_draw(u.key[i]), halo_item_draw(u.led[i]), halo_item_draw(u.kword[i]);
  hb.x0 = sx0, hb.x1 = sx1;
  if (u.key_most > 0) {
    const int y0 = (int)halo_view_y(v, RT_KEYS_Y0 - 3), y1 = (int)halo_view_y(v, RT_KEYS_Y1 + 3);
    if (u.key_off < u.key_most - 0.5f)
      for (int e = 0; e < 48; e++) rt_fade_col(xb - e, e / 48.0f, y0, y1);
    if (u.key_off > 0.5f)
      for (int e = 0; e < 36; e++) rt_fade_col(xa + e, e / 36.0f, y0, y1);
  }
}
// The ticks, column by column straight into the band: their sides by coverage, on black (nothing is under them)
static void HALO_HOT rt_ticks_draw() {
  const HaloView &v = rt_v;
  if (v.a < 0.004f) return;
  const float yb = halo_view_y(v, RT_TICK_Y);
  for (int i = 0; i < rt_tk_n; i++) {
    const RtTick &t = rt_tk[i];
    const float xa = halo_view_x(v, t.x - t.w / 2), xb = halo_view_x(v, t.x + t.w / 2), ya = halo_view_y(v, t.top);
    if (!halo_hits(xa, ya, xb, yb)) continue;
    const float al = t.a * v.a;
    const int y0 = max(halo_floor(ya), hb.y), y1 = min(halo_floor(yb) - 1, hb.y + hb.rows - 1);
    for (int cx = max(halo_floor(xa), hb.x0); cx <= min(halo_floor(xb), hb.x1); cx++) {
      const float cov = fminf(cx + 1.0f, xb) - fmaxf((float)cx, xa);
      if (cov <= 0.02f) continue;
      const uint16_t px = halo_565(halo_dim(t.c, (int)(al * fminf(cov, 1) * 256)));
      for (int y = y0; y <= y1; y++) halo_row(y)[cx] = px;
    }
  }
}
static void rt_draw_window(RtUi &u, const HaloView &v) {  // the frame, the ticks, the logos, the names, the needle
  for (int c = 0; c < 4; c++) halo_item_draw(u.win_side[c]), halo_item_draw(u.win_corner[c]);
  rt_ticks_draw();
  for (int j = u.lv0; j < u.lv1; j++) halo_item_draw(u.logo[j]), halo_item_draw(u.mono[j]);
  for (const auto &e : u.eq) halo_item_draw(e);
  halo_item_draw(u.ring);
  for (int j = u.lv0; j < u.lv1; j++) halo_item_draw(u.lab[j]);
  halo_item_draw(u.msg), halo_item_draw(u.needle);
  if (v.a > 0.004f)
    rt_mark(halo_view_x(v, CX), halo_view_y(v, RT_WIN_Y0 + 4), halo_view_y(v, RT_WIN_Y0 + 18), 12 * v.s, halo_dim(RD_ORANGE, (int)(v.a * 256)));
}
static void rt_draw_knobs(RtUi &u, const HaloView &v) {  // the knobs; previous, Stop / Play, next
#ifdef RT_PILLS  // (the square: the volume's pills and line instead of the knobs)
  for (const auto &it : u.vbtn) halo_item_draw(it);
  for (const auto &it : u.vsign) halo_item_draw(it);
  for (const auto &it : u.vline) halo_item_draw(it);
  halo_item_draw(u.prev_bg), halo_item_draw(u.prev), halo_item_draw(u.next_bg), halo_item_draw(u.next);
  halo_item_draw(u.play), halo_item_draw(u.play_ic);
  (void)v;
  return;
#endif
  halo_item_draw(u.arc_track), halo_item_draw(u.arc_lvl);
  for (int k = 0; k < 2; k++) {
    const float cx = k ? RT_TUNK_X : RT_VOLK_X;
    if (!rt_here(cx - RT_KNOB_R - 20, RT_KNOB_Y - RT_KNOB_R - 20, cx + RT_KNOB_R + 20, RT_KNOB_Y + RT_KNOB_R + 40)) continue;
    halo_item_draw(u.body[k]), halo_item_draw(u.inner[k]);
    for (const auto &d : u.dot[k]) halo_item_draw(d);
    halo_item_draw(u.klab[k]);
    halo_item_draw(u.klab[k]);
    if (v.a > 0.004f) {  // the notch: from 0.42 to 0.8 of the radius, white on the volume, orange on the tuning
      const float a = rt_notch[k] * (HALO_PI / 180), s = sinf(a), c = -cosf(a);
      rt_capsule(halo_view_x(v, cx + RT_KNOB_R * 0.42f * s), halo_view_y(v, RT_KNOB_Y + RT_KNOB_R * 0.42f * c),
                 halo_view_x(v, cx + RT_KNOB_R * 0.8f * s), halo_view_y(v, RT_KNOB_Y + RT_KNOB_R * 0.8f * c), 3.5f * v.s,
                 halo_dim(k ? RD_ORANGE : RD_TEXT, (int)(v.a * 256)));
    }
  }
  halo_item_draw(u.prev_bg), halo_item_draw(u.prev), halo_item_draw(u.next_bg), halo_item_draw(u.next);
  halo_item_draw(u.play), halo_item_draw(u.play_ic);
}

#ifdef HALO_WATCH
// ---------- The watch's poster (variant 3A of watch206-media.html; the round board's main screen, made straight) ----------
// The time on top; the station big — one line of 52 px, two, else two of 32 (a long one cut); its song as a marquee;
// STATIONS (the tuner above) and VOLUME (the volume screen) — an icon over a spaced word, a thin line between; the
// equalizer as rays from the left and the right edges (the low bands at the bottom); Stop / Play — a big orange pill.
// A swipe left / right — the next / previous station of the dial, as on the round board
static void rt_poster_update(float dt) {
  RtUi &u = *rt_u;
  HaloFinger &f = halo_finger;
  if (vol_update(dt)) return void(rt_armed = RT_NONE);  // the volume screen: "+", "−"; elsewhere or 3 s — closed
  if (f.drag_start && f.horiz && rd_list_n > 1 && halo_finger_claim(&rp_owner)) rp_drag = true, rt_armed = RT_NONE;
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
  if (f.pressed && !f.owner) {
    rt_armed = RT_NONE;
    if (fabsf(f.y0 - PT_BTN_Y) < PT_BTN_H / 2 + 10 && fabsf(f.x0 - CX) < PT_BTN_W / 2 + 10) rt_armed = RT_PLAY_B;
    else if (fabsf(f.y0 - (PT_IC_Y + PT_WORD_Y) / 2) < 44 && fabsf(f.x0 - (CX - PT_WORD_DX)) < 62) rt_armed = RT_STATIONS;
    else if (fabsf(f.y0 - (PT_IC_Y + PT_WORD_Y) / 2) < 44 && fabsf(f.x0 - (CX + PT_WORD_DX)) < 62) rt_armed = RT_VOLUME;
  }
  if (f.dragging) rt_armed = RT_NONE;
  if (f.tap) {
    if (rt_armed == RT_PLAY_B) rd_toggle();
    else if (rt_armed == RT_STATIONS) rt_mode(false);
    else if (rt_armed == RT_VOLUME) vol_open();
  }
  press_update(u.p_pb, rt_armed == RT_PLAY_B && f.down, dt);
  press_update(u.p_st, rt_armed == RT_STATIONS && f.down, dt);
  press_update(u.p_vo, rt_armed == RT_VOLUME && f.down, dt);
  if (!f.down) rt_armed = RT_NONE;
}

// The name in one or two lines of the big font, else two of the small one (the second cut to fit)
static void rt_poster_wrap(RtUi &u) {
  const char *s = rp_name_s;
  for (const HaloFont *f : {&gl_ra_n52, &gl_ra_n32}) {
    if (halo_text_width(*f, s) <= PT_NAME_W) {
      u.pt_f = f, u.pt_lines = 1, rd_copy(u.pt_line[0], s, sizeof(u.pt_line[0])), u.pt_line[1][0] = 0;
      return;
    }
    const int n = strlen(s);
    int best = -1;
    for (int i = 1; i < n; i++) {  // the space nearest the middle with both halves fitting
      if (s[i] != ' ') continue;
      char a[RD_NAME + 8];
      rd_copy(a, s, min(i + 1, (int)sizeof(a)));
      if (halo_text_width(*f, a) <= PT_NAME_W && halo_text_width(*f, s + i + 1) <= PT_NAME_W && (best < 0 || abs(i - n / 2) < abs(best - n / 2))) best = i;
    }
    if (best > 0) {
      u.pt_f = f, u.pt_lines = 2;
      rd_copy(u.pt_line[0], s, min(best + 1, (int)sizeof(u.pt_line[0]))), rd_copy(u.pt_line[1], s + best + 1, sizeof(u.pt_line[1]));
      return;
    }
  }
  u.pt_f = &gl_ra_n32, u.pt_lines = 2;  // (longer: the first word or words, the rest cut)
  const char *sp = strchr(s, ' ');
  const int i = sp ? (int)(sp - s) : (int)strlen(s);
  char a[RD_NAME + 8];
  rd_copy(a, s, min(i + 1, (int)sizeof(a)));
  rd_fit(gl_ra_n32, a, PT_NAME_W, u.pt_line[0], sizeof(u.pt_line[0]));
  rd_fit(gl_ra_n32, sp ? sp + 1 : "", PT_NAME_W, u.pt_line[1], sizeof(u.pt_line[1]));
}

static void rt_poster_layout(const HaloView &v) {
  RtUi &u = *rt_u;
  gl_text_mid(u.pt_time, gl_ra_t24, rp_time, rd_mid_t24, CX, PT_TIME_Y, 1, 0xC9CACF, 1, &halo_black);
  halo_view_commit(u.pt_time, v);
  if (u.pt_gen != rp_name_gen || !u.pt_f) u.pt_gen = rp_name_gen, rt_poster_wrap(u);
  const float slide = rp_slide * 60 + rp_drag_x, na = 1 - fminf(1, fabsf(rp_slide) * 0.9f);
  const float mid = u.pt_f == &gl_ra_n52 ? rd_mid_n52 : rd_mid_n32, lh = u.pt_f == &gl_ra_n52 ? 56 : 38;
  for (int k = 0; k < 2; k++) {
    const float y = PT_NAME_Y + (u.pt_lines == 2 ? (k ? lh / 2 : -lh / 2) : 0);
    gl_text_mid(u.pt_name[k], *u.pt_f, u.pt_line[k], mid, CX + slide, y, 1, RD_TEXT, k < u.pt_lines ? na : 0, &halo_black);
    halo_view_commit(u.pt_name[k], v);
  }
  // The song: drawn by radio_draw (a marquee) — its place redraws when it changes
  if (u.pt_song_gen != rp_song_gen) u.pt_song_gen = rp_song_gen, u.pt_song_w = halo_text_width(gl_ra_t20, rp_song);
  const float mq = v.a > 0.004f ? rt_marquee(u.pt_song_w, PT_SONG_W) : 0, hh = gl_ra_t20.line / 2.0f;
  rt_damage(u.pt_song_dmg, rd_hash(rp_song) ^ (uint32_t)(int32_t)(mq * 16) ^ (rt_song_err() ? 0x5bd1e995u : 0),
            CX - PT_SONG_W / 2, PT_SONG_Y - hh, CX + PT_SONG_W / 2, PT_SONG_Y + hh);
  // STATIONS and VOLUME: an icon over a spaced word (pressed — dimmer), the line between
  static const char *const WORD[2] = {"STATIONS", "VOLUME"};
  for (int k = 0; k < 2; k++) {
    const float x = CX + (k ? PT_WORD_DX : -PT_WORD_DX), pr = halo_clamp01((k ? u.p_vo : u.p_st).s.x), a = 1 - 0.4f * pr;
    gl_text_mid(u.pt_ic[k], gl_ra_ic24, k ? RD_VOL : RD_BARS, rd_mid_ic24, x, PT_IC_Y, 1, RD_ORANGE, a, &halo_black);
    gl_text_mid(u.pt_word[k], gl_ra_t14, WORD[k], rd_mid_t14, x, PT_WORD_Y, 1, RD_ORANGE, a, &halo_black);
    u.pt_word[k].spacing = 2;
    halo_view_commit(u.pt_ic[k], v), halo_view_commit(u.pt_word[k], v);
  }
  rt_rrect(u.pt_div, CX, (PT_IC_Y + PT_WORD_Y) / 2, 2, 30, 1, 0x5A3A20, 1, v);
  // Stop / Play
  const bool playing = rd_on && rd_has_cur;
  const float pb = halo_clamp01(u.p_pb.s.x), k = 1 - 0.05f * pb;
  rt_rrect(u.pt_btn, CX, PT_BTN_Y, PT_BTN_W * k, PT_BTN_H * k, PT_BTN_H / 2 * k, RD_ORANGE, 1, v);
  const char *word = playing ? "Stop" : "Play";
  const float tw = halo_text_width(gl_ra_t24, word), x0 = CX - (tw + 34) / 2;
  rt_icon(u.pt_btn_ic, playing ? RD_STOP : RD_PLAY, x0 + 10, PT_BTN_Y, 0.5f, RD_ON_ORANGE, 1, &u.pt_btn, v);
  gl_text_mid(u.pt_btn_t, gl_ra_t24, word, rd_mid_t24, x0 + 34 + tw / 2, PT_BTN_Y, 1, RD_ON_ORANGE, 1, &u.pt_btn);
  halo_view_commit(u.pt_btn_t, v);
  // The rays (drawn by radio_draw): both sides redraw when the levels move
  uint32_t h = 2166136261u;
  for (int i = 0; i < RD_BANDS; i++) h = (h ^ (uint32_t)(int32_t)(ru->lv[i] * 64)) * 16777619u;
  rt_damage(u.pt_ray_dmg[0], h, 0, PT_RAY_Y0 - 6, PT_RAY_X + PT_RAY_L0 + PT_RAY_L1 + 6, PT_RAY_Y1 + 6);
  rt_damage(u.pt_ray_dmg[1], h ^ 0x9e3779b9u, W - PT_RAY_X - PT_RAY_L0 - PT_RAY_L1 - 6, PT_RAY_Y0 - 6, W, PT_RAY_Y1 + 6);
}

static void rt_poster_draw() {
  RtUi &u = *rt_u;
  const HaloView &v = rt_v;
  halo_item_draw(u.pt_time);
  for (const auto &it : u.pt_name) halo_item_draw(it);
  for (int k = 0; k < 2; k++) halo_item_draw(u.pt_ic[k]), halo_item_draw(u.pt_word[k]);
  halo_item_draw(u.pt_div), halo_item_draw(u.pt_btn), halo_item_draw(u.pt_btn_ic), halo_item_draw(u.pt_btn_t);
  if (v.a < 0.004f) return;
  // The song: a marquee cut to its width (two copies 120 px apart, the ends fading)
  const uint32_t c = halo_dim(rt_song_err() ? RD_ERR : 0xC9CACF, (int)(v.a * 256));
  const float top = halo_view_y(v, PT_SONG_Y) - rd_mid_t20 * v.s;
  if (rt_here(0, PT_SONG_Y - 20, W, PT_SONG_Y + 20)) {
    if (u.pt_song_w > PT_SONG_W) {
      const float xa = halo_view_x(v, CX - PT_SONG_W / 2), xb = halo_view_x(v, CX + PT_SONG_W / 2);
      const float x0 = halo_view_x(v, CX - PT_SONG_W / 2 - rt_marquee(u.pt_song_w, PT_SONG_W));
      rt_text_clip(gl_ra_t20, rp_song, x0, top, v.s, c, xa, xb);
      rt_text_clip(gl_ra_t20, rp_song, x0 + (u.pt_song_w + 120) * v.s, top, v.s, c, xa, xb);
      const int y0 = (int)top, y1 = (int)(top + gl_ra_t20.line * v.s);
      for (int e = 0; e < 24; e++) rt_fade_col((int)xa + e, e / 24.0f, y0, y1), rt_fade_col((int)xb - 1 - e, e / 24.0f, y0, y1);
    } else {
      halo_text(gl_ra_t20, rp_song, halo_view_x(v, CX), top, c, 1, 1, v.s, 0, 0);
    }
  }
  // The rays: a capsule a band each side, from the edge inward — the radio's orange, lighter when loud
  if (!rt_here(0, PT_RAY_Y0 - 4, W, PT_RAY_Y1 + 4)) return;
  for (int i = 0; i < RD_BANDS; i++) {
    const float y = PT_RAY_Y1 - (PT_RAY_Y1 - PT_RAY_Y0) * i / (RD_BANDS - 1.0f), l = ru->lv[i];
    const float len = PT_RAY_L0 + PT_RAY_L1 * l;
    const uint32_t col = halo_dim(l > 0.05f ? rt_ray_col(l) : RD_RAY_OFF, (int)(v.a * 256));
    for (int side = 0; side < 2; side++) {
      const float x0 = side ? W - PT_RAY_X : PT_RAY_X, x1 = side ? x0 - len : x0 + len;
      rt_capsule(halo_view_x(v, x0), halo_view_y(v, y), halo_view_x(v, x1), halo_view_y(v, y), 2.5f * v.s, col);
    }
  }
}
#endif

static void radio_draw() {
  if (!ru || !rt_u) return;
  RtUi &u = *rt_u;
  const HaloView &v = rt_v;
#ifdef HALO_WATCH
  if (u.poster) return rt_poster_draw(), vol_draw();
#endif
  if (rt_here(0, 0, W, RT_KEYS_Y1 + 10)) rt_draw_keys(u, v);
  if (rt_here(0, RT_MID_Y0, W, RT_MID_Y1)) {
    halo_item_draw(u.name);
    rt_song_draw();
    halo_item_draw(u.time), halo_item_draw(u.heart_bg), halo_item_draw(u.heart);
  }
  if (rt_here(RT_WIN_X0 - 6, RT_WIN_Y0 - 6, RT_WIN_X1 + 6, RT_WIN_Y1 + 6)) rt_draw_window(u, v);
  if (rt_here(0, RT_WIN_Y1 + 4, W, HALO_H)) rt_draw_knobs(u, v);
#ifdef HALO_WATCH
  vol_draw();
#endif
}

// The round board's main screen is made of parts (face.h FL_RTIME … FL_RPLAY); a rectangle has none
static void rd_part_free(FaceRun &) {}
static void radio_part_base(int, float &x, float &y) { x = CX, y = CY; }  // (the editor's studio_bases)
static void radio_part_build(const FaceScreen &, int) {}
static void radio_part_layout(const FaceScreen &S, int i, float, const HaloView &) {
  FaceRun &R = S.run[i];
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
}
static void radio_part_draw(const FaceScreen &, int) {}
