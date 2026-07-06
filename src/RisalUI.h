#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "RisalWidget.h"

// Optional MQTT export — opt in with -D RISAL_ENABLE_MQTT and add the PubSubClient
// dependency. Kept out of the default build so it never costs flash unless used.
#ifdef RISAL_ENABLE_MQTT
  #if defined(ESP32)
    #include <WiFi.h>
  #else
    #include <ESP8266WiFi.h>
  #endif
  #include <PubSubClient.h>
#endif

// RisalDash — beautiful real-time web dashboards for ESP32/ESP8266.
// https://dash.risal.io
//
// v0.1: declare widgets, serve the branded dashboard over a streamed HTTP response.
// Each widget type's CSS is emitted once (Zero-Waste). Live WebSocket updates (P3),
// the full widget set + sensor presets (P2), and AP captive-portal provisioning (P4)
// follow — all prototyped in dev/.
#define RISALDASH_VERSION "0.4.0"

#ifndef RISAL_MAX_WIDGETS
#define RISAL_MAX_WIDGETS 32
#endif

class RisalUI {
 public:
  enum Theme { DARK, LIGHT, AUTO };

  explicit RisalUI(const char* title);

  // First-boot flow: use saved Wi-Fi creds (NVS) if present; otherwise raise an AP
  // with a captive portal so the user can pick a network. Falls back to the portal
  // if a saved network can't be joined.
  void begin();
  void begin(const char* ssid, const char* pass);             // STA with given creds (fallback: portal)
  void beginAP(const char* ssid, const char* pass = nullptr);  // dashboard over a plain access point
  RisalUI& theme(Theme t);
  RisalUI& apName(const char* name) { _apSsid = name; return *this; }  // portal AP name
  // UI language: "en" (default), "ru", "ar". "ar" switches the layout to RTL.
  RisalUI& lang(const char* code) { _langCode = code; _rtl = (code[0] == 'a' && code[1] == 'r'); return *this; }
  // Visual effects (orbs + appbar backdrop-blur) — on by default. effects(false) flattens
  // them for weak boards / lower GPU load, keeping the same colors.
  RisalUI& effects(bool on) { _effects = on; return *this; }
  // Status-bar timezone, in minutes from UTC (e.g. 180 = +03:00). Default Riyadh.
  // Normally chosen on the first-boot portal and saved; this sets the default.
  RisalUI& timezone(int minutes) { _tz = minutes; return *this; }
  // Default accent: 0 Aqua · 1 Blue · 2 Violet · 3 Amber · 4 Rose. The user can change it
  // live in Settings (persists per-browser); this sets the out-of-the-box color.
  RisalUI& accent(int idx) { _accent = idx; return *this; }
  // Bind a real battery percentage (0-100) to the status-bar indicator — e.g. from a LiPo on a
  // voltage divider: bat = map(analogRead(A0), rawEmpty, rawFull, 0, 100). Without this the bar
  // shows a cosmetic (simulated) battery. Users can hide the indicator in Settings either way.
  RisalUI& battery(int* pct) { _battery = pct; return *this; }
  // Expose widgets to an AI agent via MCP: GET /api/mcp/manifest lists each widget as a
  // get_*/set_* tool (the risal-mcp-server bridge reads it). Token-guarded.
  RisalUI& enableMCP(const char* token) { _mcpToken = token; return *this; }
  // Over-the-air firmware update at GET/POST /update. Call before begin().
  RisalUI& enableOTA() { _ota = true; return *this; }
#ifdef RISAL_ENABLE_MQTT
  // Mirror widget state to MQTT: publishes <baseTopic>/<key> (retained) on change and
  // subscribes to <baseTopic>/<key>/set for inbound commands. STA mode only.
  RisalUI& mqtt(const char* host, uint16_t port = 1883, const char* baseTopic = "risaldash");
  // Home Assistant MQTT discovery: on connect, publish a retained config for each widget to
  // homeassistant/<component>/<nodeId>/<key>/config, so HA auto-creates entities (sensors,
  // switches, numbers, buttons). Requires mqtt(); HA must use the default discovery prefix.
  RisalUI& enableHomeAssistant(const char* nodeId = "risaldash") { _ha = true; _haNode = nodeId; return *this; }
#endif

  // Widget factories — return the concrete widget so config chains (e.g. .decimals(1)).
  MetricWidget& metric(const char* name, float* val, const char* unit = "");
  GaugeWidget&  gauge(const char* name, float* val, float mn, float mx, const char* unit = "");
  ToggleWidget& toggle(const char* name, bool* val, ToggleWidget::Cb cb = nullptr);
  SliderWidget& slider(const char* name, int* val, int mn, int mx, SliderWidget::Cb cb = nullptr);
  ButtonWidget& button(const char* name, const char* label, ButtonWidget::Cb cb);
  BadgeWidget&  badge(const char* name, int* val);
  LedWidget&    led(const char* name, bool* val);
  ProgressWidget& progress(const char* name, int* val, const char* unit = "%");
  StatWidget&   stat(const char* name, float* val, const char* unit = "");
  ChartWidget&  chart(const char* name, float* val, const char* unit = "");
  // Animated robot face — two glowing eyes that blink and change emotion. `mood` is 0..6
  // (Neutral/Happy/Sad/Angry/Surprised/Sleepy/Love); set it from your logic or an AI agent.
  FaceWidget&   face(const char* name, int* mood);
  // Live map (Leaflet) — a marker + trail following the bound lat/lon. NEEDS INTERNET on the client
  // (Leaflet + dark CARTO tiles load from a CDN), so it's an opt-in online widget.
  MapWidget&    map(const char* name, float* lat, float* lon);
  // 3D orientation cube — a CSS cube that rotates with the bound pitch/roll/yaw (degrees), e.g. from
  // an IMU (MPU6050/9250). Pure client-side; no external requests.
  CubeWidget&   cube(const char* name, float* pitch, float* roll, float* yaw);
  // Terminal / console — a scrolling output (.print(line)) plus a command input; the callback fires
  // with each entered command over the WebSocket.
  TerminalWidget& terminal(const char* name, TerminalWidget::Cb cb);
  // Heatmap / thermal camera — a cols x rows grid of colours (e.g. MLX90640 32x24). Push frames with
  // heat.frame(temps); auto-scaled to each frame's min..max.
  HeatmapWidget& heatmap(const char* name, uint8_t cols, uint8_t rows);
  NumberWidget& number(const char* name, int* val, int mn, int mx, int step = 1, NumberWidget::Cb cb = nullptr);
  SelectWidget& select(const char* name, const char* csvOptions, int* idx, SelectWidget::Cb cb = nullptr);
  GroupWidget&  group(const char* title);
  // Start a new switchable page. Widgets declared after it belong to this layout; a swipe-up
  // sheet lets the user switch pages. icon = a RICON_* glyph for the sheet tile.
  LayoutWidget& layout(const char* name, const char* icon = nullptr);
  LabelWidget&  label(const char* name, String* val);
  TextWidget&   text(const char* name, String* val, TextWidget::Cb cb = nullptr);
  LogWidget&    log(const char* name, uint8_t lines = 5);
  PasswordWidget& password(const char* name, String* val, PasswordWidget::Cb cb = nullptr);
  TimeWidget&   time(const char* name, String* val, TimeWidget::Cb cb = nullptr);
  DateWidget&   date(const char* name, String* val, DateWidget::Cb cb = nullptr);
  ColorWidget&  color(const char* name, String* val, ColorWidget::Cb cb = nullptr);
  ImageWidget&  image(const char* name, String* url);
  TableWidget&  table(const char* title);
  RadioWidget&  radio(const char* name, const char* csvOptions, int* idx, RadioWidget::Cb cb = nullptr);
  SeparatorWidget& separator(const char* title);
  TabWidget&    tab(const char* title);
  AiWidget&     ai(const char* name, String* note);

  // Sensor preset (quantity-based): expands a known sensor into the right widgets.
  // e.g. dash.sensor("bme280", &temp, &hum, &pres);
  void sensor(const char* id, float* p0 = nullptr, float* p1 = nullptr, float* p2 = nullptr, float* p3 = nullptr);

  // Push changed widget values to clients over the WebSocket (throttled). Call from loop().
  void update();
  RisalUI& interval(uint16_t ms) { _interval = ms; return *this; }

 private:
  void _startServer();
  void _handleRoot(AsyncWebServerRequest* req);
  // The page is streamed via a chunked response (a fill-callback re-renders through a windowing
  // Print sink) so it never needs one big RAM buffer — ESP8266 heap fragmentation caps a single
  // AsyncResponseStream buffer at ~15 KB and silently drops the rest, breaking large pages.
  void _renderRoot(Print& out, const char* eff, bool rtl, int active);
  void _onWs(AsyncWebSocketClient* client, AwsEventType type, uint8_t* data, size_t len);
  void _handleSet(AsyncWebServerRequest* req);
  void _handleMetrics(AsyncWebServerRequest* req);
  void _handleManifest(AsyncWebServerRequest* req);
  String _stateJson();
  void _add(Widget* w);
  // Provisioning (P4).
  bool _tryStation(const char* ssid, const char* pass, uint32_t timeoutMs);
  void _startPortal();
  void _handlePortal(AsyncWebServerRequest* req);
  void _renderPortal(Print& out);
  void _handleConnect(AsyncWebServerRequest* req);
  bool _loadCreds(String& ssid, String& pass);
  void _saveCreds(const char* ssid, const char* pass);
#ifdef RISAL_ENABLE_MQTT
  void _mqttLoop();
  void _mqttPublish(Widget* w);
  void _haDiscover(Widget* w);  // Home Assistant MQTT discovery config for one widget
  static void _mqttCb(char* topic, uint8_t* payload, unsigned int len);
  WiFiClient _mqttNet;
  PubSubClient _mqtt{_mqttNet};
  const char* _mqttHost = nullptr;
  uint16_t _mqttPort = 1883;
  const char* _mqttBase = "risaldash";
  bool _ha = false;
  const char* _haNode = "risaldash";
  uint32_t _mqttRetry = 0;
  static RisalUI* _self;
#endif

  const char* _title;
  Theme _theme = DARK;
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  DNSServer _dns;
  bool _running = false;
  bool _portal = false;
  const char* _apSsid = nullptr;
  const char* _mcpToken = nullptr;
  const char* _langCode = "en";
  bool _rtl = false;
  bool _effects = true;
  int _tz = 180;
  int _accent = 0;
  int* _battery = nullptr;  // bound real battery % (dash.battery); null -> cosmetic status-bar battery
  bool _ota = false;
  uint32_t _rebootAt = 0;
  int16_t _scanN = 0;  // Wi-Fi networks found once at portal start (cached; see _startPortal)
  Widget* _widgets[RISAL_MAX_WIDGETS];
  uint8_t _count = 0;
  uint32_t _lastPush = 0;
  uint16_t _interval = 250;  // min ms between WS pushes
};
