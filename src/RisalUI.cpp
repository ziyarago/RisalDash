#include "RisalUI.h"
#include "RisalAssets.h"
#include <string.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <Preferences.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <EEPROM.h>
#endif

// ── Saved Wi-Fi credentials (ESP32: NVS via Preferences; ESP8266: EEPROM) ──
bool RisalUI::_loadCreds(String& ssid, String& pass) {
#if defined(ESP32)
  Preferences pr;
  pr.begin("risaldash", true);
  ssid = pr.getString("ssid", "");
  pass = pr.getString("pass", "");
  pr.end();
  return ssid.length() > 0;
#elif defined(ESP8266)
  struct { uint32_t magic; char s[33]; char p[65]; } c;
  EEPROM.begin(sizeof(c));
  EEPROM.get(0, c);
  EEPROM.end();
  if (c.magic != 0x52534C31UL) return false;
  c.s[32] = 0; c.p[64] = 0;
  ssid = c.s; pass = c.p;
  return ssid.length() > 0;
#else
  return false;
#endif
}

void RisalUI::_saveCreds(const char* ssid, const char* pass) {
#if defined(ESP32)
  Preferences pr;
  pr.begin("risaldash", false);
  pr.putString("ssid", ssid);
  pr.putString("pass", pass ? pass : "");
  pr.end();
#elif defined(ESP8266)
  struct { uint32_t magic; char s[33]; char p[65]; } c = {};
  c.magic = 0x52534C31UL;
  strncpy(c.s, ssid, 32);
  strncpy(c.p, pass ? pass : "", 64);
  EEPROM.begin(sizeof(c));
  EEPROM.put(0, c);
  EEPROM.commit();
  EEPROM.end();
#else
  (void)ssid; (void)pass;
#endif
}

bool RisalUI::_tryStation(const char* ssid, const char* pass, uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

// ── Sensor presets (quantity-based) ──
// A measure binds the right widget kind + unit + range for a physical quantity; a sensor
// is a composition of measures. dash.sensor("bme280", &t, &h, &p) expands to the proper
// widgets automatically. Curated common set; for an ultra-minimal build use widgets directly.
namespace {
enum RKind { K_METRIC, K_GAUGE, K_CHART };
struct RMeasure { const char* title; const char* unit; RKind kind; float lo; float hi; };
struct RSensor { const char* id; uint8_t n; RMeasure m[4]; };

const RSensor RSENSORS[] = {
  {"bme280", 3, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100},{"Pressure","hPa",K_CHART,300,1100}}},
  {"bmp280", 2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,85},{"Pressure","hPa",K_CHART,300,1100}}},
  {"dht22",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,80},{"Humidity","%",K_METRIC,0,100}}},
  {"dht11",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,0,50},{"Humidity","%",K_METRIC,20,90}}},
  {"sht3x",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,125},{"Humidity","%",K_METRIC,0,100}}},
  {"ds18b20",1, {{"Temperature","\xC2\xB0""C",K_GAUGE,-55,125}}},
  {"bh1750", 1, {{"Illuminance","lx",K_METRIC,0,65535}}},
  {"ina219", 3, {{"Voltage","V",K_GAUGE,0,26},{"Current","A",K_METRIC,0,3.2f},{"Power","W",K_CHART,0,80}}},
  {"hcsr04", 1, {{"Distance","cm",K_METRIC,0,400}}},
  {"ccs811", 2, {{"CO2","ppm",K_GAUGE,400,8000},{"TVOC","ppb",K_METRIC,0,1100}}},
};
const uint8_t RSENSOR_COUNT = sizeof(RSENSORS) / sizeof(RSENSORS[0]);
}  // namespace

RisalUI::RisalUI(const char* title) : _title(title) {}

void RisalUI::sensor(const char* id, float* p0, float* p1, float* p2, float* p3) {
  const RSensor* s = nullptr;
  for (uint8_t i = 0; i < RSENSOR_COUNT; i++)
    if (strcmp(id, RSENSORS[i].id) == 0) { s = &RSENSORS[i]; break; }
  if (!s) return;
  float* p[4] = {p0, p1, p2, p3};
  for (uint8_t i = 0; i < s->n && i < 4; i++) {
    const RMeasure& m = s->m[i];
    switch (m.kind) {
      case K_GAUGE: gauge(m.title, p[i], m.lo, m.hi, m.unit); break;
      case K_CHART: chart(m.title, p[i], m.unit); break;
      default:      metric(m.title, p[i], m.unit); break;
    }
  }
}

void RisalUI::begin() {
  // First-boot flow: saved creds -> try STA -> dashboard; else -> captive portal.
  String ss, pw;
  if (_loadCreds(ss, pw) && _tryStation(ss.c_str(), pw.c_str(), 15000)) {
    _startServer();
  } else {
    _startPortal();
  }
}

void RisalUI::begin(const char* ssid, const char* pass) {
  if (_tryStation(ssid, pass, 15000)) {
    _saveCreds(ssid, pass);
    _startServer();
  } else {
    _startPortal();  // couldn't join -> let the user reconfigure
  }
}

void RisalUI::beginAP(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);  // plain dashboard-over-AP (no provisioning portal)
  _startServer();
}

// Raise a SoftAP with a captive portal: catch-all DNS + a Wi-Fi setup page.
void RisalUI::_startPortal() {
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(_apSsid ? _apSsid : "RisalDash-Setup");
  _dns.start(53, "*", apIP);  // resolve every domain to us -> captive portal

  _server.on("/connect", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleConnect(req); });
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { _handlePortal(req); });
  // Any other path (OS captive-portal probes) -> show the setup page.
  _server.onNotFound([this](AsyncWebServerRequest* req) { _handlePortal(req); });
  _server.begin();
  _portal = true;
}

void RisalUI::_handlePortal(AsyncWebServerRequest* req) {
  int n = WiFi.scanNetworks();
  AsyncResponseStream* res = req->beginResponseStream("text/html");
  res->print(FPSTR(RISAL_HEAD));
  res->print(FPSTR(RISAL_CSS));
  res->print(FPSTR(RISAL_PORTAL_CSS));
  res->print(FPSTR(RISAL_PORTAL_OPEN));
  for (int i = 0; i < n; i++) {
    res->print(F("<option>"));
    res->print(WiFi.SSID(i));
    res->print(F("</option>"));
  }
  res->print(FPSTR(RISAL_PORTAL_CLOSE));
  req->send(res);
  WiFi.scanDelete();
}

void RisalUI::_handleConnect(AsyncWebServerRequest* req) {
  String ssid = req->hasParam("ssid") ? req->getParam("ssid")->value() : "";
  String pass = req->hasParam("pass") ? req->getParam("pass")->value() : "";
  if (!ssid.length()) { req->redirect("/"); return; }
  _saveCreds(ssid.c_str(), pass.c_str());
  String html = F("<!DOCTYPE html><meta name=viewport content=\"width=device-width,initial-scale=1\">"
                  "<body style=\"font-family:sans-serif;background:#0F1115;color:#F2F4F8;text-align:center;padding:48px\">"
                  "Saved. Rebooting to join <b>");
  html += ssid;
  html += F("</b>&hellip;</body>");
  req->send(200, "text/html", html);
  _rebootAt = millis() + 1500;  // restart shortly so begin() re-runs with the new creds
}

RisalUI& RisalUI::theme(Theme t) {
  _theme = t;
  return *this;
}

void RisalUI::_add(Widget* w) {
  if (_count < RISAL_MAX_WIDGETS) _widgets[_count++] = w;
}

MetricWidget& RisalUI::metric(const char* name, float* val, const char* unit) {
  MetricWidget* w = new MetricWidget(name, name, val, unit);
  _add(w);
  return *w;
}

GaugeWidget& RisalUI::gauge(const char* name, float* val, float mn, float mx, const char* unit) {
  GaugeWidget* w = new GaugeWidget(name, name, val, mn, mx, unit);
  _add(w);
  return *w;
}

ToggleWidget& RisalUI::toggle(const char* name, bool* val, ToggleWidget::Cb cb) {
  ToggleWidget* w = new ToggleWidget(name, name, val, cb);
  _add(w);
  return *w;
}

SliderWidget& RisalUI::slider(const char* name, int* val, int mn, int mx, SliderWidget::Cb cb) {
  SliderWidget* w = new SliderWidget(name, name, val, mn, mx, cb);
  _add(w);
  return *w;
}

ButtonWidget& RisalUI::button(const char* name, const char* label, ButtonWidget::Cb cb) {
  ButtonWidget* w = new ButtonWidget(name, name, label, cb);
  _add(w);
  return *w;
}

BadgeWidget& RisalUI::badge(const char* name, int* val) {
  BadgeWidget* w = new BadgeWidget(name, name, val);
  _add(w);
  return *w;
}

LedWidget& RisalUI::led(const char* name, bool* val) {
  LedWidget* w = new LedWidget(name, name, val);
  _add(w);
  return *w;
}

ProgressWidget& RisalUI::progress(const char* name, int* val, const char* unit) {
  ProgressWidget* w = new ProgressWidget(name, name, val, unit);
  _add(w);
  return *w;
}

StatWidget& RisalUI::stat(const char* name, float* val, const char* unit) {
  StatWidget* w = new StatWidget(name, name, val, unit);
  _add(w);
  return *w;
}

ChartWidget& RisalUI::chart(const char* name, float* val, const char* unit) {
  ChartWidget* w = new ChartWidget(name, name, val, unit);
  _add(w);
  return *w;
}

void RisalUI::_startServer() {
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleRoot(req); });
  _ws.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
                     void*, uint8_t* data, size_t len) { _onWs(client, type, data, len); });
  _server.addHandler(&_ws);
  _server.begin();
  _running = true;
}

void RisalUI::_onWs(AsyncWebSocketClient* client, AwsEventType type, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Send the full current state so the new client paints live values immediately.
    String s = "{";
    bool first = true;
    for (uint8_t i = 0; i < _count; i++) {
      if (!_widgets[i]->hasState()) continue;
      if (!first) s += ',';
      first = false;
      _widgets[i]->writeKV(s);
    }
    s += "}";
    client->text(s);
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

// Stream the page in segments (no giant String in RAM). CSS for each widget type is
// emitted once — Zero-Waste, since unused widget classes are dropped by the linker.
void RisalUI::_handleRoot(AsyncWebServerRequest* req) {
  AsyncResponseStream* res = req->beginResponseStream("text/html");
  res->print(FPSTR(RISAL_HEAD));
  res->print(FPSTR(RISAL_CSS));

  const char* seen[RISAL_MAX_WIDGETS];
  uint8_t sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* c = _widgets[i]->css();
    if (!c) continue;
    bool dup = false;
    for (uint8_t j = 0; j < sc; j++)
      if (seen[j] == c) { dup = true; break; }
    if (!dup) { seen[sc++] = c; res->print(FPSTR(c)); }
  }

  res->print(FPSTR(RISAL_BODY_OPEN));
  res->print(_title);
  res->print(FPSTR(RISAL_BODY_MID));
  res->print(FPSTR(RISAL_DEFS));
  if (_count == 0)
    res->print(FPSTR(RISAL_EMPTY));
  else
    for (uint8_t i = 0; i < _count; i++) _widgets[i]->card(*res);
  res->print(FPSTR(RISAL_BODY_FOOT));

  // Client runtime + each widget type's JS (once) + init.
  res->print(FPSTR(RISAL_SCRIPT_OPEN));
  res->print(FPSTR(RISAL_RUNTIME_JS));
  sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* j = _widgets[i]->js();
    if (!j) continue;
    bool dup = false;
    for (uint8_t k = 0; k < sc; k++)
      if (seen[k] == j) { dup = true; break; }
    if (!dup) { seen[sc++] = j; res->print(FPSTR(j)); }
  }
  res->print(FPSTR(RISAL_INIT_JS));
  res->print(FPSTR(RISAL_HTML_END));

  req->send(res);
}

void RisalUI::update() {
  if (_portal) {
    _dns.processNextRequest();
    if (_rebootAt && millis() > _rebootAt) ESP.restart();
    return;
  }
  if (!_running) return;
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
  }
  s += "}";
  if (any) _ws.textAll(s);
  _ws.cleanupClients();
}
