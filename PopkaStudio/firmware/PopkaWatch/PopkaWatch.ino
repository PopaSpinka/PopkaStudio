// PopkaWatch — the board firmware of PopkaStudio (PopkaStudio/README.md): the screen made in the editor.
//
// The home is HaloWatch's face (projects/HaloWatch/face.h), one to one, made of layers — the date, the clock and the
// weather pill, as many of each as the editor put on the screen, where it put them and as it styled them (face.h); the
// other screens are made of the same layers and open over the face from a layer with a link, as HaloWatch opens its
// Weather screen from the pill (Fade; back — a swipe up from the bottom edge). HaloWatch's Weather screen and its
// gallery are layers too (wxscreen.h, gallery.h): the gallery is reached by a swipe up on the face, its cards open
// their screens with the Android 16 transition (back — a swipe from the left edge). The editor writes layers.h and
// assets/halo.conf (the fonts and icons at the sizes picked); tools/halo_assets.py makes them into src/halo_assets.*.
// The other parts of HaloWatch (Always On, the buttons, the screens behind the gallery's cards) come into PopkaStudio
// one at a time.
//
// The editor builds a copy of this folder with its own layers in build/PopkaStudio/PopkaWatch and flashes that; this
// folder holds the default — HaloWatch's face as it is, so ./tools/flash.sh PopkaStudio/firmware/PopkaWatch shows it.
// The same code runs in the editor's browser page (PopkaStudio/engine): what the page shows is what the board shows.
// Commands: demo, wx N, tick [N], day (face.h); aod, idle, sleep, wake (power.h); time N; live (live.h); hello (hello.h);
// ntp, wifi, wifi scan
// (with Wi-Fi: HaloClock.h); Halo's own — face / face off, back,
// boot / boot long, pwr / pwr long, steps (power.h, steps.h), shot, trace, sys, log ...
#pragma GCC optimize("O3", "fast-math")
#include <Halo.h>
#include "config.h"  // the editor's Functions that decide what is built at all: Wi-Fi, Bluetooth; the zone, 12/24 h
#if FN_WIFI
#include <HaloClock.h>  // the time from the internet (and the networks the board sees: the command wifi scan)
#if __has_include("wifi_net.h")
#include "wifi_net.h"  // the network picked in the editor — written into the editor's build copy only, never into git
#endif
#else
static inline bool halo_clock_valid() {  // no Wi-Fi: the time comes from the computer (studio.py: the command time)
  return time(nullptr) > 1735689600;
}
#include "wifiscan.h"  // no Wi-Fi function: still "wifi scan" — the editor's Wi-Fi window lists the networks it sees
#endif
#include "src/halo_assets.h"

#define W 466
#define CX 233.0f

#include "weather.h"  // the weather: the editor's cities (wxdata.h) and their weather
#include "face.h"     // the watch face made of layers
#include "wxfetch.h"  // fresh weather from Open-Meteo over Wi-Fi (only with Wi-Fi)
#include "power.h"    // the Functions: Always On, the BOOT and PWR buttons
#include "steps.h"    // today's steps (only with a Steps layer)
#include "bt.h"       // Bluetooth (only when it is on)
#include "live.h"     // Live: the layers' looks from the editor over USB, at once (no Load)
#include "hello.h"    // hello: who this board is — its model, the firmware, the project (the editor asks)

static void update(float dt) {
  halo_finger_update();  // the finger — once a frame, first
  live_tick();           // the editor's Live: the layers it sent since the last frame (it wakes the screen)
  power_tick(dt);        // Always On, sleep and the buttons — before the face: in Always On its digits don't roll
  steps_tick();          // today's steps (the motion chip counts them)
  settings_tick(dt);     // Display: the brightness glides to the slider's value (power.h sends it); Battery: its words
  wx_poll();             // new weather from the Wi-Fi task — to the pills and pages (it rolls)
  face_tick(dt);         // the time, the date and the layers' animations, on every screen
  power_view();          // the whole interface's view on the way to Always On (after face_tick: its minute is fresh)
  gallery_tick();        // a card tapped in the last frame: open its screen (face.h)
  halo_nav_update(dt);   // the face, the gallery, the screens open over them, the transitions, the ways back
  power_lock_layout();   // the lock at the top — over everything
}

static void draw() {  // both cores, each its part of a band
  halo_nav_draw();
  power_lock_draw();
}

void setup() {
  face_build();
  gallery_begin();      // the gallery (if the project has one) — lib/Halo's main screen: a swipe up on the face
  halo_nav_face(FACE);  // the home: the board starts on it
  HaloConfig cfg;
  cfg.name = "PopkaWatch";
  settings_load();  // Display's saved brightness, the battery's history (flash)
  cfg.brightness = pw_awake_level;  // Display's (else HaloWatch's default: Settings -> Display, step 7)
  halo_begin(update, draw, cfg);
  power_build();         // the motion sensor (it wakes the board when picked up) and the commands aod, idle, sleep
  steps_build();         // the chip's pedometer, today's steps from flash
  live_build();          // the command live (the editor's Live)
  hello_build();         // the command hello (the editor: is this board ours, and which project)
  setenv("TZ", TZ, 1), tzset();
  // The time from the computer: studio.py sends "time <seconds since 1970, UTC>" after Flash (also with Wi-Fi: the
  // clock is right at once, before the first sync)
  halo_command("time", [](const char *a) {
    const long long t = atoll(a);
    if (t < 1735689600) return;
#ifndef __EMSCRIPTEN__  // (the editor's page has the computer's clock already)
    const timeval tv = {(time_t)t, 0};
    settimeofday(&tv, nullptr);
#endif
    LOG("time: set by the computer\n");
  });
#if FN_WIFI
  wx_begin();  // the weather with every Wi-Fi wake (every 30 minutes)
#ifdef PW_WIFI_SSID
  halo_clock_begin(TZ, PW_WIFI_SSID, PW_WIFI_PASS);  // Wi-Fi — to learn the time; the editor's network
#else
  halo_clock_begin(TZ);  // Wi-Fi — to learn the time; the shared network (HaloClock.h), if there is one
#endif
#endif
  bt_begin();
#if !FN_WIFI
  wifiscan_build();  // (the editor's Wi-Fi window: the networks around, before the first Load with Wi-Fi)
#endif
  Serial.println("setup done");
}

void loop() {
  halo_loop();
  steps_write();  // flash: here, not in the frame task
  bt_tick();      // Bluetooth starts a moment after the first frames
  settings_poll();  // Display's brightness and the battery's history into flash
}
