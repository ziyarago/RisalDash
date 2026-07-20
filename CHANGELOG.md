# Changelog

All notable changes to RisalDash are documented here. The format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions follow [semver](https://semver.org/).

## [Unreleased]

## [0.11.0] — 2026-07-20

"Over-the-air, on the map." Firmware updates and Wi-Fi reset without a cable, a real-world GPS
tracker project, and a much better map.

### Added
- **OTA firmware updates** — `dash.enableOTA()` serves `/update`, and when enabled an **"Update
  firmware"** button appears in Settings (localized en/ru/uz/ar). Upload a `.bin` from the browser;
  no USB.
- **"Reset Wi-Fi"** button in Settings — confirms, erases saved credentials (`forgetWiFi()`), and
  reboots into the captive setup portal. Built in, no sketch code.
- **`dash.link(name, label, url)`** — a button that navigates to a URL (a custom page like `/tracks`,
  `/update`, or another dashboard).
- **`dash.server()`** — public accessor for the underlying `AsyncWebServer`, so sketches can register
  their own routes (custom REST endpoints, file downloads, streaming) on the dashboard's port.
- **`RisalDash.h`** umbrella header (equivalent to `RisalUI.h`).
- **Map geofence overlay** — `dash.map(...).geofence(&homeLat, &homeLon, &radiusM)` draws a circle.
- **GPS Tracker project** (`examples/06_Projects/GPS-Tracker`) — NEO-6M, home geofence with alarm,
  day-by-day CSV logs with wear-safe writes, streaming CSV→GPX export, a `/tracks` download page,
  NTP time assist, and a boot heartbeat on the alert output. Runs on ESP32 and ESP8266.

### Fixed
- **ESP8266 OTA** crashed mid-upload under the async server — now calls `Update.runAsync(true)`.
- **Map** no longer draws a line from `0,0` to the first fix, filters out parked GPS drift, and spans
  the full grid width on every screen size.
- **Wi-Fi join** retries a few times before falling back to the portal (a single timeout after a soft
  reset, or with a second radio sharing 2.4 GHz, no longer drops a configured device into setup).
- **Card layout** distributes content vertically so widgets fill their height instead of clumping.

## [0.10.0] — 2026-07-10

"A fake for every sensor." A big sensor + example release: build a full dashboard for almost any
sensor with no hardware, then swap in the real driver with your variable names unchanged.

### Added
- **Sensor presets grown to ~60** across every category — `dash.sensor("id", …)` now maps up to
  **8 quantities** per sensor (was 4) and gained a `stat` readout kind. New presets span air quality
  (SEN55, PMS5003, SDS011, SPS30, SGP30/40/41, ENS160, MH-Z19, SFA30), power/energy/BMS (INA226,
  CT-clamp, HLW8012/CSE7766, Daly-style `bms`), light + UV (LTR390, VEML6075), weight (HX711,
  NAU7802), flow/pulse (`flow`, `pulse`, `rpm`), IMU + compass (MPU6050/9250 with gyro, BNO055,
  QMC5883L), thermocouples (MAX6675/31855/31865), weather, water quality (TDS/pH/turbidity),
  health (MAX30102), gas (MQ-2/7/9), and a `system` diagnostics preset.
- **9 new `RisalFake*` bundles** — `RisalFakePower, RisalFakeAir, RisalFakeIMU, RisalFakeWeight,
  RisalFakeWeather, RisalFakeBattery, RisalFakeFlow, RisalFakeHealth, RisalFakeGas` — realistic,
  correlated models (a room fills with CO₂ then airs out, a mains load cycles, a battery cycles…),
  one paired with each sensor category.
- **`radioBrowser` widget** — an editable list (drag to reorder, inline-edit name/URL, add/remove);
  its value round-trips as `Name | URL | Meta` lines, so parsing/persistence stays yours.
- **10 new `04_Templates` examples**, each a preset paired with its fake and buildable with no
  hardware: AirQuality, PowerMeter, WeatherStation, MotionIMU, WeightScale, WaterQuality, WaterMeter,
  BatteryBMS, HealthMonitor, GasSafety — plus AquariumController, SmartHome, CNCMachine, Printer3D.
- **ESP32-S3 board showcase** gains internet radio via the ES8311 codec (editable station list) and
  a mic → proxy (Whisper → Claude → TTS) voice assistant whose robot eyes reflect the reply's mood.
- **ESP32-C6 showcase** gains a live BLE slide: nearby devices with signal bars + a decoded
  Xiaomi temp/humidity beacon (ATC/pvvx + MiBeacon).

### Changed
- `dash.sensor()` signature extended to 8 optional pointer params (backward compatible — existing
  calls are unchanged). CI matrix now builds 25 examples.

## [0.9.0] — 2026-07-08

A structural release: the library got split into readable modules and audited end to end on real
boards (ESP8266 D1 mini + ESP32-C6) — the firmware got smaller on every target while gaining
features and fixing bugs the audit surfaced.

### Added
- **`dash.forgetWiFi()`** — erase the saved credentials and reboot into the captive setup portal
  (previously only possible by erasing flash). See the new `01_Start/WifiSetup` example.
- **Live status-bar telemetry** — the Wi-Fi RSSI in the status bar now updates every ~5 s over the
  WebSocket (reserved `_rssi` key); a bound real battery (`dash.battery`) rides along as `_bat`.
- **Themed time & color pickers** — `dash.time()` opens an hour/minute popover scrolled to the
  current value; `dash.color()` opens a 16-swatch grid (also used by the Settings modal). No more
  native OS pickers clashing with the theme; all four popovers (select/date/time/color) now close
  each other and lift their card above neighbours.
- **New examples** — `WidgetSizes` (the S/M/L cell grid), `FakeSensors` (the whole RisalFake
  toolbox), `UiKit` (every structural element in one sketch), `WifiSetup` (the provisioning
  circle), and a `Wemos-D1-Mini` board showcase (real onboard LED/A0/heap + a fake demo page).
- **Cyrillic LCD labels (ESP32-C6 example)** — Russian slide labels render via a compact U8g2 font.
- **More LCD widget styles (ESP32-C6 example)** — LED indicator, progress bar, multi-metric grid
  and a thermal heatmap; the carousel now has 13 slides.

### Changed
- **The source got a structure** — `RisalWidget.h` (1459 lines) is now an umbrella over
  `src/widgets/{base,icons,display,controls,inputs,visual,layout}.h`; `RisalUI.cpp` (1183 lines)
  split into core + Render/Provision/Api/Sensors/Mqtt translation units; assets share one PROGMEM
  copy. Public API unchanged; flash SHRANK on every board (up to −2.2 KB) and ESP8266 freed 816 B
  of RAM.
- **`examples/` is a learning path** — numbered categories (`01_Start` → `05_Boards`) with an
  index README; the Arduino IDE menu now reads in order.
- **`RISAL_MAX_WIDGETS` default 32 → 48** — widgets past the cap were silently dropped.
- **The browser tab shows the device title**, not the brand.
- **The mobile nav drawer hides when `layout()` pages exist** — the swipe pager owns navigation.
- **First paint is localized** — toggle/led render Вкл/Выкл (etc.) immediately; badge labels and
  radio options run through `dash.translate()`.
- **ESP32-C6 example now does a real BLE scan** (NimBLE) instead of the simulated feed, live
  alongside the Wi-Fi dashboard; its sketch slimmed 533 → ~275 lines by moving translations, data
  sources, weather and BLE into their own headers.

### Fixed
- **The linear gauge (`.variant("bar")`) never rendered correctly** — its `bar` class collided
  with the core `.bar` (metric track) CSS, which clipped the value inside a 7-px pill. The variant
  now emits class `lin`.
- **Uzbek strings were corrupted** by a greedy hex escape (`»c` → out-of-range char): «Oʻchiq»
  now renders correctly everywhere.
- **Removed dead weight** shipped with every page: the unused `.lng`/`.lhandle`/orbs CSS+DOM, the
  no-op `.span()` API, and the never-wired `lang/` generator.

### Removed
- **`.span()`** — the size presets (`.size(RSIZE_S/M/L)`) replaced it long ago; it did nothing.

## [0.8.1] — 2026-07-07

### Fixed
- README hero image now uses an absolute URL, so it renders on the ESP Component Registry (and
  anywhere the README is shown outside GitHub), not only in the repo.

## [0.8.0] — 2026-07-07

Building blocks and starting points: a multi-line input, tunable sensor templates, gauge looks, and
two ready-to-ship dashboard templates.

### Added
- **`dash.textarea(name, &str)`** — multi-line text input bound to a `String`, for notes, a small
  JSON/config blob, or any longer field. Rounds out the custom UI primitives (input · textarea ·
  select · tabs). Not in ESPUI.
- **Sensor templates are tunable** — `dash.sensor(...)` now returns a handle you can chain:
  `.chart()` adds a live trend chart for every quantity, `.size(RSIZE_S/M/L)` resizes the readouts.
  So the same preset ships as a compact card, a big readout, or a full readout-plus-trend layout.
  New `examples/Any-ESP/SensorTemplates`.
- **Gauge design variants** — `dash.gauge(...).variant("ring" | "semi" | "bar")`: the full-circle
  ring (default), a speedometer half-circle, or a linear bar. Same data, three looks; pick what
  fits the card size and the quantity.
- **Ready-made dashboard templates** — `examples/Templates/Greenhouse` and
  `examples/Templates/EnergyMonitor`: complete, opinionated dashboards you copy, bind to your
  pins/sensors, and ship. They showcase the sensor presets, gauge variants and multi-page layout.

## [0.7.0] — 2026-07-07

Device chrome that remembers itself: the status bar shows only the radios your board has, the
Settings you pick survive a reboot, and the clock is real.

### Added
- **`dash.gsm()` / `dash.bluetooth()`** — status-bar radios. Wi-Fi always shows (the device serves
  over it); the cellular signal bars appear only if you declare a GSM/LTE modem, and the Bluetooth
  glyph only when BT is active. Both hidden by default.
- **NVS-persisted UI preferences (ESP32)** — the theme, accent and language chosen in the appbar
  Settings now save to the device (not just the browser) and become the served defaults after a
  reboot, alongside the already-persisted Wi-Fi credentials and timezone. New `POST /api/pref`
  endpoint (called by the Settings modal). On ESP8266 these stay per-browser to keep the EEPROM
  credential layout stable.
- **NTP clock** — with no RTC battery, the device now syncs real time over NTP once online, using the
  timezone picked in the first-boot portal (offset applied; skipped in AP-only mode).

## [0.6.0] — 2026-07-07

Multilingual, end to end. The dashboard already spoke several languages in its own chrome; now the
strings *you* write can follow, and the ESP32-C6 LCD localizes too.

### Added
- **Uzbek (`uz`)** across the device chrome — captive portal, status bar and the Settings modal now
  offer en / ru / uz / ar.
- **`dash.translate(fn)`** — register one lookup and the library localizes the strings it can't
  translate itself: widget titles, page/group/tab names, button labels and select options. Called at
  render time as `fn(text, lang)`; zero overhead until you set it. New example:
  `examples/Any-ESP/Multilingual`.
- **LCD localization (ESP32-C6)** — panel labels follow the device language for Latin scripts
  (English, Oʻzbek) and fall back to English for ru/ar, since the stock Arduino_GFX font is
  Latin-only. A **Language** selector in the Settings gear persists to NVS and drives both the web
  dashboard and the LCD together.

### Changed
- Denser **4-column grid on mobile** (was 2) so compact metric cards pack tighter on phones.
- ESP32-C6 example shows the robot **face at L (full-width)** to demonstrate size variety.

## [0.5.0] — 2026-07-06

A big feature release: new widgets, a "develop with no hardware" fake-data engine, live weather, more
sensor presets, and full device settings in the appbar gear.

### Added
- **New widgets** — `dash.face(&mood)` (animated robot face, 10 emotions), `dash.map(&lat, &lon)`
  (Leaflet live map with a trail, dark basemap), `dash.cube(&pitch, &roll, &yaw)` (3D orientation
  cube for an IMU), `dash.terminal(name, cb)` (WebSocket console with a command input), and
  `dash.heatmap(name, cols, rows)` (thermal/heatmap grid, e.g. MLX90640).
- **Fake-data engine — build & debug with no hardware attached.** `#include <RisalFake.h>`:
  `RisalFake` (one realistic drifting signal), `RisalFakeEnv` (a day/night environment with light),
  `RisalFakeGPS` (GPX-style route playback), `RisalFakeBLE` (a scan feed with a Xiaomi beacon), and
  `RisalRecorder` (record a real signal, then replay it on a loop).
- **Live weather** — `#include <RisalWeather.h>`: Open-Meteo (no API key) with `geocode(city)`; meant
  to run in a background FreeRTOS task so the blocking HTTPS fetch never stalls the loop.
- **Device settings in the gear** — `.gear()` now moves *any* control (toggle, slider, number, select,
  colour) into the appbar Settings modal, not just toggles.
- **More sensor presets** — `scd40`, `ld2410`, `ld2450`, `neo-m10`, `inmp441`.
- **New example** — `ESP32-S3-DualCore`: the dashboard on one core while a heavy worker runs on the
  other (real parallelism). The `ESP32-C6-LCD-1.47` example grew into a full showcase (weather with a
  city field, live map, energy panel, robot face, thermal, BLE/Zigbee panels, a console…).

### Note
- Big dashboards may exceed the default `RISAL_MAX_WIDGETS` (32) — raise it with a build flag
  (`-D RISAL_MAX_WIDGETS=64`), not a sketch `#define` (which can't reach `RisalUI.cpp`).

## [0.4.0] — 2026-07-06

### Added
- **`dash.face(&mood)` — animated robot face.** Two glowing accent eyes on a dark panel with idle
  blinking and 10 emotions (Neutral, Happy, Sad, Angry, Surprised, Sleepy, Love, Wink, Dizzy, Look).
  Bind the emotion to your logic or an AI agent and the face reacts live over the WebSocket — an
  expressive status face for AI companions and robots. Renders on the web and (in the C6 example) the
  device LCD.
- **`RisalFake` — fake sensors for hardware-free development.** `#include <RisalFake.h>` gives
  realistic drifting readings (slow trend + wobble + noise, not a flat sine). `RisalFake(center, amp,
  noise)` is one signal; `RisalFakeEnv` bundles a whole environment (temperature/humidity/pressure/
  soil/air quality). Build and debug a dashboard with nothing plugged in, then swap for the real
  driver — the sketch is unchanged. Opt-in, works on ESP8266 + ESP32.
- **`.gear()` — device settings in the appbar gear.** A control marked `.gear()` moves out of the
  grid into the Settings modal, which drives it via `/api/state` + `/api/set` (e.g. an "Auto-slide"
  device toggle), keeping device settings out of the dashboard tiles.

### Changed
- **`select` is now a custom themed dropdown** instead of a native `<select>`: a scoped popup that
  matches the theme (dark/light), an accent-highlighted option, a rotating chevron, RTL support, and
  it lifts its own card above sibling cards on open (cards are `backdrop-filter` stacking contexts, so
  a native popup couldn't be styled to match).

## [0.3.0] — 2026-07-05

Verified end-to-end on real ESP8266 (Wemos D1 mini) hardware. Fixes the biggest ESP8266 gap
(large pages were silently truncated) and reworks the layout / widget-size system.

### Fixed
- **Large pages were truncated on ESP8266** — the whole response was buffered in a single RAM
  block, and heap fragmentation capped it at ~15 KB, so trailing content (the `<script>`) was
  silently dropped: JS broke on every page and big dashboards (AllWidgets) rendered an empty grid.
  The page is now streamed in chunks (`sendChunked` + a windowing `Print` sink) — no big buffer,
  so any page size works. CSS-only pages looked fine, which is why it hid on ESP32 (300 KB RAM).
- **Captive portal dropped its own access point** — the setup page ran a blocking
  `WiFi.scanNetworks()` inside the async request handler on every load, which hopped the radio and
  reset the connected client. The scan now runs once when the portal starts and is cached.
- **Provisioning could join a stale network** — the ESP8266 SDK auto-connected to a previously
  saved AP; `_tryStation` now clears the old STA config (`persistent(false)` + `disconnect(true)`)
  before joining the network the user picked.
- Portal network list: nothing is pre-selected, entries are sorted by signal, hidden SSIDs dropped.

### Added
- **iOS-style cell grid + size presets** — 2 columns on phone (4 on tablet+). Each widget picks a
  footprint: `.size(RSIZE_S)` (1 cell), `RSIZE_M` (full width), `RSIZE_L` (full width, tall), with
  a sensible default per type (gauge/chart/table/ai → L; metric/stat/badge/… → S).
- **Swipeable multi-page layouts** — layout pages swipe left/right (scroll-snap) and a sticky nav
  strip under the app bar shows `‹ PAGE NAME ›`; tapping the name opens the tile sheet.
- **Wi-Fi RSSI** in the status bar (real `WiFi.RSSI()` in STA mode), next to the Wi-Fi icon.
- **`dash.battery(int* pct)`** — bind a real battery percentage (e.g. `map(analogRead(A0), …)`);
  the status-bar battery then shows it instead of the cosmetic simulation.
- Settings modal: **Signal (dBm)** and **Battery** show/hide toggles (per-browser).
- Portal password field: a reveal (eye) toggle, and focus jumps to it when you pick a network.

### Changed
- The `RisalDash vX · served by ESP` footer is now pinned to the bottom of every page.

## [0.2.1] — 2026-06-27

### Added
- **Home Assistant MQTT discovery** — `dash.enableHomeAssistant("node")` (after `mqtt()`) publishes
  retained discovery configs so HA auto-creates entities (sensor / switch / number / binary_sensor /
  button), grouped under one device. Reuses the existing `<base>/<key>` topics.
- **Tabs example** — `dash.tab()` switchable panels (added to the CI matrix).

### Fixed
- Redrew the `clock`, `gauge` and `motion` built-in icons with clean MDI paths (clock filled solid
  before; motion was malformed).

### Docs
- Measured per-widget flash footprint on ESP32 (README) — replaces the earlier optimistic estimate.

## [0.2.0] — 2026-06-27

A design + structure release: the firmware now serves the same OKLCH "liquid glass" UI as the
web emulator, with multi-page layouts, an OS-style status bar and a global Settings modal.

### Redesign
- New **OKLCH liquid-glass** design system: translucent glass cards (`backdrop-filter` blur),
  vibrant gradient background, and accent-following SVG (gauge arc / chart line track the accent).

### Pages & chrome
- **Multi-page layouts** — `dash.layout("Name", RICON_*)` splits the dashboard into switchable
  pages; a swipe-up sheet of icon tiles (or the bottom handle) switches them.
- **iOS-style status bar** inside the page — timezone-aware clock, Wi-Fi/GSM/Bluetooth, battery.
- **Settings modal** (appbar gear) — Language / Theme / Accent, applied live and persisted
  per-browser. Replaces the old appbar language switcher + theme pill.

### New APIs
- `dash.timezone(minutes)` — status-bar clock and the portal's default timezone (saved to NVS).
- `dash.accent(0..4)` — default accent color: Aqua · Blue · Violet · Amber · Rose.

### Portal
- Redesigned Wi-Fi setup: networks show signal strength + a lock; a custom timezone picker;
  the chosen network and timezone are saved to NVS.

### Examples
- New **Layouts** example; **AllWidgets** regrouped by purpose. All four examples build for
  ESP32 and ESP8266 in CI.

[0.2.1]: https://github.com/ziyarago/RisalDash/releases/tag/v0.2.1
[0.2.0]: https://github.com/ziyarago/RisalDash/releases/tag/v0.2.0

## [0.1.0] — 2026-06-26

First public release.

### Widgets
- 25 types: metric, gauge, chart, stat, progress, badge, led, toggle, slider, button,
  number, select, radio, text, password, time, color, label, log, image, table, ai —
  plus layout (group, separator, tab) and `.span()` / `.icon()` modifiers.

### Connectivity & first boot
- Dashboard streamed over HTTP; live updates over WebSocket (changed values only).
- Offline-first first boot: SoftAP + captive portal + Wi-Fi provisioning, saved to NVS.

### Integrations
- REST (`/api/state`, `/api/set`), Prometheus (`/metrics`), optional MQTT,
  OTA firmware update, and an MCP manifest (`/api/mcp/manifest`) that exposes widgets as AI tools.

### i18n
- `dash.lang("en" | "ru" | "ar")` with a live appbar switcher; Arabic switches the layout to RTL.
- Compile-time language selection — only the languages you use reach flash (Zero-Waste).

### Sensor presets
- 17 quantity-based presets: bme280, bmp280, dht11/22, sht3x, ds18b20, bh1750, ccs811,
  ina219, acs712, pzem004t, hcsr04, vl53l0x, mq135, soil, mpu6050/9250.

### Verified
- Builds for ESP32 and ESP8266 in CI; every example compiled for both targets.

[0.1.0]: https://github.com/ziyarago/RisalDash/releases/tag/v0.1.0
