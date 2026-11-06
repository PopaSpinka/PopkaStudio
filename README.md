# PopkaStudio

**Design watch screens for ESP32 boards in your browser — and see them on the real watch in one click.**

[Русская версия](README.ru.md) · [Video: how it started (YouTube)](https://www.youtube.com/watch?v=7rBIQl6EG08)

![PopkaStudio](docs/screenshots/editor.png)

PopkaStudio is a visual editor for round watch interfaces. You put together a watch face, apps and screens from
ready parts, link them like frames in Figma, and press **Load** — the board gets firmware built from exactly what you
made, running smoothly at 60 frames per second.

The preview is not a mock-up: it is **the board's own code**, compiled to WebAssembly. What you see in the editor is
what the watch shows, pixel for pixel — the same fonts, the same animations, the same touch.

## What it can do

- **Watch faces from layers** — big clock, dot clock, date lines, curved text along the edge, weather, steps,
  battery, forecast, a starry sky with meteors, pixel art (meet **Popka**, the parrot). Move, resize, recolor
  everything; any number of copies.
- **Apps** — a gallery of cards (as on modern smartwatches), Weather with real data and several cities, Steps (the
  motion chip's own pedometer), Display brightness, Battery, a Music player, a voice Recorder, three self-playing
  snake games, and About and Power — live cards about the board itself (processor, memory, battery, temperature,
  sound, motion; voltages).
- **A map of screens** — every screen at once, links drawn as curves; tap a card or a layer to open a screen, with
  transitions.
- **Try** — use the watch right in the editor, the mouse as your finger.
- **Live** — with the board on USB, moves and colors change on the watch as you drag them, no Load needed.
- **It knows your board** — the dot next to the board's name: green — this project is on it, blue — PopkaStudio
  firmware from another project or an older version, yellow — someone else's firmware. **Check the board** reads it
  without writing anything; **Save its firmware** keeps a copy of someone else's firmware before your first Load.
- **Functions** — Always On display, what the side buttons do (screen off, lock), waking up by touch, raising your
  wrist or a button, Wi-Fi (exact time and fresh weather), Bluetooth.
- **Only what you add gets in.** A layer you don't use isn't in the firmware at all — no code, no fonts, sensors off.

![The map of screens](docs/screenshots/map.png)

## The board

[Waveshare ESP32-S3-Touch-AMOLED-1.75C](https://www.waveshare.com/esp32-s3-touch-amoled-1.75c.htm): a round 1.75″
AMOLED, 466 × 466, touch, a motion sensor, a speaker and microphones. The 7″ ESP32-P4 board is coming next.

![The board](PopkaStudio/web/img/board-amoled175.webp)

## Install — macOS and Linux

One command:

```bash
curl -fsSL https://raw.githubusercontent.com/PopaSpinka/PopkaStudio/main/install.sh | bash
```

Prefer to look first? `git clone https://github.com/PopaSpinka/PopkaStudio.git`, read `install.sh`, run
`./install.sh`.

It puts everything into the PopkaStudio folder (`~/PopkaStudio`), nothing system-wide: its own Python with numpy,
Pillow and pyserial ([uv](https://docs.astral.sh/uv/)), [arduino-cli](https://arduino.github.io/arduino-cli/) with
the ESP32 core 3.3.11, and the fonts. The ESP32 tools are the big part: 1.5–3 GB, 5–20 minutes the first time. On
Linux it asks for your password once, to let you use the USB port (then log out and back in).

Then:

```bash
popkastudio
```

It opens **http://localhost:8466**.

**Updates:** when a new version is out, a blue number appears on the heart at the top right — how many updates
since yours. Click it to see what's new in each one, then **Update**: PopkaStudio updates itself and restarts, and you
**Load** your project again. **About → Check for updates** checks right away. (Running `install.sh` again updates too.)

Windows is not supported.

## First steps

1. **Choose a board**, then **New** — a project. It starts as an empty black watch face: you add only what you need.
2. Add parts from the folders at the top of the left card: **Faces** (a whole face), **Layers** (its parts), **App**.
   A tile with a green frame is already in your project.
3. Connect the board with a USB-C cable and press **Load** (top right). The first Load builds everything and takes a
   minute or two; the next ones are faster.

Wi-Fi is off in a new project — the clock takes its time from your computer at each Load. Choose **From: Wi-Fi** for
a clock's time or pick a weather city, and the Wi-Fi window lists the networks your board sees.

Your projects are saved on your computer (`PopkaStudio/projects/`). The Wi-Fi password stays on your computer too
(`PopkaStudio/secrets/`) and goes only into the firmware it builds.

To go back to the firmware that was on the board, use the copy from **Save its firmware** (`PopkaStudio/backups/`,
with `restore.txt`), or see [Waveshare's examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C) for
the factory demo.

## Trouble?

- **No board in the list** — try another cable (some carry only power). Linux: did you log out and back in after
  installing? Ubuntu's ModemManager can grab the port: `sudo systemctl stop ModemManager`.
- **Load can't reach the board** — PopkaStudio shows how: hold **BOOT**, plug the cable in again, then **Load
  again**.
- **Load failed** — the log is under the Load button; open an [issue](https://github.com/PopaSpinka/PopkaStudio/issues)
  with it.
- **The board hangs** — hold PWR for 8–10 seconds, then turn it on again.

## Privacy

No accounts, no telemetry. The editor talks only to [Open-Meteo](https://open-meteo.com) (the weather for the cities
you pick), [Zippopotam.us](https://zippopotam.us) (when you look up a US ZIP code) and GitHub — once a day, for the
list of PopkaStudio's releases (the same request any browser makes; nothing about you is sent). The watch connects to
your Wi-Fi only for the time and the weather, and only if you turn Wi-Fi on.

## Plans

I'm going to keep developing PopkaStudio — I love ESP32 boards and devices like them, and making beautiful
interfaces for them that run flawlessly at 60 fps and more.

- More boards: the 7″ ESP32-P4 (1024 × 600) next, then other round and square screens.
- Your own GIF or video clip as a pixel-art layer, in seconds.
- More apps and widgets, more watch faces.
- Even smoother motion and longer battery life.

**Board makers:** want your board in PopkaStudio and the Halo library, tuned for smooth 60+ fps? Write to
**lastmail.sk@gmail.com**.

## License

[PolyForm Noncommercial 1.0.0](LICENSE): **free for personal, hobby, educational and other noncommercial use.**
Selling devices, firmware, faces or services made with PopkaStudio or Halo needs a commercial license — see
[COMMERCIAL.md](COMMERCIAL.md). The watches you design are yours.

## Credits

![About](docs/screenshots/about.png)

Made by **[PopaSpinka](https://github.com/PopaSpinka)** together with **Claude Opus 5.5** (Anthropic) — every screen,
every spring, every pixel. Thank you, Claude.

The code is written from scratch. The motion follows published open-source numbers, no firmware or app was
decompiled, and no code was taken from GPL projects:

| Source | License | What was used |
| --- | --- | --- |
| [androidx](https://github.com/androidx/androidx) — Compose for Wear OS | Apache 2.0 | spring stiffness and damping, durations and easing curves, how cards shrink at the edge, swipe back |
| [Horologist](https://github.com/google/horologist) | Apache 2.0 | the player: sizes, the progress ring, icon timing |
| [wear-os-samples](https://github.com/android/wear-os-samples) | Apache 2.0 | how a screen is put together from components |
| [PebbleOS](https://github.com/coredevices/PebbleOS) | Apache 2.0 | screen transitions on a microcontroller |

Third-party parts:

| What | Author | License |
| --- | --- | --- |
| Google Sans Flex font (downloaded by the installer; glyphs rendered into the firmware) | Google | [SIL OFL 1.1](https://openfontlicense.org) |
| Font Awesome Free icons (downloaded by the installer; rendered into the firmware) | Fonticons, Inc. | [CC BY 4.0](https://fontawesome.com/license/free) |
| Weather data | [Open-Meteo.com](https://open-meteo.com) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| US ZIP code lookup | [Zippopotam.us](https://zippopotam.us) | free API |
| ES8311 codec register sequence (`lib/Halo/src/halo_audio.cpp`) | based on Espressif's es8311 driver | Apache 2.0 |
| Board pins and display start-up sequence | based on [Waveshare's examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C) | Apache 2.0 |
| [ArduinoJson](https://arduinojson.org) (`lib/ArduinoJson`) | Benoît Blanchon | MIT |
| [Emscripten](https://emscripten.org) runtime (`PopkaStudio/web/build`) | Emscripten authors | MIT |
| [Electron](https://www.electronjs.org) (the optional Mac app) | OpenJS Foundation | MIT |
| Music — 20-second excerpts of [Musopen](https://musopen.org) recordings | see [firmware/music](PopkaStudio/firmware/music/README.md) | public domain, CC0 |
| The board photo | PopaSpinka | part of this repository |

PopkaStudio is an independent project, not affiliated with or endorsed by Google, Espressif or Waveshare. Wear OS and
Android are trademarks of Google LLC; ESP32 is a trademark of Espressif Systems; Waveshare is a trademark of Waveshare
Electronics.
