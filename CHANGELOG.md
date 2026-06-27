# Changelog

All notable changes to RisalDash are documented here. The format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions follow [semver](https://semver.org/).

## [Unreleased]

### Added
- **Home Assistant MQTT discovery** — `dash.enableHomeAssistant("node")` (after `mqtt()`) publishes
  retained discovery configs so HA auto-creates entities (sensor / switch / number / binary_sensor /
  button), grouped under one device. Reuses the existing `<base>/<key>` topics.

### Fixed
- Redrew the `clock`, `gauge` and `motion` built-in icons with clean MDI paths (clock filled solid
  before; motion was malformed).

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
