#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "RisalWidget.h"

// RisalDash — beautiful real-time web dashboards for ESP32/ESP8266.
// https://dash.risal.io
//
// v0.1: declare widgets, serve the branded dashboard over a streamed HTTP response.
// Each widget type's CSS is emitted once (Zero-Waste). Live WebSocket updates (P3),
// the full widget set + sensor presets (P2), and AP captive-portal provisioning (P4)
// follow — all prototyped in dev/.
#define RISALDASH_VERSION "0.1.0"

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
  // Expose widgets to an AI agent via MCP: GET /api/mcp/manifest lists each widget as a
  // get_*/set_* tool (the risal-mcp-server bridge reads it). Token-guarded.
  RisalUI& enableMCP(const char* token) { _mcpToken = token; return *this; }
  // Over-the-air firmware update at GET/POST /update. Call before begin().
  RisalUI& enableOTA() { _ota = true; return *this; }

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
  NumberWidget& number(const char* name, int* val, int mn, int mx, int step = 1, NumberWidget::Cb cb = nullptr);
  SelectWidget& select(const char* name, const char* csvOptions, int* idx, SelectWidget::Cb cb = nullptr);
  GroupWidget&  group(const char* title);
  LabelWidget&  label(const char* name, String* val);
  TextWidget&   text(const char* name, String* val, TextWidget::Cb cb = nullptr);
  LogWidget&    log(const char* name, uint8_t lines = 5);

  // Sensor preset (quantity-based): expands a known sensor into the right widgets.
  // e.g. dash.sensor("bme280", &temp, &hum, &pres);
  void sensor(const char* id, float* p0 = nullptr, float* p1 = nullptr, float* p2 = nullptr, float* p3 = nullptr);

  // Push changed widget values to clients over the WebSocket (throttled). Call from loop().
  void update();
  RisalUI& interval(uint16_t ms) { _interval = ms; return *this; }

 private:
  void _startServer();
  void _handleRoot(AsyncWebServerRequest* req);
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
  void _handleConnect(AsyncWebServerRequest* req);
  bool _loadCreds(String& ssid, String& pass);
  void _saveCreds(const char* ssid, const char* pass);

  const char* _title;
  Theme _theme = DARK;
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  DNSServer _dns;
  bool _running = false;
  bool _portal = false;
  const char* _apSsid = nullptr;
  const char* _mcpToken = nullptr;
  bool _ota = false;
  uint32_t _rebootAt = 0;
  Widget* _widgets[RISAL_MAX_WIDGETS];
  uint8_t _count = 0;
  uint32_t _lastPush = 0;
  uint16_t _interval = 250;  // min ms between WS pushes
};
