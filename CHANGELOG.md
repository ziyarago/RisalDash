# Changelog

All notable changes to RisalDash are documented here. The format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions follow [semver](https://semver.org/).

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
