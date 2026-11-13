// steps.h — today's steps for the Steps layer (face.h: FL_STEPS). The motion chip counts them itself, while the board
// sleeps too (lib/Halo: halo_steps_begin, halo_steps — its own pedometer); here they become today's: from local
// midnight, kept in flash (NVS "steps": the day and the count, written from loop() every 20 steps, a minute after the
// last one and when the day changes) so a restart keeps them. The number rolls as the temperature does; in Always On —
// set without rolling. The chip starts counting after 10 steps in a row (and counts those too).
// Only in a firmware with a Steps layer (FN_STEPS — the editor writes it into layers.h); the editor's page has it
// always, with a number to show (the command steps N).
// With the Steps screen (stepscreen.h) also the history: today an hour each and the last days' totals (NVS "steps":
// "hours" — the day and 24 counts, "days" — seven days by their number), written with the count.
// Commands: steps (the count in the log; the page: steps N — set it).
// Included from PopkaWatch.ino after power.h.
#if HAS_STEPS
#ifndef __EMSCRIPTEN__
#include <Preferences.h>
#endif

static uint32_t st_saved;           // counted today before this start (from flash)
static uint32_t st_base;            // halo_steps() at the start of today (0 — of this run)
static int st_day = -1;             // today (year * 400 + day of the year; -1 — the clock is not set yet)
static int st_saved_day = -1;       // the day st_saved is of
static uint32_t st_written;         // the count last written to flash
static uint32_t st_changed_ms;      // when the count last changed
static volatile bool st_write_req;  // loop(): write it
#if HAS_STEPSCR
static uint32_t st_hours_have;      // today's steps already in ss_hour (a new step goes to this hour)
struct StHours { int32_t day; uint16_t h[24]; };  // (NVS "hours")
struct StDays { int32_t d[7]; uint32_t n[7]; };   // (NVS "days")
#endif

static void steps_show(uint32_t n) {  // into every Steps layer: 6,233
  fc_steps = n;
  char s[16];
  snprintf(s, sizeof(s), "%lu", (unsigned long)n);
  const int len = strlen(s);
  char *o = fc_steps_s;
  for (int i = 0; i < len; i++) {
    *o++ = s[i];
    if ((len - 1 - i) % 3 == 0 && i < len - 1) *o++ = ',';
  }
  *o = 0;
  face_set_all(FL_STEPS, fc_steps_s, !fc_still);
}

// Once a second (and right after a wake-up): the chip's count, the day, today's steps
static void steps_tick() {
  static uint32_t checked;
  if (halo_ms() - checked < 1000 && !halo_woke()) return;
  checked = halo_ms();
  const uint32_t total = halo_steps();
  if (halo_clock_valid()) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    const int day = (t.tm_year + 1900) * 400 + t.tm_yday;
#if HAS_STEPSCR
    const int32_t dnum = ss_daynum(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    if (dnum != ss_today) {
      // That day is over (midnight now, or the board was off through it): its total into the week; today's hours from
      // zero (no hours in flash yet but today's count there — that much is already "in the hours")
      if (ss_today >= 0) ss_day_d[ss_today % 7] = ss_today, ss_day_n[ss_today % 7] = st_day >= 0 ? fc_steps : st_saved;
      const bool counted = ss_today < 0 && st_day < 0 && st_saved_day == day;
      memset(ss_hour, 0, sizeof(ss_hour)), st_hours_have = counted ? st_saved : 0;
      ss_today = dnum;
    }
#endif
    if (day != st_day) {
      if (st_day >= 0) st_saved = 0, st_base = total;  // midnight: a new day from here
      else if (st_saved_day != day) st_saved = 0;      // the time is known now: flash had another day's steps
      st_day = day, st_write_req = true;
    }
  }
  const uint32_t today = st_saved + (total - st_base);
#if HAS_STEPSCR
  if (ss_today >= 0 && today > st_hours_have) {  // the new steps — to this hour
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    ss_hour[t.tm_hour] = (uint16_t)min<uint32_t>(65535, ss_hour[t.tm_hour] + (today - st_hours_have));
    st_hours_have = today;
  }
#endif
  if (today != fc_steps) steps_show(today), st_changed_ms = halo_ms();
  if (st_day >= 0 && today != st_written && (today >= st_written + 20 || halo_ms() - st_changed_ms > 60000))
    st_write_req = true;
}

static void steps_build() {  // after power_build (the motion sensor is set up there)
  halo_steps_begin();
#ifndef __EMSCRIPTEN__
  Preferences p;
  if (p.begin("steps", true)) {
    st_saved_day = p.getInt("day", -1), st_saved = st_written = p.getUInt("n", 0);
#if HAS_STEPSCR
    StHours hs;
    StDays ds;
    if (p.getBytes("hours", &hs, sizeof(hs)) == sizeof(hs)) {  // (kept only if it is still that day: steps_tick)
      ss_today = hs.day, memcpy(ss_hour, hs.h, sizeof(ss_hour));
      for (int i = 0; i < 24; i++) st_hours_have += ss_hour[i];
      st_hours_have = max(st_hours_have, st_saved);  // (steps counted but not in the hours — not put in this hour)
    } else {
      st_hours_have = st_saved;
    }
    if (p.getBytes("days", &ds, sizeof(ds)) == sizeof(ds)) memcpy(ss_day_d, ds.d, sizeof(ss_day_d)), memcpy(ss_day_n, ds.n, sizeof(ss_day_n));
#endif
    p.end();
  }
#endif
  steps_show(st_saved + halo_steps());  // (the editor's page: its number at once — the map's copies don't tick)
}

static void steps_write() {  // from loop(): flash may stall the frame task
  if (!st_write_req) return;
  st_write_req = false;
  const uint32_t n = fc_steps;
#ifndef __EMSCRIPTEN__
  Preferences p;
  if (!p.begin("steps", false)) return;
  p.putInt("day", st_day), p.putUInt("n", n);
#if HAS_STEPSCR
  StHours hs = {ss_today, {}};
  memcpy(hs.h, ss_hour, sizeof(hs.h));
  StDays ds;
  memcpy(ds.d, ss_day_d, sizeof(ds.d)), memcpy(ds.n, ss_day_n, sizeof(ds.n));
  p.putBytes("hours", &hs, sizeof(hs)), p.putBytes("days", &ds, sizeof(ds));
#endif
  p.end();
#endif
  st_written = n;
}
#else
static void steps_build() {}
static void steps_tick() {}
static void steps_write() {}
#endif
