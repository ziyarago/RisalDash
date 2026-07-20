#include "RisalUI.h"
#include "RisalAssets.h"
#include "RisalInternal.h"
#include <string.h>

// Author-string translation hook (declared in RisalWidget.h). Off unless dash.translate() is set.
RTranslator g_rTranslator = nullptr;
const char* g_rActiveLang = "en";

RisalUI::RisalUI(const char* title) : _title(title) {}

void RisalUI::begin() {
  _loadPrefs();
  // First-boot flow: saved creds -> try STA -> dashboard; else -> captive portal.
  // Retry the join a few times before giving up — a single 15s attempt is flaky right after a soft
  // reset or when a second radio (BLE/802.15.4) shares the 2.4GHz front end, and one timeout should
  // not drop a configured device into setup mode.
  String ss, pw;
  bool joined = false;
  if (_loadCreds(ss, pw))
    for (uint8_t i = 0; i < 3 && !joined; i++) {
      joined = _tryStation(ss.c_str(), pw.c_str(), 12000);
      if (!joined) delay(500);
    }
  if (joined) _startServer();
  else _startPortal();
}

void RisalUI::begin(const char* ssid, const char* pass) {
  _loadPrefs();
  if (_tryStation(ssid, pass, 15000)) {
    _saveCreds(ssid, pass);
    _startServer();
  } else {
    _startPortal();  // couldn't join -> let the user reconfigure
  }
}

void RisalUI::beginAP(const char* ssid, const char* pass) {
  _loadPrefs();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);  // plain dashboard-over-AP (no provisioning portal)
  _startServer();
}

RisalUI& RisalUI::theme(Theme t) {
  _theme = t;
  return *this;
}

void RisalUI::_add(Widget* w) {
  if (_count < RISAL_MAX_WIDGETS) { _widgets[_count++] = w; _structRev++; }  // bump so clients reload
}

MetricWidget& RisalUI::metric(const char* name, float* val, const char* unit) { return _make<MetricWidget>(name, name, val, unit); }
GaugeWidget& RisalUI::gauge(const char* name, float* val, float mn, float mx, const char* unit) { return _make<GaugeWidget>(name, name, val, mn, mx, unit); }
ToggleWidget& RisalUI::toggle(const char* name, bool* val, ToggleWidget::Cb cb) { return _make<ToggleWidget>(name, name, val, cb); }
SliderWidget& RisalUI::slider(const char* name, int* val, int mn, int mx, SliderWidget::Cb cb) { return _make<SliderWidget>(name, name, val, mn, mx, cb); }
ButtonWidget& RisalUI::button(const char* name, const char* label, ButtonWidget::Cb cb) { return _make<ButtonWidget>(name, name, label, cb); }
LinkWidget& RisalUI::link(const char* name, const char* label, const char* url) { return _make<LinkWidget>(name, name, label, url); }
BadgeWidget& RisalUI::badge(const char* name, int* val) { return _make<BadgeWidget>(name, name, val); }
LedWidget& RisalUI::led(const char* name, bool* val) { return _make<LedWidget>(name, name, val); }
ProgressWidget& RisalUI::progress(const char* name, int* val, const char* unit) { return _make<ProgressWidget>(name, name, val, unit); }
FaceWidget& RisalUI::face(const char* name, int* mood) { return _make<FaceWidget>(name, name, mood); }
MapWidget& RisalUI::map(const char* name, float* lat, float* lon) { return _make<MapWidget>(name, name, lat, lon); }
CubeWidget& RisalUI::cube(const char* name, float* pitch, float* roll, float* yaw) { return _make<CubeWidget>(name, name, pitch, roll, yaw); }
TerminalWidget& RisalUI::terminal(const char* name, TerminalWidget::Cb cb) { return _make<TerminalWidget>(name, name, cb); }
HeatmapWidget& RisalUI::heatmap(const char* name, uint8_t cols, uint8_t rows) { return _make<HeatmapWidget>(name, name, cols, rows); }
StatWidget& RisalUI::stat(const char* name, float* val, const char* unit) { return _make<StatWidget>(name, name, val, unit); }
ChartWidget& RisalUI::chart(const char* name, float* val, const char* unit) { return _make<ChartWidget>(name, name, val, unit); }
NumberWidget& RisalUI::number(const char* name, int* val, int mn, int mx, int step, NumberWidget::Cb cb) { return _make<NumberWidget>(name, name, val, mn, mx, step, cb); }
SelectWidget& RisalUI::select(const char* name, const char* csvOptions, int* idx, SelectWidget::Cb cb) { return _make<SelectWidget>(name, name, csvOptions, idx, cb); }
GroupWidget& RisalUI::group(const char* title) { return _make<GroupWidget>(title); }
LayoutWidget& RisalUI::layout(const char* name, const char* icon) {
  LayoutWidget& w = _make<LayoutWidget>(name);
  if (icon) w.icon(icon);
  return w;
}
LabelWidget& RisalUI::label(const char* name, String* val) { return _make<LabelWidget>(name, name, val); }
TextWidget& RisalUI::text(const char* name, String* val, TextWidget::Cb cb) { return _make<TextWidget>(name, name, val, cb); }
TextareaWidget& RisalUI::textarea(const char* name, String* val, TextareaWidget::Cb cb) { return _make<TextareaWidget>(name, name, val, cb); }
RadioBrowserWidget& RisalUI::radioBrowser(const char* name, String* val, RadioBrowserWidget::Cb cb) { return _make<RadioBrowserWidget>(name, name, val, cb); }
DeviceCardWidget& RisalUI::deviceCard(const char* name, const char* emoji, bool* power, bool* online, DeviceCardWidget::PowerCb cb) { return _make<DeviceCardWidget>(name, name, emoji, power, online, cb); }
SummaryWidget& RisalUI::summary(const char* eyebrow, String* headline, String* detail, int* mood) { return _make<SummaryWidget>(eyebrow, eyebrow, headline, detail, mood); }
NetworkWidget& RisalUI::network(const char* name, String* data) { return _make<NetworkWidget>(name, name, data); }
DeviceTableWidget& RisalUI::deviceTable(const char* name, String* data) { return _make<DeviceTableWidget>(name, name, data); }
LogWidget& RisalUI::log(const char* name, uint8_t lines) { return _make<LogWidget>(name, name, lines); }
PasswordWidget& RisalUI::password(const char* name, String* val, PasswordWidget::Cb cb) { return _make<PasswordWidget>(name, name, val, cb); }
TimeWidget& RisalUI::time(const char* name, String* val, TimeWidget::Cb cb) { return _make<TimeWidget>(name, name, val, cb); }
DateWidget& RisalUI::date(const char* name, String* val, DateWidget::Cb cb) { return _make<DateWidget>(name, name, val, cb); }
ColorWidget& RisalUI::color(const char* name, String* val, ColorWidget::Cb cb) { return _make<ColorWidget>(name, name, val, cb); }
ImageWidget& RisalUI::image(const char* name, String* url) { return _make<ImageWidget>(name, name, url); }
TableWidget& RisalUI::table(const char* title) { return _make<TableWidget>(title, title); }
RadioWidget& RisalUI::radio(const char* name, const char* csvOptions, int* idx, RadioWidget::Cb cb) { return _make<RadioWidget>(name, name, csvOptions, idx, cb); }
SeparatorWidget& RisalUI::separator(const char* title) { return _make<SeparatorWidget>(title); }
TabWidget& RisalUI::tab(const char* title) { return _make<TabWidget>(title); }
AiWidget& RisalUI::ai(const char* name, String* note) { return _make<AiWidget>(name, name, note); }

// Full current state as JSON (shared by the WS connect snapshot and GET /api/state).
String RisalUI::_stateJson() {
  String s = "{\"_struct\":";  // structure revision first, so clients record the baseline on connect
  s += String(_structRev);
  for (uint8_t i = 0; i < _count; i++) {
    if (!_widgets[i]->hasState()) continue;
    s += ',';
    _widgets[i]->writeKV(s);
  }
  s += "}";
  return s;
}

void RisalUI::_startServer() {
  // No RTC battery: sync the real clock over NTP once we're online (offset = the chosen timezone).
  // Skipped in AP-only mode (no internet); SNTP fills the time in the background.
  if (WiFi.status() == WL_CONNECTED)
    configTime((long)_tz * 60, 0, "pool.ntp.org", "time.nist.gov");
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleRoot(req); });
  _server.on("/api/mcp/manifest", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleManifest(req); });
  // REST API for integrations (Home Assistant, scripts, …).
  _server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", _stateJson());
  });
  _server.on("/api/set", HTTP_ANY, [this](AsyncWebServerRequest* req) { _handleSet(req); });
  _server.on("/api/pref", HTTP_ANY, [this](AsyncWebServerRequest* req) { _handlePref(req); });
  // Settings → "Reset Wi-Fi": erase saved credentials and reboot into the captive setup portal.
  _server.on("/api/forget", HTTP_POST, [this](AsyncWebServerRequest* req) {
    req->send(200, "text/plain", "ok");
    forgetWiFi();  // clears creds + schedules the reboot (begin() then raises the portal)
  });
  _server.on("/metrics", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleMetrics(req); });

  if (_ota) {
    _server.on("/update", HTTP_GET, [](AsyncWebServerRequest* r) {
      r->send(200, "text/html",
              "<!DOCTYPE html><meta name=viewport content=\"width=device-width,initial-scale=1\">"
              "<body style=\"font-family:sans-serif;background:#0F1115;color:#F2F4F8;padding:32px\">"
              "<h2>OTA update</h2><form method=POST action=/update enctype=multipart/form-data>"
              "<input type=file name=firmware> <button>Upload</button></form></body>");
    });
    _server.on(
        "/update", HTTP_POST,
        [this](AsyncWebServerRequest* r) {
          bool ok = !Update.hasError();
          r->send(200, "text/plain", ok ? "OK, rebooting" : "update failed");
          if (ok) _rebootAt = millis() + 800;
        },
        [](AsyncWebServerRequest*, String, size_t index, uint8_t* data, size_t len, bool final) {
          if (index == 0) {
#if defined(ESP32)
            Update.begin(UPDATE_SIZE_UNKNOWN);
#else
            Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#endif
          }
          if (len) Update.write(data, len);
          if (final) Update.end(true);
        });
  }

  _ws.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
                     void*, uint8_t* data, size_t len) { _onWs(client, type, data, len); });
  _server.addHandler(&_ws);
  _server.begin();
  _running = true;
}

void RisalUI::_onWs(AsyncWebSocketClient* client, AwsEventType type, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Send the full current state so the new client paints live values immediately.
    client->text(_stateJson());
  } else if (type == WS_EVT_DATA) {
    // Minimal {"key":value} command parse → dispatch to the matching widget.
    String msg;
    msg.reserve(len + 1);
    for (size_t i = 0; i < len; i++) msg += (char)data[i];
    int k1 = msg.indexOf('"');
    if (k1 < 0) return;
    int k2 = msg.indexOf('"', k1 + 1);
    if (k2 < 0) return;
    String key = msg.substring(k1 + 1, k2);
    int co = msg.indexOf(':', k2);
    if (co < 0) return;
    int en = msg.indexOf('}', co);
    if (en < 0) en = msg.length();
    String val = msg.substring(co + 1, en);
    val.trim();
    if (val.length() && val[0] == '"') val = val.substring(1, val.length() - 1);
    for (uint8_t i = 0; i < _count; i++)
      if (key == _widgets[i]->key()) { _widgets[i]->applyCommand(val); break; }
  }
}

void RisalUI::update() {
  if (_rebootAt && millis() > _rebootAt) ESP.restart();  // pending reboot (provisioning / OTA)
  if (_portal) {
    _dns.processNextRequest();
    return;
  }
  if (!_running) return;
#ifdef RISAL_ENABLE_MQTT
  _mqttLoop();  // connection management + inbound, every call (not throttled)
#endif
  uint32_t now = millis();
  if (now - _lastPush < _interval) return;
  _lastPush = now;

  String s = "{";
  bool any = false;
  for (uint8_t i = 0; i < _count; i++) {
    if (!_widgets[i]->poll()) continue;
    if (any) s += ',';
    any = true;
    _widgets[i]->writeKV(s);
#ifdef RISAL_ENABLE_MQTT
    if (_mqttHost && _mqtt.connected()) _mqttPublish(_widgets[i]);
#endif
  }
  // Status-bar extras: live Wi-Fi RSSI (and the real battery, when bound) every ~5 s.
  // Reserved "_" keys are handled by the client runtime itself, not by widgets.
  // Structure changed (runtime widget added/discovered) -> tell open clients to reload for new tiles.
  if (_structRev != _lastStruct) {
    _lastStruct = _structRev;
    if (any) s += ',';
    s += "\"_struct\":";
    s += String(_structRev);
    any = true;
  }
  if (WiFi.status() == WL_CONNECTED && now - _lastSbPush > 5000) {
    _lastSbPush = now;
    if (any) s += ',';
    s += "\"_rssi\":";
    s += String(WiFi.RSSI());
    if (_battery) { s += ",\"_bat\":"; s += String(*_battery); }
    any = true;
  }
  s += "}";
  if (any) _ws.textAll(s);
  _ws.cleanupClients();
}

