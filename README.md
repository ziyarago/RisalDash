# RisalDash

**Beautiful real-time web dashboards for ESP32 / ESP8266 — in a few lines of C++.**

Describe widgets; RisalDash generates the HTML, CSS, JS and the WebSocket protocol for you.
The dashboard is served by the device itself in the Risal brand style (dark-first, glass,
cyan→emerald), updates live over WebSocket, and works **offline-first** — including a captive
portal for first-boot Wi-Fi setup. Zero front-end code.

🌐 **[dash.risal.io](https://dash.risal.io)** · MIT · ESP32 + ESP8266

```cpp
#include <RisalUI.h>
RisalUI dash("Greenhouse");

float temp = 24.3, volts = 12.1; int bright = 128; bool pump = false;

void setup() {
  dash.gauge ("Voltage", &volts, 0, 14, "V");
  dash.chart ("Temperature", &temp, "C");
  dash.slider("Brightness", &bright, 0, 255, [](int v){ analogWrite(LED_PIN, v); });
  dash.toggle("Pump", &pump, [](bool on){ digitalWrite(PUMP_PIN, on); });
  dash.begin();        // saved Wi-Fi → connect; first boot → captive setup portal
}

void loop() {
  temp = readTemp(); volts = readVolts();
  dash.update();       // pushes changed values to the browser
}
```

## Why RisalDash

- **Zero-Waste UI** — the linker (`--gc-sections`) strips widgets you don't use. Each widget
  type ships its own CSS/JS only when instantiated (~1–1.5 KB each).
- **Offline-first first boot** — `begin()` raises a Wi-Fi access point with a **captive portal**;
  the user picks a network and the credentials are saved to NVS. No internet, no app, no CDN
  (system fonts, everything served from flash).
- **Real-time** — values are pushed over WebSocket only when they change; controls send commands
  back to your callbacks.
- **Widgets for everything** — metric, gauge, chart, stat, progress, badge, led, toggle, slider,
  button (more on the way), plus one-line **sensor presets**.
- **Brand-consistent** — same Risal design system as the app and [dash.risal.io](https://dash.risal.io),
  dark/light with an in-UI toggle.

## Install

**Arduino IDE** — Library Manager → search **"RisalDash"**.

**PlatformIO** — `platformio.ini`:
```ini
lib_deps =
    RisalDash
    esp32async/ESPAsyncWebServer
    esp32async/AsyncTCP        ; ESP32
  ; esp32async/ESPAsyncTCP    ; ESP8266
```

## Wi-Fi: first boot vs. fixed credentials

```cpp
dash.begin();                          // saved creds → STA; otherwise captive setup portal
dash.begin("ssid", "password");        // connect to this network (falls back to the portal)
dash.beginAP("Greenhouse", "12345678");// plain dashboard over its own access point
dash.apName("Greenhouse-Setup");       // name of the captive-portal AP (optional)
```

On first boot the device appears as a `RisalDash-Setup` Wi-Fi. Connect to it — the setup page
opens automatically (captive portal). Pick your network, enter the password; the device reboots
and serves the dashboard on your Wi-Fi.

## Widgets

All widgets bind to a variable by pointer and update live.

| Method | Binds | Notes |
|---|---|---|
| `metric(name, &float, unit)` | `float*` | big number + bar; `.decimals(n)`, `.zone(warn, bad)` |
| `gauge(name, &float, min, max, unit)` | `float*` | circular gauge |
| `chart(name, &float, unit)` | `float*` | live sparkline (30-point history) |
| `stat(name, &float, unit)` | `float*` | read-only number; `.decimals(n)` |
| `progress(name, &int, unit)` | `int*` | 0–100 % bar |
| `badge(name, &int)` | `int*` | 0/1/2 → ok/warn/bad; `.labels(a, b, c)` |
| `led(name, &bool)` | `bool*` | on/off indicator |
| `toggle(name, &bool, cb)` | `bool*` | switch → `cb(bool)` |
| `slider(name, &int, min, max, cb)` | `int*` | range → `cb(int)` |
| `button(name, label, cb)` | — | momentary action → `cb()` |

## Sensor presets

One line drops the right widgets, units and ranges for a known sensor:

```cpp
dash.sensor("bme280", &temp, &hum, &pres);  // gauge °C + metric % + chart hPa
dash.sensor("ina219", &volts, &cur, &pwr);  // V / A / W
```

Built-in: `bme280`, `bmp280`, `dht11`, `dht22`, `sht3x`, `ds18b20`, `bh1750`, `ina219`,
`hcsr04`, `ccs811`. The widget is chosen by the **quantity**, not the sensor model.

## Examples

- **Minimal** — a few widgets over an access point.
- **FirstBoot** — captive-portal Wi-Fi provisioning, then the dashboard on your network.

## Footprint

Empty web core ≈ the ESPAsyncWebServer stack; each widget type adds ~1–1.5 KB flash. A typical
dashboard (a handful of widgets) is a few KB of RisalDash on top of the web stack.

## Roadmap

More controls (select, color, time, d-pad…), layout (tabs/groups), MDI icons, MQTT/Prometheus
export, OTA, and **MCP** (control the device from an AI agent — every widget becomes a tool).
See [dash.risal.io](https://dash.risal.io).

## License

MIT © Shaxzod Ahmedov. Brand: Risal.
