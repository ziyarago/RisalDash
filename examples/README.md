# RisalDash examples

Numbered folders = the learning path. Every sketch is self-contained; most serve over a
plain access point (connect to `RisalDash-Demo`, password `12345678`, open
`http://192.168.4.1/`). None of them need extra hardware unless the folder says so.

## 01_Start — first contact
| Sketch | What it shows |
|---|---|
| **Minimal** | The smallest useful dashboard: 4 widgets over an AP, live in ~20 lines. |
| **FirstBoot** | `dash.begin()` provisioning: captive portal → pick Wi-Fi → dashboard on your network. |
| **WidgetSizes** | The cell grid: the same reading at `RSIZE_S/M/L`, compact gauges, preset resizing. |
| **WifiSetup** | The provisioning circle: portal → STA, live connection info, and `dash.forgetWiFi()` back to the portal. |

## 02_Widgets — the widget set
| Sketch | What it shows |
|---|---|
| **AllWidgets** | Every widget type across five swipe pages — gauges to robot face, map, 3D cube, terminal, thermal heatmap. |
| **SensorTemplates** | `dash.sensor("bme280", …)` presets, tuned with `.chart()` / `.size()`. |
| **FakeSensors** | The whole `<RisalFake.h>` toolbox: day/night greenhouse, GPS route playback, fake BLE scan, record-&-replay — no hardware attached. |

## 03_Pages — structure & language
| Sketch | What it shows |
|---|---|
| **Layouts** | Multi-page dashboard with the swipe-up page switcher. |
| **Tabs** | Switchable tab panels with cards pinned above them. |
| **Multilingual** | `dash.translate()`: the whole UI, your titles included, switches EN/RU/UZ/AR. |
| **UiKit** | The interface skeleton: pages, groups, separators, sizes, icons, `.gear()`, and every chrome knob (theme/accent/language/status bar). |

## 04_Templates — copy, bind pins, ship
| Sketch | What it shows |
|---|---|
| **Greenhouse** | Climate + soil + pump control, ready to wire to a BME280 and relays. |
| **EnergyMonitor** | Live V/A/W, kWh + cost, mains toggle — bind a PZEM-004T or INA219. |

## 05_Boards — board showcases (specific hardware)
| Sketch | Board | What it shows |
|---|---|---|
| **ESP32-C6-LCD-1_47** | Waveshare ESP32-C6-LCD-1.47 | Web dashboard + a 13-slide LCD carousel, real BLE scan, live weather, robot eyes. Split into `display.h` / `led.h` / `i18n.h` / `sensors.h` / `weather.h` / `ble.h`. |
| **ESP32-S3-DualCore** | Any dual-core ESP32 | A heavy worker on core 0 while the dashboard stays smooth on core 1. |
| **Wemos-D1-Mini** | Wemos/LOLIN D1 mini (ESP8266) | All-real-hardware: onboard LED toggle + blink, A0 gauge, live heap/RSSI — the streaming renderer at home on 80 KB of RAM. |
