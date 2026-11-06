"""PopkaStudio — the editor of Halo watch screens, in the browser; this is its helper on the Mac.

Run:   python3 PopkaStudio/studio.py      (after ./install.sh: popkastudio, i.e. bin/popkastudio — it starts this and opens the page)
then open http://localhost:8466.

The page does the editing; it shows the screen with the board's own code built for the browser (engine/build.sh),
so it needs this helper only for what a browser can't do:
  * fonts and icons — made by tools/halo_assets.py (and HaloWatch's assets/gen_icons.py for the weather icons), the
    same code that makes them for the board, so the page gets exactly the board's pixels;
  * the board: the list of connected boards, and Load — the project becomes the PopkaWatch sketch
    (firmware/PopkaWatch with its own layers.h and halo.conf, in build/PopkaStudio/PopkaWatch), arduino-cli compiles
    and uploads it;
  * the projects — a file each in projects/ (main.json, …) with its picture for the start window (main.png), so the
    work stays when the page is closed.

Other runs:
  studio.py --template   rewrite firmware/PopkaWatch's layers.h, assets and src from the default project (HaloWatch's
                         face) — after a change to what the generator writes
  studio.py --build      only build the page's engine (web/build/PopkaWatch.*)
  studio.py --port N     listen on port N instead of 8466 (--projects DIR: the projects from another folder)
"""
import contextlib
import glob
import http.server
import importlib.util
import io
import json
import os
import re
import shlex
import shutil
import socketserver
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unicodedata
import urllib.parse
import urllib.request
import zlib

import numpy as np

sys.dont_write_bytecode = True  # no __pycache__ next to tools/halo_assets.py and HaloWatch's gen_icons.py
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PY = sys.executable
FIRMWARE = os.path.join(HERE, "firmware/PopkaWatch")
WEB = os.path.join(HERE, "web")
ENGINE_OUT = os.path.join(WEB, "build")
BUILD = os.path.join(ROOT, "build/PopkaStudio")
SKETCH = os.path.join(BUILD, "PopkaWatch")  # what "Flash" compiles: firmware/PopkaWatch + the project's layers
PREVIEW = os.path.join(BUILD, "preview")      # halo_assets.py's previews of the fonts the page asked for
PROJECTS = os.path.join(HERE, "projects")
PORT = 8466
VERSION = "1.2"  # PopkaStudio's version (the About window; the release notes)

sys.path.insert(0, os.path.join(ROOT, "tools"))
import halo_assets as ha  # noqa: E402 — the board's font and image code


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


gi = _load("gen_icons", os.path.join(ROOT, "projects/HaloWatch/assets/gen_icons.py"))  # HaloWatch's weather icons
assets_lock = threading.Lock()  # gen_icons keeps its scale in a module global; halo_assets writes previews

# ---------- The project: what is on the screen ----------
# Screens (firmware/PopkaWatch/face.h): the first is the watch face — the home; the others open over it. Each is a list
# of layers, bottom to top, of HaloWatch's parts: the watch face's date, clock and weather pill (projects/HaloWatch/
# face.h) and the Weather screen's clock along the top edge, city, icon, temperature, high/low, hourly pill and page
# dots (weather.h there; firmware/PopkaWatch/wxscreen.h). A layer has HaloWatch's look by default (assets/halo.conf and
# the code there): its font size, weight (wght), width (wdth), letter spacing and colors; the pills and the icon have
# a size k (1 — as on HaloWatch) instead of a font; the clock along the top edge — the radius of its letters. dx, dy —
# moved from HaloWatch's place of that part, px. A layer of the face may have a link: tap = {"to": screen id, "fx":
# "fade" | "zoom" | "slide"} — a tap opens that screen over the face (HaloWatch's pill: Fade), back — a swipe up from
# the bottom edge. x, y of a screen — its place on the editor's map.
# The gallery's parts (HaloWatch.ino there: the title, the cards, the button at the bottom edge, the scroll indicator;
# firmware/PopkaWatch/gallery.h) have HaloWatch's fonts and places (they don't move: the list places them) and their
# text: text (the title; a card's, the button's), subtitle (the line under a card's title), icon (a card's: a key of
# ICONS); a card or the button may have a link: tap = {"to": screen id, "fx": "a16"} — the Android 16 transition, back
# — a swipe from the left edge (none — a screen with its title). The project's "main" — the screen a swipe up on the
# face opens (HaloWatch's gallery), back — a swipe down from its top edge.
KINDS = {"date": 0, "clock": 1, "weather": 2,                                             # FL_* in face.h
         "arc": 3, "city": 4, "wxicon": 5, "wxtemp": 6, "wxhilo": 7, "hourly": 8, "dots": 9,
         "title": 10, "card": 11, "edge": 12, "scroll": 13, "steps": 14, "stars": 15, "feels": 17,
         "display": 18, "battery": 19, "stnum": 20, "stcap": 21, "stfoot": 22, "stchart": 23,
         "curve": 24, "stepring": 25, "weekarc": 26, "infoline": 27, "forecast": 28, "dateframe": 29, "dotclock": 30,
         "music": 31, "recorder": 32, "snkdots": 33, "snkspace": 34, "snkiso": 35, "batring": 36, "parrot": 37,
         "devcard": 38, "device": 39, "charge": 40}
GALLERY = ("title", "card", "edge", "scroll", "devcard")
TAPS = ("card", "edge", "devcard")  # the gallery's parts a tap on opens a screen (halo_nav_open)
# About (device.h): what a live card or a card's screen shows (p2: DV_CPU …) — the processor, the memory, the battery,
# the temperature, the sound, the motion
DEVS = ("cpu", "mem", "bat", "temp", "snd", "mot")
STEPS_SCREEN = ("stnum", "stcap", "stfoot", "stchart")  # the Steps screen's parts (stepscreen.h)
CURVES = ("weekday", "shortdate", "daymonth", "weather", "steps", "datesteps", "battery")  # widgets.h CV_* (curved text)
INFOS = ("words", "short", "icons")                                             # widgets.h IF_* (the info line)
WHATS = {"curve": CURVES, "devcard": DEVS, "device": DEVS}  # what a layer may show, by its kind


def uses_steps(layer):
    """The layer shows today's steps (the motion chip's pedometer runs, Raise does not wake)."""
    kind = layer["type"]
    return (kind in ("steps", "stepring", "infoline") or kind in STEPS_SCREEN
            or (kind == "curve" and layer.get("what") in ("steps", "datesteps")))
FX = {"zoom": "HALO_FX_ZOOM", "fade": "HALO_FX_FADE", "slide": "HALO_FX_SLIDE"}  # lib/Halo: halo_nav_face_open
MAX_SCREENS, MAX_LAYERS = 24, 32  # what the page's engine has room for (engine/studio_api.h)
# The watch's Functions (firmware/PopkaWatch/power.h; the page: state.js FUNCTIONS — the same): Always On when idle
# (after idle s, the panel at bright of 255), what BOOT and PWR do (on: False — nothing) — act: "off" (the screen off),
# "aod" (Always On at once; the button again — back) or "lock" (Always On, locked; a long press of that button unlocks)
# Waking up (power.h: FN_WAKE_*): a touch (tap), picking it up (raise), a button (btn) — at least one is on; home —
# out of Always On on the watch face (power.h: FN_AOD_HOME), else where it was
# A new project starts with Always On, the buttons and Wi-Fi off — each is switched on when wanted (Wi-Fi: picked for a
# clock's time or with the weather — the editor's Wi-Fi window; without it the time comes from the computer) (6 Nov 2026, the user's
# word: the first firmware is a base, the rest is added like Lego); Always On's burn-in shift (a few pixels every
# minute) is on with it
FUNCTIONS = {"aod": {"on": False, "idle": 15, "bright": 24, "tap": True, "raise": True, "btn": True, "home": False,
                     "shift": True},
             "boot": {"on": False, "act": "off"},
             "pwr": {"on": False, "act": "lock"}, "wifi": {"on": False}, "bt": {"on": False}}
# Wi-Fi (HaloClock: the time from the internet) and Bluetooth (bt.h) — off: not in the firmware at all. The time (the
# page: state.js ZONES, TIME): the zone (POSIX TZ for the board) and 12 or 24 hours; without Wi-Fi the board gets the
# computer's time over USB after Flash (the command time)
ZONES = {"New York": "EST5EDT,M3.2.0,M11.1.0", "Chicago": "CST6CDT,M3.2.0,M11.1.0", "Denver": "MST7MDT,M3.2.0,M11.1.0",
         "Los Angeles": "PST8PDT,M3.2.0,M11.1.0", "London": "GMT0BST,M3.5.0/1,M10.5.0",
         "Berlin": "CET-1CEST,M3.5.0,M10.5.0/3", "Kyiv": "EET-2EEST,M3.5.0/3,M10.5.0/4", "Moscow": "MSK-3",
         "Dubai": "<+04>-4", "Tokyo": "JST-9", "Sydney": "AEST-10AEDT,M10.1.0,M4.1.0/3", "UTC": "UTC0"}
TIME = {"zone": "New York", "h24": False}
# The weather (the page: state.js CITY, WX_MAX): the cities — the Weather screen's pages (the first one — the face's
# pill too) — and °C or °F. Open-Meteo: free, no key, the whole world (geocoding — a city's name; a US ZIP — Zippopotam)
WX_MAX = 6  # firmware/PopkaWatch/weather.h: WX_MAX
CITY = {"name": "New York", "where": "New York, United States", "lat": 40.71427, "lon": -74.00597}
WX_AHEAD = 24  # hours of forecast the board keeps (weather.h)
WX_KINDS = ("WX_PARTLY", "WX_SUN", "WX_CLOUD", "WX_RAIN")  # weather.h, in its order
DEFAULTS = {
    "date": {"name": "Date", "size": 41, "weight": 700, "width": 100, "spacing": 0, "color": "#C2C3CB"},
    "clock": {"name": "Clock", "size": 148, "weight": 580, "width": 106, "spacing": -1, "color": "#F8F7F9"},
    "weather": {"name": "Weather", "scale": 1, "spacing": 0, "color": "#2D2F2F", "color2": "#F8F7F9"},
    # The Weather screen (projects/HaloWatch/weather.h and halo.conf: t24, t_title, w_now, w_hl, w_hour)
    "arc": {"name": "Top clock", "size": 24, "weight": 620, "width": 104, "spacing": 1, "radius": 219, "color": "#C6C6C6"},
    "city": {"name": "City", "size": 34, "weight": 620, "width": 104, "spacing": 0, "color": "#F9F9F9"},
    "wxicon": {"name": "Weather icon", "scale": 1},
    "wxtemp": {"name": "Temperature", "size": 91, "weight": 660, "width": 100, "spacing": 0, "color": "#F9F9F9"},
    "wxhilo": {"name": "High / low", "size": 37, "weight": 660, "width": 100, "spacing": 0, "color": "#F9F9F9",
               "color2": "#A2A2A2"},
    "hourly": {"name": "Hourly", "scale": 1, "spacing": 0, "color": "#2E3132", "color2": "#F9F9F9"},
    "dots": {"name": "Page dots", "color": "#FFFFFF", "color2": "#FFFFFF"},
    # HaloWatch's Settings sections (settings.h): each fills its screen, behind a gallery card
    "display": {"name": "Display"},
    "battery": {"name": "Battery"},
    "music": {"name": "Music"},  # HaloWatch's player (player.h, music.h): the music — the flash's "ffat" partition
    "recorder": {"name": "Recorder"},  # record, play back, forget (recorder.h): the microphones
    # the Games (games.h): self-playing snakes, each its own look
    "snkdots": {"name": "Dot Snake"}, "snkspace": {"name": "Comet"},
    "snkiso": {"name": "Voxel Snake"},
    # The Steps screen's parts (stepscreen.h; PopkaStudio's own): each on both pages — today and the week; the goal (the
    # same in each, and in the Steps pill), the chart's accent (the pill's ring too)
    "stnum": {"name": "Number", "size": 64, "weight": 660, "width": 100, "spacing": 0, "color": "#FFF8EE", "goal": 10000},
    "stcap": {"name": "Caption", "size": 20, "weight": 620, "width": 104, "spacing": 0, "color": "#9E9E9E", "goal": 10000},
    "stfoot": {"name": "Footer", "size": 20, "weight": 620, "width": 104, "spacing": 0, "color": "#FF8A80", "goal": 10000},
    "stchart": {"name": "Chart", "scale": 1, "color": "#FF8A80", "color2": "#4A4C52", "color3": "#7A7A7A", "goal": 10000},
    # The face's widgets from the twenty mockups (widgets.h): what a curved line says (CURVES) and where (angle), the
    # info line's format (INFOS); the editor's Face folder has them ready (state.js FACE_PRESETS)
    "curve": {"name": "Curved text", "what": "shortdate", "angle": 0, "radius": 196, "size": 19, "weight": 650,
              "width": 100, "spacing": 3, "color": "#9E9E9E"},
    "stepring": {"name": "Steps ring", "scale": 1, "radius": 224, "color": "#FF8A80", "color2": "#2A2B2E", "goal": 10000},
    # the battery's charge as Steps ring's (FL_BATRING): the radius goes in p
    "batring": {"name": "Battery ring", "scale": 1, "radius": 224, "color": "#6DD58C", "color2": "#2A2B2E"},
    "weekarc": {"name": "Week", "angle": 0, "radius": 190, "size": 18, "weight": 650, "width": 100, "spacing": 0,
                "color": "#9E9E9E", "color2": "#5E5F64", "color3": "#FF8A80"},
    "infoline": {"name": "Info line", "format": "words", "size": 18, "weight": 620, "width": 100, "spacing": 0,
                 "color": "#9E9E9E", "color3": "#FF8A80"},
    "forecast": {"name": "Forecast", "angle": 0, "scale": 0.5, "radius": 168, "size": 15, "weight": 700, "width": 100,
                 "spacing": 0, "color": "#FFF8EE", "color2": "#5E5F64"},
    "dateframe": {"name": "Date frame", "gap": 160, "size": 15, "weight": 650, "width": 100, "spacing": 4,
                  "color": "#9E9E9E", "color2": "#3A3B40"},
    "dotclock": {"name": "Dot clock", "scale": 1, "color": "#FFF8EE", "color2": "#1E1F23", "color3": "#FF8A80"},
    "feels": {"name": "Feels like", "size": 20, "weight": 620, "width": 104, "spacing": 0, "color": "#A2A2A2"},
    # The gallery (HaloWatch.ino: TEXT, CARD_HI, the scroll indicator's colors); a card — a plain one, to fill in
    "title": {"name": "Title", "text": "Gallery", "color": "#FFF8EE"},
    "card": {"name": "Card", "text": "Card", "subtitle": "Description", "icon": "star", "color": "#A8C7FA",
             "color2": "#2E3034"},
    "edge": {"name": "More button", "text": "More", "color": "#2E3034"},
    "scroll": {"name": "Scroll indicator", "color": "#C6C6C6", "color2": "#333333"},
    # Steps (not on HaloWatch: PopkaStudio's, face.h FL_STEPS): the pill, its number, the ring and the shoe; the goal
    "steps": {"name": "Steps", "scale": 1, "spacing": 0, "color": "#2D2F2F", "color2": "#F8F7F9", "color3": "#FF8A80",
              "goal": 10000, "pill": True},
    # Stars (projects/Orbit's starry background, stars.h FL_STARS): how many, brightness (scale), twinkling, meteors/min
    "stars": {"name": "Stars", "scale": 1, "count": 90, "twinkle": 1, "meteors": 6, "color": "#C8DCFF"},
    # Pixel art drawn cell by cell (pixelart.h): Popka, the parrot — its size (scale), the turn (degrees), its notes
    # (sparkles: off, around it, all over the screen)
    "parrot": {"name": "Popka", "scale": 1, "angle": 0, "sparkles": "around"},
    # About (device.h; PopkaStudio's own, web/mockups/hardware.html variant 2): a gallery card with a live value and
    # picture (what — DEVS; its label, icon, accent, the card's color) and a card's own screen (what, the accent); Power —
    # variant 10's battery page
    "devcard": {"name": "Live card", "what": "cpu", "text": "Processor", "icon": "chip", "color": "#A8C7FA",
                "color2": "#1B1B1F"},
    "device": {"name": "Device screen", "what": "cpu", "color": "#A8C7FA"},
    "charge": {"name": "Power"},
}
PIXEL_ART = ["parrot"]  # the pixel art layers: what is around the picture (p2), its trail (p3)
# The gallery's icons: Font Awesome (tools/fonts/fa-solid-900.ttf) at HaloWatch's ic24 size; the first five — the
# icons of HaloWatch's cards (Pages, Alarm, Music, Weather, Settings). The page has the same list (state.js: ICONS)
ICONS = {"chevron": 0xF054, "bell": 0xF0F3, "music": 0xF001, "sun": 0xF185, "gear": 0xF013, "heart": 0xF004,
         "moon": 0xF186, "gauge": 0xF624, "clock": 0xF017, "calendar": 0xF073, "envelope": 0xF0E0, "phone": 0xF095,
         "wifi": 0xF1EB, "location": 0xF3C5, "user": 0xF007, "house": 0xF015, "search": 0xF002, "play": 0xF04B,
         "chart": 0xF201, "pulse": 0xF21E, "battery": 0xF240, "bolt": 0xF0E7, "volume": 0xF028, "contrast": 0xF042,
         "gamepad": 0xF11B, "camera": 0xF030, "coffee": 0xF0F4, "run": 0xF70C, "snow": 0xF2DC, "drop": 0xF043,
         "cloud": 0xF0C2, "fire": 0xF06D, "star": 0xF005, "bulb": 0xF0EB, "car": 0xF1B9, "plane": 0xF072,
         "stopwatch": 0xF2F2, "image": 0xF03E, "chat": 0xF086, "wrench": 0xF0AD, "lock": 0xF023, "menu": 0xF0C9,
         "tv": 0xF26C, "mic": 0xF130, "chip": 0xF2DB, "memory": 0xF538, "thermo": 0xF2C9}
# HaloWatch's fonts of the gallery (its halo.conf: t_title, t24, t20, ic24) — in every PopkaWatch, as gl_*
GALLERY_FONTS = [
    "font  gl_title  GoogleSansFlex.ttf  34  latin punct \"é\"  @wght=620,wdth=104,ROND=100   # the gallery: titles, stubs",
    "font  gl_t24    GoogleSansFlex.ttf  24  latin punct \"é\"  @wght=620,wdth=104,ROND=100   # cards' titles, the button",
    "font  gl_t20    GoogleSansFlex.ttf  20  latin punct  @wght=620,wdth=104,ROND=100   # the line under a card's title",
    "font  gl_ic24   fa-solid-900.ttf    24  " + " ".join(f"0x{c:04x}" for c in ICONS.values())
    + " 0xf067 0xf068   # cards' icons; the player's + and −",
]
# Display's and Battery's own (HaloWatch's ic32, n48, ic18): only in a firmware with them (and in the template — the
# page's engine draws every project)
SETTINGS_FONTS = {
    "display": ["font  gl_ic32   fa-solid-900.ttf    32  0xf068 0xf067   # Display: the slider's minus and plus"],
    "battery": ["font  gl_n92    GoogleSansFlex.ttf  92  digits \"%-\" @wght=660,ROND=100   # Battery: the percent"],
    "stchart": ["font  gl_t16    GoogleSansFlex.ttf  16  digits \"APMTWFS\" @wght=620,wdth=104,ROND=100   # Steps: hours, days"],
    "recorder": ["font  gl_n44    GoogleSansFlex.ttf  44  digits \":\" @wght=660,ROND=100   # Recorder: the timer",
                 "font  gl_rc32   fa-solid-900.ttf    32  0xf04b 0xf04c 0xf028   # Recorder: play, pause, the volume"],
    ("devcard", "device", "charge"): [
        "font  gl_n80    GoogleSansFlex.ttf  80  digits \"%.,°−-\" @wght=660,ROND=100   # About, Power: the big number",
        "font  gl_n34    GoogleSansFlex.ttf  34  digits punct \"KBdg\" @wght=660,ROND=100   # About: a card's value",
        "font  gl_c16    GoogleSansFlex.ttf  16  digits punct \"ABCDEFGHIJKLMNOPQRSTUVWXYZ\" @wght=620,wdth=104,ROND=100"
        "   # About, Power: the words along the arc, small captions"],
}
TEXT_LEN = 40  # (engine/studio_api.h has room for 40 bytes)
# The letters each kind needs (halo.conf of HaloWatch)
CHARS = {"date": 'digits "ABCDEFGHIJKLMNOPQRSTUVWXYZ, "', "clock": 'digits ":-"', "weather": 'digits "°-"',
         "arc": 'digits ":"', "city": "latin", "wxtemp": 'digits "°-"', "wxhilo": 'digits "-"', "hourly": 'digits "APM:"', "feels": 'digits "Feelsik °-"',
         "steps": 'digits ","', "stnum": 'digits ","', "stcap": 'digits latin punct', "stfoot": 'digits latin punct',
         "curve": 'digits latin punct', "weekarc": '"MTWFS"', "infoline": 'digits latin punct',
         "forecast": 'digits "APM°-"', "dateframe": 'digits latin'}
# The pills' text: its size on the mockup x 1.2 (FACE_SCALE, WX_SCALE), times k; HaloWatch: 58 px, 28 px
PILL_FONT = {"weather": (48 * 1.2, 700), "hourly": (23 * 1.2, 700), "steps": (48 * 1.2, 700)}
STEPS_ICON = (34, 0xF54B)  # the Steps pill's shoe: Font Awesome's shoe-prints, px at size 1
WEATHER_ICONS = ("partly", "sun", "cloud", "rain")  # in the order of WX_PARTLY ... (weather.h)
LIMITS = {"size": (8, 300, int), "weight": (1, 1000, int), "width": (25, 151, int), "spacing": (-30, 60, int),
          "scale": (0.3, 3, float), "radius": (60, 260, float), "dx": (-500, 500, float), "dy": (-500, 500, float),
          "goal": (100, 100000, int), "count": (10, 200, int), "twinkle": (0, 3, float), "meteors": (0, 30, float),
          "angle": (-180, 180, float), "rainbow": (1, 5, float), "gap": (40, 330, int)}


SPARKLES = {"off": 0, "around": 1, "screen": 2}  # what is around a pixel art picture (pixelart.h: p2)


def new_layer(kind, n=0):
    layer = {"id": f"{kind}{n}", "type": kind, "hidden": False, "dx": 0, "dy": 0}
    layer.update(json.loads(json.dumps(DEFAULTS[kind])))
    return layer


def default_project():
    """A new project (6 Nov 2026, the user's word): an empty watch face — a black screen, nothing on it — and the
    Functions off (FUNCTIONS): its first Load is the base Live needs; what the watch shows is added like Lego."""
    return {"format": "popkastudio/2", "board": "amoled175",
            "screens": [{"id": "face", "name": "Watch face", "x": 0, "y": 0, "layers": [], "face": "blankface"}]}


def halowatch_project():
    """HaloWatch's face: the date, the clock and the pill at their places (the firmware folder's own default)."""
    return {"format": "popkastudio/2", "board": "amoled175",
            "screens": [{"id": "face", "name": "Watch face", "x": 0, "y": 0,
                         "layers": [new_layer("date", 1), new_layer("clock", 1), new_layer("weather", 1)]}]}


def migrate(project):
    """popkastudio/1 (one face: {"face": {"layers"}}) -> popkastudio/2 (screens)."""
    if "screens" not in project:
        layers = project.pop("face", {}).get("layers", [])
        project["screens"] = [{"id": "face", "name": "Watch face", "x": 0, "y": 0, "layers": layers}]
        project["format"] = "popkastudio/2"
    return project


def clean_layer(raw):
    """A layer from the page, checked: a known kind, numbers in range (None — not a layer)."""
    kind = raw.get("type")
    if kind not in KINDS:
        return None
    layer = new_layer(kind)
    layer["tap"] = None
    for key in layer:
        if key in raw:
            layer[key] = raw[key]
    for key, (lo, hi, cast) in LIMITS.items():
        if key in layer:
            layer[key] = cast(min(max(float(layer[key]), lo), hi))
    for key in ("color", "color2", "color3"):
        if key in layer and not re.fullmatch(r"#[0-9a-fA-F]{6}", str(layer[key])):
            layer[key] = DEFAULTS[kind][key]
    for key in ("text", "subtitle"):  # the gallery's fonts have Latin letters (and punctuation)
        if key in layer:
            layer[key] = "".join(c for c in str(layer[key]) if " " <= c <= "~")[:TEXT_LEN - 1]
    if "sparkles" in layer:  # (before: on or off — on was "around")
        v = layer["sparkles"]
        layer["sparkles"] = v if v in SPARKLES else "off" if v is False else "screen"
    if "what" in layer and layer["what"] not in WHATS.get(kind, ()):
        layer["what"] = DEFAULTS[kind]["what"]
    if "format" in layer and layer["format"] not in INFOS:
        layer["format"] = DEFAULTS[kind]["format"]
    if "icon" in layer and layer["icon"] not in ICONS:
        layer["icon"] = DEFAULTS[kind]["icon"]
    tap = layer["tap"]
    fx = "a16" if kind in GALLERY else tap.get("fx") if isinstance(tap, dict) and tap.get("fx") in FX else "fade"
    layer["tap"] = {"to": str(tap["to"]), "fx": fx} if isinstance(tap, dict) and tap.get("to") else None
    return layer


def clean_screens(project):
    """The screens of a project from the page, checked; the first is the watch face."""
    out = []
    for raw in migrate(dict(project)).get("screens", [])[:MAX_SCREENS]:
        layers = [layer for layer in map(clean_layer, raw.get("layers", [])[:MAX_LAYERS]) if layer]
        out.append({"id": str(raw.get("id", f"screen{len(out)}")), "name": str(raw.get("name", "Screen")),
                    "layers": layers})
    return out or [{"id": "face", "name": "Watch face", "layers": []}]


def clean_functions(project):
    """The project's Functions, checked; missing ones — FUNCTIONS."""
    raw = project.get("functions") if isinstance(project.get("functions"), dict) else {}
    out = json.loads(json.dumps(FUNCTIONS))
    for key, f in out.items():
        got = raw.get(key) if isinstance(raw.get(key), dict) else {}
        f["on"] = bool(got.get("on", f["on"]))
        if "idle" in f:
            f["idle"] = int(min(max(float(got.get("idle", f["idle"])), 5), 600))
            f["bright"] = int(min(max(float(got.get("bright", f["bright"])), 1), 255))
            for way in ("tap", "raise", "btn", "home", "shift"):
                f[way] = bool(got.get(way, f[way]))
        elif "act" in f:
            f["act"] = got.get("act") if got.get("act") in ("off", "aod", "lock") else f["act"]
    if not (out["aod"]["tap"] or out["aod"]["raise"] or out["aod"]["btn"]):  # (no way to wake: buttons do)
        out["aod"]["btn"] = True
    if out["wifi"]["on"] and out["bt"]["on"]:  # one at a time (internal memory: bt.h) — Wi-Fi keeps the time right
        out["bt"]["on"] = False
    return out


def clean_time(project):
    """The project's time: the zone (a ZONES name) and 12 or 24 hours."""
    raw = project.get("time") if isinstance(project.get("time"), dict) else {}
    return {"zone": raw.get("zone") if raw.get("zone") in ZONES else TIME["zone"],
            "h24": bool(raw.get("h24", TIME["h24"]))}


def clean_weather(project):
    """The project's weather: its cities (1..WX_MAX; none — CITY) and the units ("C" or "F")."""
    raw = project.get("weather") if isinstance(project.get("weather"), dict) else {}
    cities = []
    for c in raw.get("cities") or []:
        try:
            lat, lon = float(c["lat"]), float(c["lon"])
        except (KeyError, TypeError, ValueError):
            continue
        if -90 <= lat <= 90 and -180 <= lon <= 180:
            cities.append({"name": ascii_name(c.get("name")) or "City", "where": str(c.get("where") or "")[:80],
                           "lat": round(lat, 4), "lon": round(lon, 4)})
    return {"cities": cities[:WX_MAX] or [dict(CITY)], "units": "F" if raw.get("units") == "F" else "C"}


def ascii_name(name):
    """A city's name as the watch can draw it: Latin letters (its font — tools/halo_assets.py "latin"), 23 at most."""
    s = unicodedata.normalize("NFKD", str(name or "")).encode("ascii", "ignore").decode()
    return "".join(c for c in s if " " <= c <= "~").strip()[:23]


def kind_of(wmo):
    """Open-Meteo's sky (a WMO code) as one of the watch's four icons (weather.h: wx_kind_of — the same)."""
    wmo = int(wmo if wmo is not None else 3)
    if wmo <= 1:
        return 1  # sun
    if wmo == 2:
        return 0  # partly
    if wmo in (3, 45, 48) or 71 <= wmo <= 77 or wmo in (85, 86):
        return 2  # cloud (snow too)
    return 3  # rain: drizzle, rain, showers, thunder


def fetch_json(url):
    req = urllib.request.Request(url, headers={"User-Agent": "PopkaStudio"})
    with urllib.request.urlopen(req, timeout=8) as r:
        return json.loads(r.read().decode())


def geo_search(q):
    """A city by its name (Open-Meteo geocoding), or a US ZIP (5 digits — Zippopotam: the ZIP's own middle)."""
    q = q.strip()
    if re.fullmatch(r"\d{5}", q):
        try:
            d = fetch_json(f"https://api.zippopotam.us/us/{q}")
        except Exception:  # noqa: BLE001 — no such ZIP: 404
            return []
        return [{"name": ascii_name(pl["place name"]), "where": f"{pl['state abbreviation']} {q}, United States",
                 "lat": float(pl["latitude"]), "lon": float(pl["longitude"])} for pl in d.get("places", [])[:5]]
    if len(q) < 2:
        return []
    d = fetch_json("https://geocoding-api.open-meteo.com/v1/search?count=8&language=en&format=json&name="
                   + urllib.parse.quote(q))
    out = []
    for r in d.get("results", []):
        where = ", ".join(x for x in (r.get("admin1"), r.get("country")) if x)
        out.append({"name": ascii_name(r.get("name")) or r.get("name", ""), "where": where,
                    "lat": float(r["latitude"]), "lon": float(r["longitude"])})
    return out


wx_cache = {}  # (cities' places, units) -> (when, the answer)


def weather_for(cities, units):
    """What each city's weather is now — the fields the board asks for itself (wxfetch.h): now, feels like, today's
    high and low, the sky now and for the next WX_AHEAD hours (kinds); cached for 10 minutes."""
    key = (tuple((c["lat"], c["lon"]) for c in cities), units)
    if key in wx_cache and time.time() - wx_cache[key][0] < 600:
        return wx_cache[key][1]
    url = ("http://api.open-meteo.com/v1/forecast?latitude=" + ",".join(f"{c['lat']:.4f}" for c in cities)
           + "&longitude=" + ",".join(f"{c['lon']:.4f}" for c in cities)
           + "&current=temperature_2m,apparent_temperature,weather_code&hourly=weather_code,temperature_2m"
           + f"&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=2&forecast_hours={WX_AHEAD}"
           + "&timeformat=unixtime" + ("&temperature_unit=fahrenheit" if units == "F" else ""))
    d = fetch_json(url)
    d = d if isinstance(d, list) else [d]
    rnd = lambda v: None if v is None else int(round(v))  # noqa: E731
    out = []
    for c, e in zip(cities, d):
        cur, hourly, daily = e.get("current", {}), e.get("hourly", {}), e.get("daily", {})
        out.append({"now": rnd(cur.get("temperature_2m")), "feels": rnd(cur.get("apparent_temperature")),
                    "hi": rnd((daily.get("temperature_2m_max") or [None])[0]),
                    "lo": rnd((daily.get("temperature_2m_min") or [None])[0]),
                    "k": kind_of(cur.get("weather_code")), "t0": (hourly.get("time") or [0])[0],
                    "hk": [kind_of(w) for w in (hourly.get("weather_code") or [])][:WX_AHEAD],
                    "ht": [rnd(v) for v in (hourly.get("temperature_2m") or [])][:WX_AHEAD]})
    wx_cache[key] = (time.time(), out)
    return out


def write_wxdata(project, folder):
    """wxdata.h: the cities and the weather the editor has now (the board shows it until it gets its own)."""
    wx = clean_weather(project)
    data = [{} for _ in wx["cities"]]  # (the template in git: no numbers — "--" until the board gets its own)
    if os.path.abspath(folder) != os.path.abspath(FIRMWARE):
        try:
            data = weather_for(wx["cities"], wx["units"])
        except Exception:  # noqa: BLE001 — no internet: "--" until the board gets its own
            pass
    num = lambda v: "WX_NONE" if v is None else str(v)  # noqa: E731
    h = ["// Generated by PopkaStudio (studio.py) from the editor's project — do not edit by hand.",
         "// The cities (the Weather screen's pages; the first — the watch face's pill too) with the weather the editor had",
         "// at Flash (Open-Meteo); with Wi-Fi the board asks for fresh weather itself (wxfetch.h).",
         "#pragma once", f"#define WXD_N {len(wx['cities'])}", f"#define WXD_F {1 if wx['units'] == 'F' else 0}  // 1 — Fahrenheit",
         "static WxCity wx_city[WX_MAX] = {"]
    for c, d in zip(wx["cities"], data):
        hk = ", ".join(str(k) for k in d.get("hk", []))
        ht = ", ".join(num(v) for v in (d.get("ht", []) + [None] * WX_AHEAD)[:WX_AHEAD])  # (unknown — WX_NONE, not 0°)
        h.append(f"    {{{c_string(c['name'])}, {c['lat']}f, {c['lon']}f, {num(d.get('now'))}, {num(d.get('feels'))}, "
                 f"{num(d.get('hi'))}, {num(d.get('lo'))}, {WX_KINDS[d.get('k', 2)]}, {d.get('t0', 0)}u, {{{hk}}}, {{{ht}}}}},  // {c['where']}")
    h.append("};")
    with open(os.path.join(folder, "wxdata.h"), "w", encoding="utf-8") as f:
        f.write("\n".join(h) + "\n")


# ---------- Wi-Fi: the board's network — kept on this Mac only (secrets/wifi.json: not in git, never in a project) ----------
WIFI_FILE = os.path.join(HERE, "secrets/wifi.json")  # {"current": ssid, "nets": {ssid: password}}


def wifi_read():
    try:
        with open(WIFI_FILE, encoding="utf-8") as f:
            data = json.load(f)
        return {"current": str(data.get("current") or ""),
                "nets": {str(k): str(v) for k, v in dict(data.get("nets") or {}).items()}}
    except (OSError, ValueError):
        return {"current": "", "nets": {}}


def wifi_write(data):
    os.makedirs(os.path.dirname(WIFI_FILE), exist_ok=True)
    tmp = WIFI_FILE + ".tmp"
    with open(os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1, ensure_ascii=False)
    os.replace(tmp, WIFI_FILE)


def shared_ssid():
    """The shared network every sketch of a bigger repository may have (HaloClock.h) — the board's until one is
    picked here; none — ""."""
    return ""


def wifi_pick():
    """(ssid, password) picked in the editor; None — the shared network (HaloClock.h), if any."""
    data = wifi_read()
    ssid = data["current"]
    return (ssid, data["nets"][ssid]) if ssid and ssid in data["nets"] else None


def wifi_info():
    data, pick = wifi_read(), wifi_pick()
    return {"current": pick[0] if pick else shared_ssid(), "saved": sorted(data["nets"])}


# ---------- The board over USB: a command and its answer (the port opened without resetting the board) ----------
board_lock = threading.Lock()


def board_open(port):
    import serial  # pyserial (tools/boardio.py has it too)
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.2
    s.dtr, s.rts = True, False  # so opening the port doesn't reset the board (tools/monitor.py)
    s.exclusive = True  # (a second studio, or anything that locks it too, gets "busy" — not half of the board's answers)
    s.open()
    return s


def board_read(s, seconds, until=None):
    buf, end = b"", time.time() + seconds
    while time.time() < end:
        buf += s.read(4096)
        if until and until in buf:
            break
    return buf.decode("utf-8", "replace")


# ---------- Live (the page: the box in Edit): the layers' numbers onto the board at once, over USB (live.h) ----------
class LiveLink:
    """One connection to the board while Live is on (opening the port again and again can hang the chip). The page sends
    its project as it changes; each layer the board has (its map — "live ?") gets a line when its numbers change: every
    30 ms at most, 6 lines at a time, and once more when the editing stops (a line lost on the way is put right). What
    the board doesn't have as it is (a new layer, another font, a text, a link…) goes to `needs` — the page shows it until
    a Load. Load, the time and Wi-Fi scan take the port for a while (pause); a page silent for 6 s turns Live off."""

    def __init__(self):
        self.cv = threading.Condition()
        self.on, self.port, self.project, self.version = False, "", None, 0
        self.state, self.needs = "off", []  # off | connecting | live | noboard | old (a firmware without Live)
        self.paused, self.holding, self.seen, self.thread = 0, False, 0.0, None

    def status(self):
        with self.cv:
            return {"on": self.on, "state": self.state, "needs": list(self.needs)}

    def set(self, on, port="", project=None):
        with self.cv:
            self.seen = time.time()
            self.port = port or self.port
            if project is not None:
                self.project, self.version = project, self.version + 1
            if on and not self.on:
                self.on, self.state, self.needs = True, "connecting", []
            elif not on:
                self.on, self.state, self.needs = False, "off", []
            if self.on and not self.thread:
                self.thread = threading.Thread(target=self.run, daemon=True)
                self.thread.start()
            self.cv.notify_all()
        return self.status()

    @contextlib.contextmanager
    def pause(self):
        with self.cv:
            self.paused += 1
            self.cv.notify_all()
            while self.holding:  # (until the thread has closed the port)
                self.cv.wait(0.1)
        try:
            yield
        finally:
            with self.cv:
                self.paused -= 1
                self.cv.notify_all()

    def mark(self, state, needs=None):
        with self.cv:
            if self.on:
                self.state = state
                if needs is not None:
                    self.needs = needs

    @staticmethod
    def parse(line):
        """The board's "LIVE 1 <shape> <screen=layer:kind:key,…;…>" — its shape and each screen's layers, in order."""
        parts = line.split(" ", 3)
        if len(parts) < 3 or parts[1] != "1":
            return None
        screens = []
        for chunk in parts[3].split(";") if len(parts) > 3 and parts[3] else []:
            sid, _, rest = chunk.partition("=")
            screens.append((sid, [(a, int(b), c) for a, b, c in (x.split(":") for x in rest.split(",") if x)]))
        return {"shape": parts[2], "screens": screens}

    @staticmethod
    def plan(project, board):
        """For the board's layers: their lines ((screen, layer) -> line: its numbers, or "-" — hide it: gone or hidden
        here) and what only a Load brings (the names: a new layer, one changed beyond its numbers, the order)."""
        screens = clean_screens(project)
        by_id = {scr["id"]: scr for scr in screens}
        lines, needs, have = {}, [], set()
        for si, (sid, blayers) in enumerate(board["screens"]):
            mine = {layer["id"]: layer for layer in by_id[sid]["layers"]} if sid in by_id else {}
            for li, (lid, kind, key) in enumerate(blayers):
                have.add((sid, lid))
                layer = mine.get(lid)
                if layer is None or layer["hidden"]:
                    lines[(si, li)] = LiveLink.line(f"{si} {li} {kind} -")
                elif KINDS.get(layer["type"]) != kind or live_key(layer, si == 0) != key:
                    needs.append(layer["name"])
                else:
                    n = layer_numbers(layer)
                    lines[(si, li)] = LiveLink.line(
                        f"{si} {li} {kind} {n['dx']:.6g} {n['dy']:.6g} {n['k']:.6g} {n['c']:x} {n['c2']:x} {int(n['sp'])} "
                        f"{n['p']:.6g} {n['c3']:x} {n['p2']:.6g} {n['p3']:.6g} 0")
            order = [lid for lid, _, _ in blayers]
            shown = [layer["id"] for layer in by_id[sid]["layers"] if not layer["hidden"]] if sid in by_id else []
            if [lid for lid in shown if lid in order] != [lid for lid in order if lid in shown]:
                needs.append(f"the order on {by_id[sid]['name']}")
        for scr in screens:
            needs += [layer["name"] for layer in scr["layers"] if not layer["hidden"] and (scr["id"], layer["id"]) not in have]
        if live_shape(project, screens) != board["shape"]:
            needs.append("Functions, cities or screens")
        return lines, list(dict.fromkeys(needs))

    @staticmethod
    def line(body):
        """A line for the board's live command: its last word — the sum of the bytes before it (live.h checks it)."""
        return f"live {body} {sum(body.encode()) & 255:02x}"

    def run(self):
        ser, board, buf, sent, queue, burst, again = None, None, b"", {}, {}, {}, {}
        asked, changed, done, tries = 0.0, 0.0, -1, 0
        while True:
            with self.cv:
                if self.on and time.time() - self.seen > 6:  # (the page is gone)
                    self.on, self.state, self.needs = False, "off", []
                stop = not self.on or self.paused
                if stop and ser:
                    ser.close()
                    ser, self.holding = None, False
                    self.cv.notify_all()
                if not self.on:
                    self.thread = None
                    return
                if stop:
                    self.cv.wait(0.2)
                    continue
                port = self.port if self.port and os.path.exists(self.port) else next((p["port"] for p in usb_ports()), "")
                project, version = self.project, self.version
            if ser is None:
                try:
                    if not port:
                        raise OSError("no board")
                    ser = board_open(port)
                    ser.timeout = 0
                except Exception:  # noqa: BLE001 — not connected (or busy): again in a moment
                    self.mark("noboard", [])
                    time.sleep(1.5)
                    continue
                with self.cv:
                    self.holding = True
                board, buf, sent, queue, burst, again, asked, tries = None, b"", {}, {}, {}, {}, 0.0, 0
                said = False  # (it answered hello: ours, or "unknown command" — ours, older)
                self.mark("connecting")
            try:
                buf += ser.read(4096)
                now = time.time()
                while b"\n" in buf:
                    raw, buf = buf.split(b"\n", 1)
                    line = raw.decode("utf-8", "replace").strip()
                    if line.startswith("POPKA "):  # (hello: who it is — for the board's button)
                        boards.heard(port, BoardWatch.parse(line) or "other")
                        said = True
                    elif "unknown command: hello" in line:
                        boards.heard(port, "old")
                        said = True
                    elif line.startswith("LIVE "):
                        board, done, sent, queue = self.parse(line), -1, {}, {}  # (all of it once: the editor moved on)
                    elif "unknown command: live" in line:
                        self.mark("old", [])
                    elif line.endswith(" start") or line == "setup done":  # it started again: ask it again
                        board, asked, tries, said = None, 0.0, 0, False
                buf = buf[-4096:]
                if board is None:
                    if now - asked > 2:
                        ser.write(b"hello\nlive ?\n" if tries == 0 else b"live ?\n")
                        asked, tries = now, tries + 1
                        if tries > 3:
                            self.mark("old", [])
                            if tries == 4 and not said:  # (not a word to hello either: other firmware — once)
                                boards.heard(port, "other")
                else:
                    if version != done and project is not None:
                        lines, needs = self.plan(project, board)
                        done, changed = version, now
                        self.mark("live", needs)
                        queue.update({k: v for k, v in lines.items() if sent.get(k) != v})
                    if not queue and burst and now - changed > 0.4:  # the editing stopped: the last lines once more
                        again, burst = dict(burst), {}
                    for key in list(queue)[:6]:
                        line = queue.pop(key)
                        ser.write(line.encode() + b"\n")
                        sent[key] = burst[key] = line
                    for key in list(again)[:6] if not queue else []:
                        ser.write(again.pop(key).encode() + b"\n")
            except Exception:  # noqa: BLE001 — unplugged: again when it is back
                with self.cv:
                    ser.close()
                    ser, self.holding = None, False
                    self.cv.notify_all()
                self.mark("noboard", [])
                time.sleep(1)
                continue
            with self.cv:
                self.cv.wait(0.03)


live = LiveLink()


def board_command(cmd, until, seconds):
    """Send cmd to the board on USB, read its answer until `until` (bytes) or seconds; LookupError — no board."""
    port = next((p["port"] for p in usb_ports()), "")
    if not port:
        raise LookupError("No board")
    with live.pause(), board_lock:
        s = board_open(port)
        try:
            s.reset_input_buffer()
            s.write(cmd.encode() + b"\n")
            return board_read(s, seconds, until)
        finally:
            s.close()


def wifi_scan():
    """The networks the board sees (HaloClock: wifi scan) and how its last sync went."""
    text = board_command("wifi scan", b"wifi: scan done", 15)
    if "wifi: scan done" not in text:
        raise LookupError("Flash with Wi-Fi on")
    nets, status = {}, None
    for line in text.splitlines():
        m = re.search(r"wifi: net (-?\d+) ([01]) (.+?)\s*$", line)
        if m and (m.group(3) not in nets or nets[m.group(3)]["rssi"] < int(m.group(1))):
            nets[m.group(3)] = {"ssid": m.group(3), "rssi": int(m.group(1)), "lock": m.group(2) == "1"}
        m = re.search(r"wifi: status (\w+) ?(.*?)\s*$", line)
        if m:
            status = {"state": m.group(1), "ssid": m.group(2)}
    return {"networks": sorted(nets.values(), key=lambda n: -n["rssi"]), "status": status, **wifi_info()}


def set_board_time(port, log):
    """After Flash: wait for the board's start, send it the computer's time (the command time)."""
    time.sleep(1)
    for _ in range(40):  # the board restarts: its port comes back
        if os.path.exists(port):
            break
        time.sleep(0.25)
    with board_lock:
        for attempt in range(4):
            try:
                s = board_open(port)
                break
            except Exception:  # noqa: BLE001 — (not back yet)
                if attempt == 3:
                    raise
                time.sleep(0.5)
        try:
            boot = board_read(s, 1.5)
            if " start" in boot and "setup done" not in boot:  # (caught it starting: wait for the end of setup)
                boot += board_read(s, 20, b"setup done")
            s.write(f"time {int(time.time())}\n".encode())
            ok = "time: set" in board_read(s, 1.5, b"time: set")
        finally:
            s.close()
    log("Time: set by this computer" if ok else "Time: the board did not answer")

# The player's music (firmware/music/music.bin: 20 s of four free recordings — music/README.md): written into the
# flash's "ffat" partition by a Load with the player, once — when the board has no tracks (the command music)
MUSIC_BIN, MUSIC_AT = os.path.join(HERE, "firmware/music/music.bin"), 0x910000


def esptool_cmd():
    """esptool: the one in the ESP32 core arduino-cli installed (Mac, Linux), or one on the PATH / in this Python."""
    for name in ("esptool", "esptool.py"):
        if shutil.which(name):
            return [shutil.which(name)]
    for home in ("~/Library/Arduino15", "~/.arduino15"):
        hits = sorted(glob.glob(os.path.join(os.path.expanduser(home), "packages/esp32/tools/esptool_py/*/esptool")))
        if hits:
            return [hits[-1]]
    return [PY, "-m", "esptool"]


def board_ask(port, command, answer, seconds=1.5):
    """After a restart: wait for the board's start, send it a command, give back what it said (until answer)."""
    time.sleep(1)
    for _ in range(40):  # the board restarts: its port comes back
        if os.path.exists(port):
            break
        time.sleep(0.25)
    with board_lock:
        for attempt in range(4):
            try:
                s = board_open(port)
                break
            except Exception:  # noqa: BLE001 — (not back yet)
                if attempt == 3:
                    raise
                time.sleep(0.5)
        try:
            boot = board_read(s, 1.5)
            if " start" in boot and "setup done" not in boot:  # (caught it starting: wait for the end of setup)
                boot += board_read(s, 20, b"setup done")
            s.write((command + "\n").encode())
            return board_read(s, seconds, answer.encode())
        finally:
            s.close()


def board_music(port, log):
    """The player's tracks onto the board — only when it has none (a board with HaloWatch's music keeps it)."""
    m = re.search(r"music: (\d+) tracks", board_ask(port, "music", "tracks"))
    if m and int(m.group(1)) > 0:
        return log(f"Music: {m.group(1)} tracks on the board")
    log("Music: writing the tracks (once, ~1 MB) …")
    r = subprocess.run(esptool_cmd() + ["--chip", "esp32s3", "--port", port, "--baud", "921600", "write_flash",
                                         hex(MUSIC_AT), MUSIC_BIN], capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError((r.stdout + r.stderr).strip().splitlines()[-1] if (r.stdout + r.stderr).strip() else "esptool failed")
    log("Music: written")


def main_index(project, screens):
    """Which screen a swipe up on the face opens (the gallery): its index, -1 — none."""
    ids = [scr["id"] for scr in screens]
    main = project.get("main")
    return ids.index(main) if main in ids[1:] else -1


def c_string(text):
    """A C string literal of a UTF-8 text (bytes past ASCII as octal escapes: a hex one would eat the next letter)."""
    out = ""
    for b in text.encode("utf-8"):
        c = chr(b)
        out += "\\" + c if c in '"\\' else c if 0x20 <= b < 0x7F else f"\\{b:03o}"
    return f'"{out}"'


def font_spec(layer):
    """A layer's font as a halo.conf line without the name — the same line goes to the board and to the page (None —
    the layer has no text: the icon, the dots)."""
    kind = layer["type"]
    if kind not in CHARS:
        return None
    if kind in PILL_FONT:
        size, weight, width = int(PILL_FONT[kind][0] * layer["scale"] + 0.5), PILL_FONT[kind][1], 100
    else:
        size, weight, width = layer["size"], layer["weight"], layer["width"]
    axes = f"wght={weight}" + (f",wdth={width}" if width != 100 else "") + ",ROND=100"
    return f"GoogleSansFlex.ttf  {size}  {CHARS[kind]}  @{axes}"


def font2_spec(layer):
    """A layer's second font (the Steps pill's shoe) as a halo.conf line without the name; None — it has none."""
    if layer["type"] == "infoline" and layer.get("format") == "icons":  # (the shoe in the line)
        return f"fa-solid-900.ttf  {int(layer['size'] * 0.95 + 0.5)}  0x{STEPS_ICON[1]:04x}"
    if layer["type"] != "steps":
        return None
    return f"fa-solid-900.ttf  {int(STEPS_ICON[0] * layer['scale'] + 0.5)}  0x{STEPS_ICON[1]:04x}"


# The weather icons of a layer: which set (the face's pill — gen_icons.face_icon; the Weather screen's big icon and the
# hourly pill's small ones — icon_img and weather_icon_img) and at what size (1 — HaloWatch's); None — no icons
ICON_SETS = {"weather": "face", "wxicon": "big", "hourly": "small"}


def icon_set(layer):
    kind = layer["type"]
    if kind == "forecast":  # (the hourly pill's small ones)
        return ("small", float(layer["scale"]))
    if kind == "infoline":  # (with icons: as tall as its letters)
        return ("small", round(layer["size"] / 22, 2)) if layer.get("format") == "icons" else None
    return (ICON_SETS[kind], float(layer["scale"])) if kind in ICON_SETS else None


def make_icons(iset, scale, folder, prefix):
    """Four weather icons (WEATHER_ICONS) of a set at this size, as HaloWatch makes them (assets/gen_icons.py) → PNG
    paths. The big ones are as tall as the Weather screen's temperature digits (w_now) times the size."""
    paths = []
    with assets_lock, contextlib.redirect_stdout(io.StringIO()):
        saved = gi.FACE_SCALE
        try:
            if iset == "face":
                gi.FACE_SCALE = 1.2 * scale  # gen_icons.FACE_SCALE: HaloWatch's face is 1.2x the mockup
            elif iset == "big":
                target = round(gi.digit_band("w_now") * scale)
            for kind in WEATHER_ICONS:
                path = os.path.join(folder, f"{prefix}_{kind}.png")
                if iset == "face":
                    gi.face_icon(kind, path)
                else:
                    make = gi.icon_img if kind == "partly" else lambda sc, kind=kind: gi.weather_icon_img(kind, sc)
                    if iset == "big":
                        img = gi.fit_height(make, target, gi.BIG_SCALE * scale)
                    else:
                        img = make(gi.SMALL_SCALE * scale)
                    img.save(path)
                paths.append(path)
        finally:
            gi.FACE_SCALE = saved
    return paths


# ---------- Fonts and icons for the page: halo_assets.py's data, packed for the engine (engine/studio_api.h) ----------
class Capture(ha.Out):
    """halo_assets.Out that keeps the arrays as numbers instead of writing C."""

    def __init__(self):
        super().__init__()
        self.arrays = {}

    def arr(self, ctype, name, data, per_line=24):
        self.arrays[name] = data


def run_line(line):
    out = Capture()
    os.makedirs(PREVIEW, exist_ok=True)
    with assets_lock, contextlib.redirect_stdout(io.StringIO()):
        ha.do_line(out, FIRMWARE, line, PREVIEW)
    return out, "\n".join(out.c)


def font_blob(spec):
    """HFNT: size, line, ascent, count, the codes, the glyphs (w, h, ox, oy, adv, offsets), the bits, the ext."""
    out, text = run_line(f"font  f  {spec}")
    size, line_h, ascent, count = map(int, re.search(r"const HaloFont f = \{(-?\d+), (-?\d+), (-?\d+), (\d+),", text).groups())
    codes = [int(v) for v in re.search(r"f_codes\[\] = \{([^}]*)\}", text).group(1).split(",") if v.strip()]
    glyphs = re.findall(r"\{(-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), f_bits \+ (\d+), f_ext \+ (\d+)\}", text)
    assert len(codes) == count == len(glyphs)
    bits, ext = out.arrays["f_bits"], out.arrays["f_ext"]
    b = struct.pack("<4shhhHII", b"HFNT", size, line_h, ascent, count, len(bits), len(ext))
    b += struct.pack(f"<{count}I", *codes)
    for g in glyphs:
        w, h, ox, oy, adv, bo, eo = map(int, g)
        b += struct.pack("<hhhhhhII", w, h, ox, oy, adv, 0, bo, eo)
    return b + bytes(bits) + bytes(ext)


def image_entry(path):
    out, text = run_line(f"image  i  {shlex.quote(path)}")
    w, h = map(int, re.search(r"const HaloImage i = \{(\d+), (\d+),", text).groups())
    px, alpha, ext = out.arrays["i_px"], out.arrays["i_alpha"], out.arrays["i_ext"]
    b = struct.pack("<hhIII", w, h, len(px), len(alpha), len(ext))
    b += struct.pack(f"<{len(px)}H", *px) + bytes(alpha) + bytes(ext)
    return b + bytes(-len(b) % 4)


def icons_blob(iset, scale):
    """HIMS: a set's four icons at this size."""
    folder = os.path.join(BUILD, "icons")
    os.makedirs(folder, exist_ok=True)
    paths = make_icons(iset, scale, folder, f"{iset}{scale:.4f}")
    return struct.pack("<4sI", b"HIMS", len(paths)) + b"".join(image_entry(p) for p in paths)


blob_cache = {}


def cached(key, make):
    if key not in blob_cache:
        blob_cache[key] = make()
    return blob_cache[key]


# ---------- A layer's numbers on the board: what Load writes into layers.h and Live sends (live.h) ----------
C3_KINDS = ("steps", "weekarc", "dotclock", "infoline", "stchart")  # those that have a third color (FaceLayer.color3)


def layer_numbers(layer):
    """FaceLayer's numbers (face.h) for a layer: moved by dx, dy; the size k; color, color2, the letter spacing; p (the
    goal, the stars' count, a radius, a picture's turn, the date frame's gap); color3; p2, p3 — what each kind keeps there."""
    kind = layer["type"]
    p2 = p3 = 0
    if kind == "steps":  # (p2: 1 — no pill behind)
        p2 = 0 if layer.get("pill", True) else 1
    elif kind == "curve":  # what it says, where (degrees from 12 o'clock); the radius goes in p
        p2, p3 = CURVES.index(layer["what"]), layer["angle"]
    elif kind == "stepring":  # the radius; the goal goes in p
        p2 = layer["radius"]
    elif kind in ("weekarc", "forecast"):  # where, as a curved line's
        p3 = layer.get("angle", 0)
    elif kind == "infoline":  # the format
        p2 = INFOS.index(layer["format"])
    elif kind == "stars":  # twinkling, meteors a minute (how many — p, brightness — k)
        p2, p3 = layer["twinkle"], layer["meteors"]
    elif kind in PIXEL_ART:  # what is around it, its trail (the turn — p, the size — k)
        p2, p3 = SPARKLES[layer["sparkles"]], layer.get("rainbow", 1)
    elif kind in ("devcard", "device"):  # About: what it shows
        p2 = DEVS.index(layer["what"])
    color = lambda key: int(layer.get(key, "#000000")[1:], 16)
    p = next((layer[key] for key in ("goal", "count", "radius", "angle", "gap") if key in layer), 0)
    return {"dx": layer["dx"], "dy": layer["dy"], "k": layer.get("scale", 1), "c": color("color"), "c2": color("color2"),
            "sp": layer.get("spacing", 0), "p": p, "c3": color("color3") if kind in C3_KINDS else 0, "p2": p2, "p3": p3}


def live_hash(parts, digits):
    return f"{zlib.crc32(json.dumps(parts, default=str, sort_keys=True).encode()) & ((1 << 4 * digits) - 1):0{digits}x}"


def live_key(layer, home):
    """What only a Load brings for a layer (Live sends just its numbers): its kind, fonts, icons, texts, its link, whether
    it counts steps — 4 hex digits; the firmware keeps it (LIVE_MAP) and Live compares."""
    kind = layer["type"]
    tap = layer["tap"] if home or kind in TAPS else None
    return live_hash([kind, font_spec(layer), font2_spec(layer), icon_set(layer), layer.get("text"), layer.get("subtitle"),
                      layer.get("icon"), tap, uses_steps(layer)], 4)


def live_shape(project, screens):
    """What only a Load brings for the whole watch: the screens and their order, the gallery, the Functions, the clock,
    the cities — 8 hex digits (LIVE_SHAPE)."""
    return live_hash([[scr["id"] for scr in screens], main_index(project, screens), clean_functions(project),
                      clean_time(project), clean_weather(project)], 8)


# ---------- The board sketch from a project ----------
def cfloat(v):
    """A float literal that is exactly this float32 (9 digits are enough to come back to the same float)."""
    s = format(float(np.float32(v)), ".9g")
    if not re.search(r"[.e]", s):
        s += ".0"
    return s + "f"


def write_sketch(project, folder, meta=None):
    """firmware/PopkaWatch with the project's screens into folder: layers.h, assets/halo.conf, icons, src/. meta — the
    project's id and revision (a Load): into config.h, for the board's hello (hello.h)."""
    screens = clean_screens(project)
    for scr in screens:
        scr["layers"] = [layer for layer in scr["layers"] if not layer["hidden"]]
    index = {scr["id"]: i for i, scr in enumerate(screens)}
    main = main_index(project, screens)
    os.makedirs(os.path.join(folder, "assets"), exist_ok=True)
    if os.path.abspath(folder) != os.path.abspath(FIRMWARE):
        for name in ("PopkaWatch.ino", "face.h", "weather.h", "wxscreen.h", "gallery.h", "power.h", "steps.h", "stars.h",
                     "pixelart.h", "parrot.h", "bt.h", "wxfetch.h", "settings.h", "stepscreen.h", "widgets.h",
                     "music.h", "player.h", "sound.h", "volume.h", "recorder.h", "snake.h", "games.h", "game_dots.h",
                     "game_space.h", "game_iso.h", "live.h", "device.h", "hello.h", "wifiscan.h"):
            shutil.copy(os.path.join(FIRMWARE, name), os.path.join(folder, name))
    fonts, icons = {}, {}  # spec -> name, (set, scale) -> name
    all_layers = [layer for scr in screens for layer in scr["layers"]]
    for layer in all_layers:
        if font_spec(layer):
            fonts.setdefault(font_spec(layer), f"font{len(fonts)}")
        if font2_spec(layer):
            fonts.setdefault(font2_spec(layer), f"font{len(fonts)}")
        if icon_set(layer):
            icons.setdefault(icon_set(layer), f"icons{len(icons)}")
    conf = ["# Generated by PopkaStudio (studio.py) from the editor's project — do not edit by hand.",
            "# The fonts and icons of the screens' layers (face.h, wxscreen.h), at the sizes picked in the editor; the",
            "# gallery's (gallery.h) — HaloWatch's, always."]
    for spec, name in fonts.items():
        used = ", ".join(sorted({layer["name"] for layer in all_layers if spec in (font_spec(layer), font2_spec(layer))}))
        conf.append(f"font  {name}  {spec}   # {used}")
    conf += GALLERY_FONTS
    for kinds, lines in SETTINGS_FONTS.items():
        kinds = kinds if isinstance(kinds, tuple) else (kinds,)
        if os.path.abspath(folder) == os.path.abspath(FIRMWARE) or any(layer["type"] in kinds for layer in all_layers):
            conf += lines
    for old in glob.glob(os.path.join(folder, "assets/icons*_*.png")):
        os.remove(old)
    for (iset, scale), name in icons.items():
        for path in make_icons(iset, scale, os.path.join(folder, "assets"), name):
            conf.append(f"image {os.path.splitext(os.path.basename(path))[0]}  assets/{os.path.basename(path)}")
    with open(os.path.join(folder, "assets/halo.conf"), "w", encoding="utf-8") as f:
        f.write("\n".join(conf) + "\n")

    h = ["// Generated by PopkaStudio (studio.py) from the editor's project — do not edit by hand.",
         "// The screens (0 — the watch face, the home) and their layers, bottom to top (FaceLayer in face.h): kind,",
         "// moved from HaloWatch's place (dx, dy), the size k, colors, letter spacing, font, icons, a tap opens",
         "// screen (-1 — no link) with the transition, the radius of the clock along the top edge; the gallery's text.",
         "#pragma once", "", f"#define SCREEN_N {len(screens)}",
         f"#define MAIN_SCREEN {main} // the gallery: a swipe up on the face (-1 — none)"]
    fn = clean_functions(project)
    stscreen = any(layer["type"] in STEPS_SCREEN for layer in all_layers)
    steps = any(uses_steps(layer) for layer in all_layers)  # (the pedometer runs)
    act = lambda f: 0 if not f["on"] else {"off": 1, "lock": 2, "aod": 3}[f["act"]]
    h += ["// The Functions (power.h): Always On when idle, after idle s, the panel in it (of 255); BOOT and PWR: 0 — nothing,",
          "// 1 — the screen off, 2 — lock (Always On, the touch off; a long press of that button unlocks), 3 — Always On",
          f"#define FN_AOD {1 if fn['aod']['on'] else 0}", f"#define FN_AOD_IDLE {fn['aod']['idle']}",
          f"#define FN_AOD_BRIGHT {fn['aod']['bright']}",
          f"#define FN_AOD_HOME {1 if fn['aod']['home'] else 0} // out of Always On on the watch face (1) or where it was",
          f"#define FN_AOD_SHIFT {1 if fn['aod']['shift'] else 0} // Always On: a few pixels elsewhere every minute (burn-in)",
          f"#define FN_BOOT {act(fn['boot'])}",
          f"#define FN_PWR {act(fn['pwr'])}",
          "// Waking up from Always On or dark (power.h): a touch, picking it up (off, or the Steps layer has the motion",
          "// sensor — not at all), a button; a way that is off does not wake the screen",
          f"#define FN_WAKE_TAP {1 if fn['aod']['tap'] else 0}",
          f"#define FN_WAKE_RAISE {1 if fn['aod']['raise'] and not steps else 0}",
          f"#define FN_WAKE_BTN {1 if fn['aod']['btn'] or not (fn['aod']['tap'] or (fn['aod']['raise'] and not steps)) else 0}",
          "// HaloWatch's Settings sections (settings.h): only with their layer",
          f"#define FN_DISPLAY {1 if any(layer['type'] == 'display' for layer in all_layers) else 0}",
          f"#define FN_BATTERY {1 if any(layer['type'] == 'battery' for layer in all_layers) else 0}",
          f"#define FN_MUSIC {1 if any(layer['type'] == 'music' for layer in all_layers) else 0}",
          f"#define FN_RECORDER {1 if any(layer['type'] == 'recorder' for layer in all_layers) else 0}",
          "// About and Power (device.h): only with their layers (About: the microphones and the motion sensor only while",
          "// something on screen shows them)",
          f"#define FN_ABOUT {1 if any(layer['type'] in ('devcard', 'device') for layer in all_layers) else 0}",
          f"#define FN_CHARGE {1 if any(layer['type'] == 'charge' for layer in all_layers) else 0}",
          "// The Games (games.h): only the snakes added",
          *[f"#define FN_{k.upper()} {1 if any(layer['type'] == k for layer in all_layers) else 0}"
            for k in ("snkdots", "snkspace", "snkiso")],
          "// Steps (steps.h): only with a Steps layer or the Steps screen — then the motion chip's pedometer counts;",
          "// the screen (stepscreen.h) — only with it (and the hours and the days kept in flash)",
          f"#define FN_STEPS {1 if steps else 0}",
          f"#define FN_STEPSCR {1 if stscreen else 0}",
          "// Stars (stars.h): only with a Stars layer",
          f"#define FN_STARS {1 if any(layer['type'] == 'stars' for layer in all_layers) else 0}",
          "// Pixel art (pixelart.h): only the pictures added",
          f"#define FN_PARROT {1 if any(layer['type'] == 'parrot' for layer in all_layers) else 0}",
          ]
    write_wxdata(project, folder)
    tm = clean_time(project)
    cfg = ["// Generated by PopkaStudio (studio.py) from the editor's project — do not edit by hand.",
           "// The Functions that decide what is built at all (included first): Wi-Fi (HaloClock: the time from the internet,",
           "// the networks the board sees), Bluetooth (bt.h); the time zone and the clock's 12 or 24 hours.",
           "#pragma once", f"#define FN_WIFI {1 if fn['wifi']['on'] else 0}", f"#define FN_BT {1 if fn['bt']['on'] else 0}",
           f"#define TZ {c_string(ZONES[tm['zone']])}  // {tm['zone']}", f"#define CLOCK_24H {1 if tm['h24'] else 0}"]
    if meta is not None:  # (a Load: who this firmware is — the board's answer to hello, hello.h)
        pid = str(meta.get("pid") or "")
        cfg += ["// Who this firmware is: the board's answer to hello (hello.h) — the editor tells its own boards",
                f"#define POPKA_BOARD {c_string(project.get('board') or 'amoled175')}",
                f"#define POPKA_VERSION {c_string(VERSION)}",
                f"#define POPKA_PROJECT {c_string(pid if PROJECT_ID.fullmatch(pid) else '-')}",
                f"#define POPKA_REV {int(meta.get('rev') or 0)}",
                f"#define POPKA_LOADED {c_string(time.strftime('%Y-%m-%dT%H:%M'))}"]
    with open(os.path.join(folder, "config.h"), "w", encoding="utf-8") as f:
        f.write("\n".join(cfg) + "\n")
    # The network picked in the editor: only into the build copy (never firmware/PopkaWatch: it is in git)
    net, pick = os.path.join(folder, "wifi_net.h"), wifi_pick()
    if os.path.abspath(folder) != os.path.abspath(FIRMWARE) and fn["wifi"]["on"] and pick:
        with open(os.open(net, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), "w", encoding="utf-8") as f:
            f.write("// The board's Wi-Fi, picked in PopkaStudio (secrets/wifi.json) — written for this build only.\n"
                    "#pragma once\n"
                    f"#define PW_WIFI_SSID {c_string(pick[0])}\n#define PW_WIFI_PASS {c_string(pick[1])}\n")
    elif os.path.exists(net):
        os.remove(net)
    for (iset, scale), name in icons.items():
        h.append(f"static const HaloImage *const {name}[4] = {{" +
                 ", ".join(f"&{name}_{k}" for k in WEATHER_ICONS) + "};")
    # Live (live.h): what this firmware has — for the page to send each layer's numbers to its place and to tell what
    # only a Load brings: the shape; each screen's id and its layers' ids, kinds and keys (live_key)
    live_map = ";".join(f"{scr['id']}=" + ",".join(f"{layer['id']}:{KINDS[layer['type']]}:{live_key(layer, i == 0)}"
                                                   for layer in scr["layers"]) for i, scr in enumerate(screens))
    h += ["// Live (live.h): what this firmware has, for the editor's Live to check its project against",
          f"#define LIVE_SHAPE {c_string(live_shape(project, screens))}", f"#define LIVE_MAP {c_string(live_map)}"]
    for i, scr in enumerate(screens):
        h += ["", f"// {scr['name']}" + (" — the home" if i == 0 else ""), f"static FaceLayer screen{i}_layers[] = {{"]
        for layer in scr["layers"]:
            kind = layer["type"]
            n = layer_numbers(layer)
            font = f"&{fonts[font_spec(layer)]}" if font_spec(layer) else "nullptr"
            ic = icons[icon_set(layer)] if icon_set(layer) else "nullptr"
            # Links: from the watch face (halo_nav_face_open) and from the gallery's cards and button (halo_nav_open);
            # neither opens the face, the gallery or (a card) its own screen
            tap = layer["tap"] if i == 0 or kind in TAPS else None
            to = index.get(tap["to"], -1) if tap else -1
            if to in (0, main, i if i else -1):
                to = -1
            fx = FX[tap["fx"]] if tap and to > 0 and tap["fx"] in FX else "HALO_FX_FADE"
            if kind in GALLERY:  # its text: the title, the line under it, the icon
                icon = chr(ICONS[layer["icon"]]) if "icon" in layer else ""
                text = ", " + ", ".join(c_string(v) if v is not None else "nullptr"
                                        for v in (layer.get("text"), layer.get("subtitle"), icon or None))
                if kind == "devcard":  # (and what it shows: p2)
                    text += f", 0x000000, nullptr, {cfloat(n['p2'])}, {cfloat(n['p3'])}"
            else:  # no text; the third color, the second font (the Steps pill's shoe, the info line's), p2, p3
                f2 = f"&{fonts[font2_spec(layer)]}" if font2_spec(layer) else "nullptr"
                text = f", nullptr, nullptr, nullptr, 0x{n['c3']:06X}, {f2}, {cfloat(n['p2'])}, {cfloat(n['p3'])}"
            h.append(f"    {{FL_{kind.upper()}, {cfloat(n['dx'])}, {cfloat(n['dy'])}, {cfloat(n['k'])}, "
                     f"0x{n['c']:06X}, 0x{n['c2']:06X}, {n['sp']}, {font}, {ic}, {to if to > 0 else -1}, {fx}, "
                     f"{cfloat(n['p'])}, false{text}}},  // {layer['name']}")
        if not scr["layers"]:
            h.append("    {},  // (empty: the array needs one)")
        h += ["};", f"static FaceRun (&screen{i}_run)[{max(1, len(scr['layers']))}] = face_runs<{max(1, len(scr['layers']))}>();"]
    h += ["", "static FaceScreen screens[SCREEN_N] = {"]
    # the name: shown by a screen without layers (as HaloWatch's stub of a card) — in the gallery's font: Latin only
    h += [f"    {{screen{i}_layers, screen{i}_run, {len(scr['layers'])}, "
          f"{c_string(''.join(c for c in scr['name'] if ' ' <= c <= '~')[:39])}}},  // {scr['name']}" for i, scr in enumerate(screens)]
    h.append("};")
    with open(os.path.join(folder, "layers.h"), "w", encoding="utf-8") as f:
        f.write("\n".join(h) + "\n")
    r = subprocess.run([PY, os.path.join(ROOT, "tools/halo_assets.py"), folder], capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError("halo_assets.py: " + (r.stderr or r.stdout)[-2000:])
    return r.stdout


# ---------- The page's engine: firmware/PopkaWatch built for the browser ----------
def engine_sources():
    files = glob.glob(os.path.join(ROOT, "lib/Halo/src/*")) + glob.glob(os.path.join(HERE, "engine/**/*"), recursive=True)
    files += glob.glob(os.path.join(FIRMWARE, "**/*"), recursive=True)
    return [f for f in files if os.path.isfile(f)]


def build_engine(force=False):
    out = os.path.join(ENGINE_OUT, "PopkaWatch.wasm")
    if not force and os.path.exists(out):
        newest = max(os.path.getmtime(f) for f in engine_sources())
        if os.path.getmtime(out) >= newest:
            return
        if not shutil.which("em++"):  # (no Emscripten: the engine that came with PopkaStudio — only a developer rebuilds it)
            return
    if not shutil.which("em++"):
        sys.exit(f"The page's engine is missing ({os.path.relpath(out, ROOT)}) and Emscripten (em++) is not installed "
                 "to build it: get PopkaStudio again (git pull / install.sh), or install Emscripten.")
    print("building the page's engine (engine/build.sh) …", flush=True)
    subprocess.run([os.path.join(HERE, "engine/build.sh")], check=True)


# ---------- Boards ----------
# photo — its picture in the start window (web/img: the author's own photo, cut out, no metadata); drawn — without a
# photo ("round": the round board, start.js; none — a screen); soon — shown there, not chosen yet
BOARDS = [{"id": "amoled175", "name": "Waveshare ESP32-S3-Touch-AMOLED-1.75C", "short": "AMOLED 1.75",
           "maker": "Waveshare", "title": "ESP32-S3 AMOLED 1.75", "screen": "466 × 466, round",
           "spec": "1.75″ round · 466 × 466",
           "photo": "img/board-amoled175.webp", "drawn": "round", "usb": ["303A:1001"],
           "chip": "ESP32-S3", "psram": 8, "flash": 32},  # (what esptool reads on it: the board's check)
          {"id": "p4lcd7", "name": "Waveshare ESP32-P4-WIFI6-Touch-LCD-7B", "short": "P4 LCD 7″",
           "maker": "Waveshare", "title": "ESP32-P4 LCD 7″", "screen": "1024 × 600", "spec": "7″ · 1024 × 600",
           "soon": True, "usb": ["1A86:55D3"], "chip": "ESP32-P4", "psram": 32, "flash": 32}]


def usb_ports():
    """The boards on USB: the ESP32-S3's own USB has Espressif's vendor id (Mac: /dev/cu.usbmodem…, Linux: /dev/ttyACM…);
    its serial number is the chip's MAC — each board's own (BoardWatch remembers boards by it)."""
    ports = []
    try:
        from serial.tools import list_ports
        ports = sorted(({"port": p.device, "name": os.path.basename(p.device), "usb": f"{p.vid:04X}:{p.pid:04X}",
                         "mac": p.serial_number or ""} for p in list_ports.comports()
                        if p.vid == 0x303A and not p.device.startswith("/dev/tty.")), key=lambda q: q["port"])
    except Exception:  # noqa: BLE001 — no pyserial or no USB info: the usual names
        pass
    if not ports:
        ports = [{"port": p, "name": os.path.basename(p), "usb": "", "mac": ""}
                 for p in sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*"))]
    return ports


# ---------- Who is on USB (docs/board-detect.md, step 1): each board that appears is asked "hello" once ----------
class BoardWatch:
    """Each USB port that appears is asked once who it is, with one opening of the port (no reset: DTR=1, RTS=0; not
    again and again — that can hang the chip): our firmware answers "POPKA 1 board=… fw=… project=… rev=… loaded=…"
    (hello.h) — "ours"; "unknown command: hello" — ours, made before hello — "old"; silence — "other" firmware (or none).
    Asked again after a Load; a port another program holds — "busy", tried again in a while. While Live holds the port,
    LiveLink asks (and tells this). Boards are remembered by their MAC in projects/.boards.json: the model and what was
    on it — a board seen once is known at once the next time."""

    def __init__(self):
        self.lock = threading.Lock()
        self.ports, self.again, self.retry = {}, set(), {}  # port -> its check; ports to ask again; port -> when (busy)
        self.quiet = {}  # port -> how many times a board known as ours said nothing (asked again before "other")
        self.memory, self.thread = None, None

    def path(self):
        return os.path.join(PROJECTS, ".boards.json")

    def remembered(self):
        if self.memory is None:
            try:
                with open(self.path(), encoding="utf-8") as f:
                    self.memory = json.load(f)
            except (OSError, ValueError):
                self.memory = {}
        return self.memory

    def remember(self, mac, check):
        if not mac:
            return
        mem = self.remembered()
        was = mem.get(mac, {})
        since = was.get("since", 0) if was.get("state") == check["state"] else int(time.time())  # (when it became so)
        mem[mac] = {**was, "state": check["state"], "seen": int(time.time()), "since": since,
                    **{k: check[k] for k in ("board", "fw", "project", "rev", "loaded") if check.get(k) not in (None, "")}}
        try:
            os.makedirs(PROJECTS, exist_ok=True)
            with open(self.path(), "w", encoding="utf-8") as f:
                json.dump(mem, f, indent=1)
        except OSError:
            pass

    @staticmethod
    def parse(line):
        """ "POPKA 1 board=amoled175 fw=1.1.1 project=project-1 rev=57 loaded=… live=1" -> the check, None — not it."""
        parts = line.split()
        if len(parts) < 2 or parts[0] != "POPKA" or parts[1] != "1":
            return None
        kv = dict(x.split("=", 1) for x in parts[2:] if "=" in x)
        rev = kv.get("rev", "0")
        return {"state": "ours", "board": kv.get("board", ""), "fw": kv.get("fw", ""),
                "project": "" if kv.get("project", "-") == "-" else kv["project"],
                "rev": int(rev) if rev.isdigit() else 0, "loaded": "" if kv.get("loaded", "-") == "-" else kv["loaded"],
                "live": kv.get("live") == "1"}

    def heard(self, port, check):
        """What the board said (here or through LiveLink): a check, or the state alone ("old", "other", "busy")."""
        check = check if isinstance(check, dict) else {"state": check}
        mac = next((p["mac"] for p in usb_ports() if p["port"] == port), "")
        with self.lock:
            self.ports[port] = check
            self.again.discard(port)
            if check["state"] == "busy":
                self.retry[port] = time.time() + 5
        if check["state"] != "busy":
            self.remember(mac, check)

    def remember_check(self, mac, info):
        """The check's findings (board_check) next to what the board said: the next time — known without asking."""
        if not mac:
            return
        mem = self.remembered()
        mem.setdefault(mac, {})["check"] = {k: info[k] for k in ("chip", "rev", "psram", "flash", "firmware", "empty", "fits", "copy", "at")}
        try:
            with open(self.path(), "w", encoding="utf-8") as f:
                json.dump(mem, f, indent=1)
        except OSError:
            pass

    def ask_again(self, port):
        with self.lock:
            self.again.add(port)

    def ask(self, port):
        with self.lock:
            self.ports[port] = {"state": "checking"}
        try:
            ser = board_open(port)
        except Exception:  # noqa: BLE001 — another program has it (a monitor, an upload): later
            return self.heard(port, "busy")
        try:
            ser.timeout = 0
            board_read(ser, 0.15)  # (what it was saying before: its log)
            buf, tries, said = "", 0, None
            while tries < 3 and said is None:
                ser.write(b"hello\n")
                tries += 1
                end = time.time() + 1.2
                while time.time() < end and said is None:
                    buf += ser.read(4096).decode("utf-8", "replace")
                    *lines, buf = buf.split("\n")
                    for line in (x.strip() for x in lines):
                        if line.startswith("POPKA "):
                            said = self.parse(line) or said
                        elif "unknown command: hello" in line:
                            said = "old"
                        elif line == "setup done" or line.endswith(" start"):
                            tries = min(tries, 2)  # (it just started: one more question)
                    time.sleep(0.02)
            if said is None:  # (a board ours last time, silent now: something else read its answer, or it was busy —
                mac = next((p["mac"] for p in usb_ports() if p["port"] == port), "")  # asked twice more, 5 s apart)
                if self.remembered().get(mac, {}).get("state") in ("ours", "old") and self.quiet.get(port, 0) < 2:
                    self.quiet[port] = self.quiet.get(port, 0) + 1
                    return self.heard(port, "busy")
            self.quiet.pop(port, None)
            self.heard(port, said or "other")
        finally:
            ser.close()

    def run(self):
        while True:
            try:
                ports = {p["port"] for p in usb_ports()}
                now = time.time()
                with self.lock:
                    for gone in set(self.ports) - ports:
                        self.ports.pop(gone, None), self.again.discard(gone), self.retry.pop(gone, None)
                    todo = [port for port in sorted(ports) if port in self.again or port not in self.ports
                            or (self.ports[port]["state"] == "busy" and now > self.retry.get(port, 0))]
                for port in todo:
                    with job_lock:
                        flashing = job["state"] == "running"
                    if flashing or live.on or port_user["what"]:  # (a Load, the check or a copy holds the port;
                        continue                                    # Live asks the board itself)
                    self.ask(port)
            except Exception:  # noqa: BLE001 — never let it die: a port gone in the middle of a question
                pass
            time.sleep(1.5)

    def status(self):
        """For /api/boards: each port — its MAC, its check and the model to pick: what the board said, else what it was
        last time, else the one board its USB ids fit (if only one)."""
        if self.thread is None:
            self.thread = threading.Thread(target=self.run, daemon=True)
            self.thread.start()
        out, mem = [], self.remembered()
        for p in usb_ports():
            with self.lock:
                check = dict(self.ports.get(p["port"]) or {"state": "checking"})
            was = mem.get(p["mac"], {}) if p["mac"] else {}
            fits = [b["id"] for b in BOARDS if p["usb"] in b.get("usb", [])]
            model = check.get("board") or was.get("board") or (fits[0] if len(fits) == 1 else "")
            out.append({**p, **check, "model": model, "known": bool(was), "check": was.get("check"),
                        "since": was.get("since", 0)})  # (since when it is what it is: the page's offer — once a change)
        return out


boards = BoardWatch()


# ---------- The board's check (docs/board-detect.md, step 2): esptool reads, writes nothing ----------
# The board restarts into its loader for a few seconds (esptool does it over USB), what is read: the chip, its memory
# and flash (flash-id), the partition table, which app boots (otadata) and its description (esp_app_desc_t) — then a
# restart, and its own firmware runs again. The reads go one after another in the loader (--before / --after no-reset),
# the last one always --before default-reset: after "no-reset" the S3's USB keeps DTR held, and its hard reset (RTS)
# starts the chip in its loader again — it stayed there, dark (found on the board, 6 Nov 2026).
# What it tells: the chip, PSRAM, flash, which of our boards it fits, what kind of firmware (an
# Arduino sketch says "arduino-lib-builder": the core's, not the sketch's — ours is told by its hello), or none.
# The read isn't fast over the S3's own USB (at 460800 ~40 KB/s; at the default 115200 ~11 KB/s — measured 6 Nov 2026):
# a copy of a firmware takes only what it needs — the loader, the partition table, its data partitions (settings, its
# pictures and files: without them it wouldn't come back whole — Waveshare's factory firmware keeps its pictures in a
# 9 MB "assets") and the app that boots, by its own size — not the empty rest of the flash.
COPY_RATE = 40_000  # bytes a second a copy reads (for its estimate)


def copy_regions(parts, app, size):
    """What a copy of the firmware reads: [(file, offset, bytes)] — the loader, the partition table, each data partition
    but the core dump (nvs, otadata, phy, its file systems), the app that boots (its own size)."""
    out = [("bootloader.bin", 0, 0x8000), ("partitions.bin", 0x8000, 0xC00)]
    out += [(f"{p['name']}.bin", p["offset"], p["size"]) for p in parts if p["type"] == 1 and p["sub"] != 3]
    out += [("app.bin", app["offset"], size)] if app and size else []
    return out
port_user = {"what": ""}  # (the check or a copy has the port: BoardWatch waits)
BACKUPS = os.path.join(HERE, "backups")
BOOT_TROUBLE = ("Failed to connect", "No serial data received", "Wrong boot mode", "Could not connect to an Espressif",
                "Invalid head of packet", "Timed out waiting for packet", "Download mode")


def boot_trouble(text):
    """The board didn't go into its loader (other firmware took the USB, a boot loop, asleep): hold BOOT, plug it in."""
    return any(k in text for k in BOOT_TROUBLE)


def esptool_run(port, args, progress=None):
    """One esptool run on the board's port -> its output; progress(percent) for a read; failed — RuntimeError."""
    proc = subprocess.Popen(esptool_cmd() + ["-p", port, "-b", "460800"] + args, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, bufsize=1)  # (460800: reads ~40 KB/s, the default
                                                                             # 115200 ~11; 921600 broke off)
    lines = []
    for line in proc.stdout:  # (its progress comes with \r: a line each, in text mode)
        lines.append(line.rstrip())
        m = re.search(r"([\d.]+)%", line)
        if progress and m and line.lstrip().startswith("Reading"):
            progress(float(m.group(1)))
    text = "\n".join(lines)
    if proc.wait():
        err = next((x for x in reversed(lines) if x.strip()), "esptool failed")
        raise RuntimeError(("BOOT: " if boot_trouble(text) else "") + err.replace("A fatal error occurred: ", ""))
    return text


def parse_parts(buf):
    """The partition table (32-byte entries, magic AA 50) -> [{name, type, sub, offset, size}]."""
    out = []
    for i in range(0, min(len(buf), 0xC00), 32):
        e = buf[i:i + 32]
        if e[:2] != b"\xaa\x50":
            break
        off, size = struct.unpack("<II", e[4:12])
        out.append({"name": e[12:28].split(b"\0")[0].decode("latin1"), "type": e[2], "sub": e[3], "offset": off, "size": size})
    return out


def boot_app(parts, otadata):
    """The app partition that boots: by otadata (the newest valid sequence), else factory, else the first one."""
    apps = [p for p in parts if p["type"] == 0]
    otas = [p for p in apps if 0x10 <= p["sub"] < 0x20]
    seqs = [struct.unpack("<I", otadata[k:k + 4])[0] for k in (0, 0x1000) if len(otadata) >= k + 4]
    seqs = [q for q in seqs if q not in (0, 0xFFFFFFFF)]
    if otas and seqs:
        return otas[(max(seqs) - 1) % len(otas)]
    return next((p for p in apps if p["sub"] == 0), apps[0] if apps else None)


def app_desc(head):
    """The app image's head (0x100 bytes): magic E9, then esp_app_desc_t at 0x20 -> its words, or None — no app."""
    if len(head) < 0x100 or head[0] != 0xE9:
        return None
    d = head[0x20:0x20 + 0xB0]
    text = lambda b: b.split(b"\0")[0].decode("latin1").strip()
    if struct.unpack("<I", d[:4])[0] != 0xABCD5432:
        return {"project": "", "version": "", "date": "", "idf": ""}
    return {"version": text(d[16:48]), "project": text(d[48:80]), "date": f"{text(d[96:112])} {text(d[80:96])}".strip(),
            "idf": text(d[112:144])}


def board_check(port):
    """What the board is — board_check_once, once more if it broke off between its reads (another program opened the
    port in a gap between two esptool runs and spoke into the loader: "the chip stopped responding" — seen with a
    second studio running, 6 Nov 2026); not again when the board can't reach its loader (BOOT: that won't change)."""
    for attempt in (1, 2):
        try:
            return board_check_once(port)
        except RuntimeError as e:
            if attempt == 2 or str(e).startswith("BOOT: "):
                raise
            time.sleep(1)


def board_check_once(port):
    """What the board is (esptool, nothing written; it restarts): the chip, PSRAM, flash, MAC, the partitions, the
    firmware that boots and which of our boards it fits."""
    tmp = tempfile.mkdtemp(prefix="popka-check-")
    port_user["what"] = "check"
    loader = False  # (the board is in its loader now: a restart is owed)
    try:
        with live.pause():
            text = esptool_run(port, ["--before", "default-reset", "--after", "no-reset", "flash-id"])
            loader = True
            grab = lambda rx: (re.search(rx, text) or [None, ""])[1].strip()
            chip = grab(r"Chip type:\s*(.+)")
            info = {"chip": re.sub(r"\s*\(.*", "", chip), "package": grab(r"Chip type:.*?\((\w+)\)"),
                    "rev": grab(r"revision (v[\d.]+)"), "features": grab(r"Features:\s*(.+)"),
                    "psram": int(grab(r"PSRAM (\d+)MB") or 0), "flash": int(grab(r"Detected flash size:\s*(\d+)MB") or 0),
                    "usb": grab(r"USB mode:\s*(.+)"), "mac": grab(r"MAC:\s*([0-9a-f:]+)").upper()}
            head = os.path.join(tmp, "head.bin")  # the partition table, otadata (0xe000) and app0's head (0x10000)
            esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", "0x8000", "0x8100", head])
            buf = open(head, "rb").read()
            parts = parse_parts(buf)
            ota = next((p for p in parts if p["type"] == 1 and p["sub"] == 0), None)
            otadata = b""
            if ota and ota["offset"] + 0x1004 <= 0x10100:
                otadata = buf[ota["offset"] - 0x8000:ota["offset"] - 0x8000 + 0x1004]
            elif ota:  # (further on: read it — which app boots)
                more = os.path.join(tmp, "ota.bin")
                esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", hex(ota["offset"]), "0x1004", more])
                otadata = open(more, "rb").read()
            app = boot_app(parts, otadata)
            desc, size = None, 0
            if app and 0x8000 <= app["offset"] <= 0x10000:
                desc = app_desc(buf[app["offset"] - 0x8000:app["offset"] - 0x8000 + 0x100])
            elif app:  # (it boots from further on: its head read too)
                more = os.path.join(tmp, "app.bin")
                esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", hex(app["offset"]), "0x100", more])
                desc = app_desc(open(more, "rb").read())
            if desc is not None:
                size = image_size(port, app["offset"])  # (for a copy's estimate)
            esptool_run(port, ["--before", "default-reset", "--after", "hard-reset", "read-mac"])  # (its firmware again)
            loader = False
    except RuntimeError:
        if loader:  # (left in its loader: back to its firmware)
            try:
                esptool_run(port, ["--before", "default-reset", "--after", "hard-reset", "read-mac"])
            except RuntimeError:
                pass
        raise
    finally:
        port_user["what"] = ""
        boards.ask_again(port)  # (it restarted: its hello again)
        shutil.rmtree(tmp, ignore_errors=True)
    family = ("" if desc is None else "Arduino" if desc["project"] == "arduino-lib-builder" else "ESP-IDF" if desc["idf"] else "")
    info.update({"parts": [{**p, "offset": hex(p["offset"]), "size": p["size"]} for p in parts],
                 "app": None if app is None else {"name": app["name"], "offset": hex(app["offset"]), "size": app["size"]},
                 "firmware": None if desc is None else {**desc, "family": family},
                 "empty": not parts or desc is None,
                 "copy": None if desc is None else (lambda n: {"bytes": n, "seconds": round(n / COPY_RATE)})(
                     sum(r[2] for r in copy_regions(parts, app, size))),
                 "fits": [b["id"] for b in BOARDS if b.get("chip") == info["chip"] and b.get("psram") == info["psram"]
                          and b.get("flash") == info["flash"]],
                 "at": int(time.time())})
    boards.remember_check(info["mac"], info)
    return info


def image_size(port, offset):
    """An app image's own size (the loader is open, --before/--after no-reset): its head, then each segment's head —
    a small read each — then the checksum's 16-byte padding and the SHA-256 if appended."""
    tmp = tempfile.mkdtemp(prefix="popka-size-")
    try:
        f = os.path.join(tmp, "h.bin")
        esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", hex(offset), "0x20", f])
        h = open(f, "rb").read()
        if h[0] != 0xE9:
            return 0
        n, hashed, at = h[1], h[23] == 1, 24
        for _ in range(n):
            esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", hex(offset + at), "0x8", f])
            at += 8 + struct.unpack("<II", open(f, "rb").read()[:8])[1]
        return (at + 16) // 16 * 16 + (32 if hashed else 0)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def backup_job(port):
    """A copy of the firmware on the board (before ours replaces it): into backups/<MAC>-<time>/ — what makes it run
    again: the loader, the partition table, otadata, nvs, the app that boots (its own size); restore.txt — the command
    that writes it back. Progress: by the bytes read."""
    port_user["what"] = "backup"
    loader = False
    try:
        with live.pause():
            job_set(step="Reading the partitions", progress=1)
            text = esptool_run(port, ["--before", "default-reset", "--after", "no-reset", "read-mac"])
            loader = True
            mac = (re.search(r"MAC:\s*([0-9a-f:]+)", text) or [None, "board"])[1].replace(":", "").upper()
            out = os.path.join(BACKUPS, f"{mac}-{time.strftime('%Y%m%d-%H%M%S')}")
            os.makedirs(out, exist_ok=True)
            pt = os.path.join(out, "partitions.bin")
            esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", "0x8000", "0xC00", pt])
            parts = parse_parts(open(pt, "rb").read())
            if not parts:
                raise RuntimeError("Nothing to save: the flash has no partition table")
            ota = next((p for p in parts if p["type"] == 1 and p["sub"] == 0), None)
            otadata = b""
            if ota:
                f = os.path.join(tmp_ota := tempfile.mkdtemp(prefix="popka-ota-"), "otadata.bin")
                esptool_run(port, ["--before", "no-reset", "--after", "no-reset", "read-flash", hex(ota["offset"]), "0x1004", f])
                otadata = open(f, "rb").read()
                shutil.rmtree(tmp_ota, ignore_errors=True)
            app = boot_app(parts, otadata)
            job_set(step="Measuring the app", progress=3)
            size = image_size(port, app["offset"]) if app else 0
            if not size:
                raise RuntimeError("Nothing to save: no app in the flash")
            reads = [r for r in copy_regions(parts, app, size) if r[0] != "partitions.bin"]  # (read already)
            total, done = sum(r[2] for r in reads), 0
            for k, (name, off, n) in enumerate(reads):
                last = k == len(reads) - 1
                job_set(step=f"Saving {name[:-4]}")
                esptool_run(port, ["--before", "default-reset" if last else "no-reset", "--after", "hard-reset" if last else "no-reset", "read-flash",
                                   hex(off), hex(n), os.path.join(out, name)],
                            progress=lambda pc, d=done, n=n: job_set(progress=5 + 94 * (d + n * pc / 100) / total))
                done += n
            loader = False
            files = [(hex(off), name) for name, off, _ in copy_regions(parts, app, size)]
            with open(os.path.join(out, "restore.txt"), "w", encoding="utf-8") as f:
                f.write("The firmware that was on this board before PopkaStudio's (saved by PopkaStudio). To put it back:\n"
                        "  esptool -p <port> write-flash " + " ".join(f"{a} {b}" for a, b in files) + "\n")
            rel = os.path.relpath(out, ROOT)
            job_set(state="done", step=f"Saved in {rel}", progress=100, result=rel)
    except Exception as e:  # noqa: BLE001 — anything goes to the page
        if loader:
            try:
                esptool_run(port, ["--before", "default-reset", "--after", "hard-reset", "read-mac"])
            except RuntimeError:
                pass
        msg = str(e)
        job_set(state="error", step="Failed: " + msg.replace("BOOT: ", "").splitlines()[0][:200],
                hint="boot" if msg.startswith("BOOT: ") else "")
    finally:
        port_user["what"] = ""
        boards.ask_again(port)


# ---------- Load: arduino-cli compiles the sketch and uploads it ----------
# The board's settings (as Waveshare builds its examples): 32 MB flash, PSRAM, the partitions with the music's "ffat"
FQBN = ("esp32:esp32:esp32s3:FlashSize=32M,PSRAM=opi,FlashMode=qio,PartitionScheme=app5M_fat24M_32MB,"
        "USBMode=hwcdc,CDCOnBoot=cdc,UploadSpeed=460800")


def arduino_cli():
    """arduino-cli: the one install.sh put next to PopkaStudio, or one on the PATH."""
    for path in (os.path.join(ROOT, ".tools", "bin", "arduino-cli"), shutil.which("arduino-cli"),
                 os.path.expanduser("~/.local/bin/arduino-cli"), "/opt/homebrew/bin/arduino-cli", "/usr/local/bin/arduino-cli"):
        if path and os.path.exists(path):
            return path
    raise RuntimeError("arduino-cli not found: run ./install.sh")


def load_commands(port):
    cli, out = arduino_cli(), os.path.join(ROOT, "build", "PopkaWatch")
    return [[cli, "compile", "--fqbn", FQBN, "--libraries", os.path.join(ROOT, "lib"), "--build-path", out, SKETCH],
            [cli, "upload", "--fqbn", FQBN, "-p", port, "--input-dir", out, SKETCH]]


# ---------- Flash: one job at a time, the page polls it ----------
job = {"id": 0, "state": "idle", "step": "", "progress": 0, "log": [], "kind": "", "hint": "", "result": ""}
job_lock = threading.Lock()


def job_log(line):
    if not line.strip():
        return
    with job_lock:
        job["log"].append(line.rstrip())
        if len(job["log"]) > 2000:
            del job["log"][:500]


def job_set(**kw):
    with job_lock:
        job.update(kw)


def flash_job(project, port, meta=None):
    with live.pause():  # (Live gives the port back to the upload and the time; takes it again after)
        flash_steps(project, port, meta)
    boards.ask_again(port)  # (who it is now: this project)


def flash_steps(project, port, meta=None):
    t0 = time.time()
    try:
        job_set(step="Preparing the firmware", progress=2)
        job_log(write_sketch(project, SKETCH, meta or {}).strip())
        job_set(step="Compiling", progress=8)
        if not port:
            raise RuntimeError("No board: connect it over USB")
        for cmd in load_commands(port):
            proc = subprocess.Popen(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
            for line in proc.stdout:
                m = re.search(r"Writing at 0x[0-9a-f]+ (?:\[[^\]]*\]\s+|\.*\s*\()([\d.]+)\s*%", line)
                if m:
                    job_set(step="Uploading", progress=40 + 0.6 * float(m.group(1)))
                    continue  # 50 progress lines are no news
                if line.startswith("Sketch uses") or "Connecting" in line:
                    job_set(step="Uploading", progress=40)
                job_log(line)
            if proc.wait():
                raise RuntimeError(f"arduino-cli {cmd[1]} failed (the log above)")
        if any(layer["type"] == "music" for scr in clean_screens(project) for layer in scr["layers"] if not layer["hidden"]):
            job_set(step="Music", progress=97)
            try:
                board_music(port, job_log)
            except Exception as e:  # noqa: BLE001 — the firmware is on the board; only the music is missing
                job_log(f"Music: not written ({e})")
        job_set(step="Setting the time", progress=99)
        try:
            set_board_time(port, job_log)
        except Exception as e:  # noqa: BLE001 — the firmware is on the board; only the time is late
            job_log(f"Time: not set ({e})")
        job_set(state="done", step=f"On the board in {time.time() - t0:.0f} s", progress=100)
    except Exception as e:  # noqa: BLE001 — anything goes to the page
        job_log(str(e))
        with job_lock:
            stuck = boot_trouble("\n".join(job["log"][-60:]))  # (the upload couldn't reach the board's loader)
        job_set(state="error", step="Failed: " + ("the board didn't switch to flashing" if stuck else str(e).splitlines()[0][:200]),
                hint="boot" if stuck else "")


# ---------- The projects: a file each in projects/, its id — the file's name; every save gets the next revision ----------
# Two windows on the same project (the app's browser and Chrome, say) would overwrite each other. A save says which
# revision it was made from; an older one is refused with the current project, and every window checks the revision
# every couple of seconds and takes the newer project — so the last change wins and nothing is lost silently.
# Next to a project — its picture (id.png: the watch face, made by the page) for the start window; a deleted project
# goes to projects/.trash.
PROJECT_ID = re.compile(r"[a-z0-9][a-z0-9-]{0,47}")
project_lock = threading.Lock()


def project_path(pid, ext=".json"):
    if not PROJECT_ID.fullmatch(pid or ""):
        raise ValueError("no such project")
    return os.path.join(PROJECTS, pid + ext)


def read_project(pid="main"):
    """The project (None — there is none)."""
    path = project_path(pid)
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    data.setdefault("rev", 0)
    return migrate(data)


def write_project(pid, data):
    os.makedirs(PROJECTS, exist_ok=True)
    tmp = os.path.join(PROJECTS, f".{pid}.json")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1, ensure_ascii=False)
    os.replace(tmp, project_path(pid))


def save_project(data, pid="main"):
    """data["rev"] — the revision the window had; -> (True, new rev) or (False, the current project)."""
    with project_lock:
        cur = read_project(pid)
        if cur is None:
            raise ValueError("no such project")
        if "rev" in data and data["rev"] != cur["rev"]:  # (a page from before revisions has none: it just saves)
            return False, cur
        data["rev"] = cur["rev"] + 1
        write_project(pid, data)
        return True, data["rev"]


def project_name(pid, data):
    return str(data.get("name") or pid.replace("-", " ").capitalize())


def list_projects():
    """All projects, the last edited first: id, name, board, when edited and when its picture was made (0 — none)."""
    out = []
    for path in glob.glob(os.path.join(PROJECTS, "*.json")):
        pid = os.path.basename(path)[:-5]
        if not PROJECT_ID.fullmatch(pid):
            continue
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, ValueError):
            continue
        thumb = project_path(pid, ".png")
        out.append({"id": pid, "name": project_name(pid, data), "board": data.get("board", "amoled175"),
                    "edited": os.path.getmtime(path), "thumb": os.path.getmtime(thumb) if os.path.exists(thumb) else 0})
    return sorted(out, key=lambda p: -p["edited"])


def new_project(board, name=""):
    """A new project for the board: HaloWatch's watch face, under the name given (none — Project 1, 2, …) -> its id:
    the name in small Latin letters and dashes (none of them — project-N), made unique."""
    with project_lock:
        have = list_projects()
        names, ids = {p["name"] for p in have}, {p["id"] for p in have}
        name = " ".join(str(name).split())[:40]
        if not name:
            n = 1
            while f"Project {n}" in names:
                n += 1
            name = f"Project {n}"
        base = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")[:40].strip("-") or "project"
        pid, n = base, 2
        while pid in ids or os.path.exists(project_path(pid)):
            pid, n = f"{base}-{n}", n + 1
        write_project(pid, {**default_project(), "board": board, "name": name, "rev": 0})
        return pid


def rename_project(pid, name):
    with project_lock:
        data = read_project(pid)
        if data is None:
            raise ValueError("no such project")
        data["name"] = " ".join(str(name).split())[:40] or project_name(pid, {})
        data["rev"] += 1
        write_project(pid, data)


def delete_project(pid):
    """Not gone: to projects/.trash (with its picture), under the time it went there."""
    with project_lock:
        trash = os.path.join(PROJECTS, ".trash")
        os.makedirs(trash, exist_ok=True)
        stamp = time.strftime("%Y%m%d-%H%M%S")
        for ext in (".json", ".png"):
            if os.path.exists(project_path(pid, ext)):
                os.replace(project_path(pid, ext), os.path.join(trash, f"{pid}-{stamp}{ext}"))


# ---------- HTTP ----------
TYPES = {".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8", ".css": "text/css",
         ".wasm": "application/wasm", ".json": "application/json", ".svg": "image/svg+xml", ".png": "image/png",
         ".ttf": "font/ttf", ".woff2": "font/woff2", ".webp": "image/webp"}


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        if self.path.startswith("/api/") and not self.path.startswith("/api/job"):
            sys.stderr.write("  " + fmt % args + "\n")

    def send(self, code, body, ctype="application/json"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        elif isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def body(self):
        n = int(self.headers.get("Content-Length") or 0)
        return json.loads(self.rfile.read(n) or b"{}")

    def allowed(self):
        """Only this computer's own page: the Host must be ours (a site renamed to 127.0.0.1 — DNS rebinding — is not),
        and a request a page sends must come from our page (any site open in the browser could send one: CSRF)."""
        ours = {f"localhost:{PORT}", f"127.0.0.1:{PORT}"}
        if self.headers.get("Host", "") not in ours:
            return False
        origin = self.headers.get("Origin")
        if origin is not None and origin not in {"http://" + h for h in ours}:
            return False
        return self.headers.get("Sec-Fetch-Site", "same-origin") in ("same-origin", "none")

    def do_GET(self):
        if not self.allowed():
            return self.send(403, {"error": "forbidden"})
        url = urllib.parse.urlparse(self.path)
        q = urllib.parse.parse_qs(url.query)
        pid = q.get("p", ["main"])[0]
        if url.path in ("/api/project", "/api/project/rev", "/api/thumb"):
            try:
                if url.path == "/api/thumb":
                    if not os.path.exists(project_path(pid, ".png")):
                        return self.send(404, {"error": "no picture"})
                    with open(project_path(pid, ".png"), "rb") as f:
                        return self.send(200, f.read(), "image/png")
                data = read_project(pid)
            except ValueError:
                data = None
            if data is None:
                return self.send(404, {"error": "no such project"})
            return self.send(200, data if url.path == "/api/project" else {"rev": data["rev"]})
        if url.path == "/api/projects":
            return self.send(200, {"projects": list_projects()})
        if url.path == "/api/default":
            return self.send(200, default_project())
        if url.path == "/api/wifi":
            return self.send(200, wifi_info())
        if url.path == "/api/geo":  # ?q= a city or a US ZIP
            try:
                return self.send(200, {"results": geo_search(q.get("q", [""])[0])})
            except Exception as e:  # noqa: BLE001 — no internet
                return self.send(502, {"error": "No internet", "detail": str(e)[:200]})
        if url.path == "/api/about":
            return self.send(200, {"version": VERSION})
        if url.path == "/api/boards":
            return self.send(200, {"boards": BOARDS, "ports": boards.status()})
        if url.path == "/api/live":
            return self.send(200, live.status())
        if url.path == "/api/job":
            start = int(q.get("from", ["0"])[0])
            with job_lock:
                return self.send(200, {**{k: v for k, v in job.items() if k != "log"},
                                       "log": job["log"][start:], "lines": len(job["log"])})
        # Files: the page, and the font for its own interface (tools/fonts — not in git)
        rel = url.path.lstrip("/") or "index.html"
        base = os.path.join(ROOT, "tools/fonts") if rel.startswith("fonts/") else WEB
        if rel.startswith("fonts/"):
            rel = rel[len("fonts/"):]
        path = os.path.realpath(os.path.join(base, rel))
        if not path.startswith(os.path.realpath(base) + os.sep) or not os.path.isfile(path):
            return self.send(404, {"error": "not found"})
        with open(path, "rb") as f:
            self.send(200, f.read(), TYPES.get(os.path.splitext(path)[1], "application/octet-stream"))

    def do_POST(self):
        if not self.allowed():
            return self.send(403, {"error": "forbidden"})
        url = urllib.parse.urlparse(self.path)
        try:
            data = self.body()
            if url.path == "/api/font":  # {"type", sizes…; "part": "icon" — its second font}
                spec = (font2_spec if data.get("part") == "icon" else font_spec)(clean_layer(data))
                if not spec:
                    return self.send(400, {"error": "no font"})
                return self.send(200, cached(("font", spec), lambda: font_blob(spec)), "application/octet-stream")
            if url.path == "/api/icons":  # {"type": the layer's kind, "scale"; the info line: "size", "format"}
                iset = icon_set(clean_layer({k: data[k] for k in ("type", "scale", "size", "format") if k in data}
                                            | {"type": data.get("type", "weather")}) or {"type": ""})
                if not iset:
                    return self.send(400, {"error": "no icons"})
                return self.send(200, cached(("icons",) + iset, lambda: icons_blob(*iset)), "application/octet-stream")
            if url.path == "/api/projects":  # {"board", "name"} — a new one; {"rename": id, "name"}; {"delete": id}
                if data.get("rename"):
                    rename_project(data["rename"], data.get("name", ""))
                elif data.get("delete"):
                    delete_project(data["delete"])
                else:
                    board = data.get("board")
                    if not any(b["id"] == board and not b.get("soon") for b in BOARDS):
                        return self.send(400, {"error": "no such board"})
                    return self.send(200, {"id": new_project(board, data.get("name", "")), "projects": list_projects()})
                return self.send(200, {"projects": list_projects()})
            if url.path == "/api/weather":  # {"cities": [{lat, lon, name}], "units"} — what the page's copies show
                wx = clean_weather({"weather": data})
                try:
                    return self.send(200, {"data": weather_for(wx["cities"], wx["units"])})
                except Exception as e:  # noqa: BLE001
                    return self.send(502, {"error": "No internet", "detail": str(e)[:200]})
            if url.path == "/api/wifi":  # {"scan": true} — ask the board; {"ssid", "password"} — the board's network
                if data.get("scan"):
                    with job_lock:
                        if job["state"] == "running":
                            return self.send(409, {"error": "Flashing"})
                    try:
                        return self.send(200, wifi_scan())
                    except LookupError as e:
                        return self.send(409, {"error": str(e), **wifi_info()})
                ssid, password = str(data.get("ssid") or "")[:32], data.get("password")
                if not ssid:
                    return self.send(400, {"error": "no network"})
                saved = wifi_read()
                if password is not None:
                    saved["nets"][ssid] = str(password)[:63]
                elif ssid not in saved["nets"] and ssid != shared_ssid():
                    return self.send(400, {"error": "no password"})
                saved["current"] = ssid if ssid in saved["nets"] else ""  # (the shared one: HaloClock.h)
                wifi_write(saved)
                return self.send(200, wifi_info())
            if url.path == "/api/live":  # {"on", "port", "project"} — Live on or off; the project as it is now
                project = data.get("project") if isinstance(data.get("project"), dict) else None
                return self.send(200, live.set(bool(data.get("on")), str(data.get("port") or ""), project))
            if url.path == "/api/check":  # {"port"} — what the board is (it restarts; nothing written)
                port = data.get("port") or next((p["port"] for p in usb_ports()), "")
                with job_lock:
                    busy = job["state"] == "running"
                if not port or busy or port_user["what"]:
                    return self.send(409, {"error": "No board" if not port else "The board is busy"})
                try:
                    return self.send(200, board_check(port))
                except RuntimeError as e:
                    msg = str(e)
                    return self.send(500, {"error": msg.replace("BOOT: ", ""), "hint": "boot" if msg.startswith("BOOT: ") else ""})
            if url.path == "/api/backup":  # {"port"} — a copy of the firmware on it (a job: /api/job)
                port = data.get("port") or next((p["port"] for p in usb_ports()), "")
                if not port or port_user["what"]:
                    return self.send(409, {"error": "No board" if not port else "The board is busy"})
                with job_lock:
                    if job["state"] == "running":
                        return self.send(409, {"error": "Busy"})
                    job.update(id=job["id"] + 1, state="running", step="Starting", progress=0, log=[], kind="backup", hint="",
                               result="")
                threading.Thread(target=backup_job, args=(port,), daemon=True).start()
                return self.send(200, {"id": job["id"]})
            if url.path == "/api/flash":  # {"project", "port", "pid", "rev"}
                port = data.get("port") or next((p["port"] for p in usb_ports()), "")
                if not port:
                    return self.send(409, {"error": "No board: connect it by USB"})
                with job_lock:
                    if job["state"] == "running":
                        return self.send(409, {"error": "Already flashing"})
                    job.update(id=job["id"] + 1, state="running", step="Starting", progress=0, log=[], kind="load", hint="",
                               result="")
                meta = {"pid": str(data.get("pid") or ""), "rev": int(data.get("rev") or 0)}
                threading.Thread(target=flash_job, args=(data.get("project", {}), port, meta), daemon=True).start()
                return self.send(200, {"id": job["id"]})
            return self.send(404, {"error": "not found"})
        except Exception as e:  # noqa: BLE001
            return self.send(500, {"error": str(e)})

    def do_PUT(self):
        if not self.allowed():
            return self.send(403, {"error": "forbidden"})
        url = urllib.parse.urlparse(self.path)
        pid = urllib.parse.parse_qs(url.query).get("p", ["main"])[0]
        try:
            if url.path == "/api/thumb":  # the watch face's picture, PNG (the page makes it after a save)
                n = int(self.headers.get("Content-Length") or 0)
                png = self.rfile.read(n) if n <= 1 << 20 else b""
                if not png.startswith(b"\x89PNG") or read_project(pid) is None:
                    return self.send(400, {"error": "not a picture of a project"})
                with open(project_path(pid, ".png"), "wb") as f:
                    f.write(png)
                return self.send(200, {"ok": True})
            if url.path != "/api/project":
                return self.send(404, {"error": "not found"})
            ok, result = save_project(self.body(), pid)
        except ValueError:
            return self.send(404, {"error": "no such project"})
        return self.send(200, {"rev": result}) if ok else self.send(409, {"project": result})


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True
    request_queue_size = 64  # (the page asks for every screen's fonts at once: with 5, some were refused — "Failed to fetch")


def main():
    if "--template" in sys.argv:
        print(write_sketch(halowatch_project(), FIRMWARE))
        return
    build_engine(force="--build" in sys.argv)
    if "--build" in sys.argv:
        return
    global PORT, PROJECTS
    if "--port" in sys.argv:  # (another port: a second copy, a test)
        PORT = int(sys.argv[sys.argv.index("--port") + 1])
    if "--projects" in sys.argv:  # (another folder of projects: a test)
        PROJECTS = os.path.abspath(sys.argv[sys.argv.index("--projects") + 1])
    server = Server(("127.0.0.1", PORT), Handler)
    print(f"PopkaStudio: http://localhost:{PORT}  (Ctrl+C — stop)", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
