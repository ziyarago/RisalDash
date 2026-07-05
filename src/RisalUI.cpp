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
#if defined(ESP32)
  #include <Update.h>
#elif defined(ESP8266)
  #include <Updater.h>
#endif

// ── Saved Wi-Fi credentials (ESP32: NVS via Preferences; ESP8266: EEPROM) ──
bool RisalUI::_loadCreds(String& ssid, String& pass) {
#if defined(ESP32)
  Preferences pr;
  pr.begin("risaldash", true);
  ssid = pr.getString("ssid", "");
  pass = pr.getString("pass", "");
  _tz = pr.getInt("tz", _tz);  // portal-chosen timezone (falls back to the configured default)
  pr.end();
  return ssid.length() > 0;
#elif defined(ESP8266)
  struct { uint32_t magic; char s[33]; char p[65]; int tz; } c;
  EEPROM.begin(sizeof(c));
  EEPROM.get(0, c);
  EEPROM.end();
  if (c.magic != 0x52534C32UL) return false;
  c.s[32] = 0; c.p[64] = 0;
  ssid = c.s; pass = c.p; _tz = c.tz;
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
  pr.putInt("tz", _tz);
  pr.end();
#elif defined(ESP8266)
  struct { uint32_t magic; char s[33]; char p[65]; int tz; } c = {};
  c.magic = 0x52534C32UL;
  strncpy(c.s, ssid, 32);
  strncpy(c.p, pass ? pass : "", 64);
  c.tz = _tz;
  EEPROM.begin(sizeof(c));
  EEPROM.put(0, c);
  EEPROM.commit();
  EEPROM.end();
#else
  (void)ssid; (void)pass;
#endif
}

bool RisalUI::_tryStation(const char* ssid, const char* pass, uint32_t timeoutMs) {
  // We persist credentials ourselves (NVS). Stop the ESP8266 SDK from auto-connecting to a stale
  // cached AP (e.g. a neighbour's network it once saw) and clear any old STA config first, so only
  // the SSID the user just picked is used.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
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
  // Power pack
  {"acs712",  1, {{"Current","A",K_GAUGE,-30,30}}},
  {"pzem004t",4, {{"Voltage","V",K_GAUGE,0,260},{"Current","A",K_METRIC,0,100},{"Power","W",K_CHART,0,23000},{"Energy","kWh",K_METRIC,0,9999}}},
  // Distance / motion / gas pack
  {"vl53l0x", 1, {{"Distance","mm",K_METRIC,0,2000}}},
  {"mq135",   1, {{"Air quality","ppm",K_GAUGE,0,1000}}},
  {"soil",    1, {{"Soil moisture","%",K_GAUGE,0,100}}},
  // IMU pack
  {"mpu6050", 3, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2}}},
  {"mpu9250", 3, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2}}},
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
  // AP_STA so the one-time scan below can run on the station interface without tearing down the
  // access point. On ESP8266 a blocking scan inside the request handler (per page load) hops the
  // radio and drops the connected client — so we scan ONCE here, before any client connects, and
  // the portal handler renders that cached list. Networks are a snapshot from boot (reboot to
  // refresh); this keeps the AP rock-stable while the user provisions.
  WiFi.mode(WIFI_AP_STA);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(_apSsid ? _apSsid : "RisalDash-Setup");
  _scanN = WiFi.scanNetworks();  // cached; results persist (never scanDelete'd) for the portal
  _dns.start(53, "*", apIP);  // resolve every domain to us -> captive portal

  _server.on("/connect", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleConnect(req); });
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { _handlePortal(req); });
  // Any other path (OS captive-portal probes) -> show the setup page.
  _server.onNotFound([this](AsyncWebServerRequest* req) { _handlePortal(req); });
  _server.begin();
  _portal = true;
}

// Localized library chrome (portal + footer). Kept tiny; widget titles are the dev's own.
namespace {
enum RLoc { L_SUBTITLE, L_PASSWORD, L_CONNECT, L_SERVED, L_NETWORKS, L_TIMEZONE };
const char* rloc(RLoc k, const char* lang) {
  bool ru = lang[0] == 'r';
  bool ar = lang[0] == 'a';
  switch (k) {
    case L_SUBTITLE: return ar ? "إعداد Wi-Fi · اختر الشبكة" : ru ? "Настройка Wi-Fi · выберите сеть" : "Wi-Fi setup · choose your network";
    case L_PASSWORD: return ar ? "كلمة المرور" : ru ? "Пароль" : "Password";
    case L_CONNECT:  return ar ? "اتصال وحفظ" : ru ? "Подключить и сохранить" : "Connect & save";
    case L_SERVED:   return ar ? "يخدمها ESP" : ru ? "обслуживается ESP" : "served by ESP";
    case L_NETWORKS: return ar ? "الشبكات" : ru ? "Сети" : "Networks";
    case L_TIMEZONE: return ar ? "المنطقة الزمنية" : ru ? "Часовой пояс" : "Timezone";
  }
  return "";
}
}  // namespace

// Windowing Print sink for chunked streaming. The fill-callback re-runs the whole render on each
// call and this keeps only the bytes of the current window [start, start+max) — flat RAM, no big
// contiguous buffer to hit ESP8266 heap fragmentation (which capped AsyncResponseStream at ~15 KB
// and silently dropped the rest, breaking every page's trailing <script>).
namespace {
class WindowPrint : public Print {
  uint8_t* _buf;
  size_t _cap, _start, _pos = 0, _n = 0;
 public:
  WindowPrint(uint8_t* buf, size_t maxLen, size_t start) : _buf(buf), _cap(maxLen), _start(start) {}
  size_t produced() const { return _n; }
  size_t write(uint8_t c) override {
    if (_pos >= _start && _n < _cap) _buf[_n++] = c;
    _pos++;
    return 1;
  }
  size_t write(const uint8_t* data, size_t len) override {
    size_t segStart = _pos;
    _pos += len;
    if (_n >= _cap || segStart + len <= _start || segStart >= _start + _cap) return len;
    size_t from = segStart < _start ? _start - segStart : 0;
    size_t to = segStart + len < _start + _cap ? len : _start + _cap - segStart;
    size_t cpy = to - from;
    if (cpy > _cap - _n) cpy = _cap - _n;
    memcpy(_buf + _n, data + from, cpy);
    _n += cpy;
    return len;
  }
};
}  // namespace

void RisalUI::_renderPortal(Print& out) {
  int n = _scanN;  // cached at portal start — never scan inside the async handler (drops the AP)
  out.print(F("<!DOCTYPE html><html class=\""));
  out.print(_effects ? "dark" : "dark flat");
  out.print(F("\" lang=\""));
  out.print(_langCode);
  out.print(F("\" dir=\""));
  out.print(_rtl ? "rtl" : "ltr");
  out.print(F("\">"));
  out.print(FPSTR(RISAL_HEAD));
  out.print(FPSTR(RISAL_CSS));
  out.print(FPSTR(RISAL_PORTAL_CSS));
  out.print(F("</style></head><body><div class=\"wrap\"><div class=\"card pc\">"
               "<h2><b>Risal</b>Dash</h2><p class=\"s\">"));
  out.print(rloc(L_SUBTITLE, _langCode));
  out.print(F("</p><form action=\"/connect\" method=\"get\">"
               "<input type=\"hidden\" name=\"ssid\" id=\"ssid\" value=\"\"><div class=\"l\">"));
  out.print(rloc(L_NETWORKS, _langCode));
  out.print(F("</div><div class=\"nets\">"));
  // Order strongest-first (ESP8266 scan results aren't sorted) and drop hidden SSIDs. Nothing is
  // pre-selected — the user must tap a network, so a wrong default can't be submitted by accident.
  int order[24];
  int m = 0;
  for (int i = 0; i < n && m < (int)(sizeof(order) / sizeof(order[0])); i++)
    if (WiFi.SSID(i).length()) order[m++] = i;
  for (int a = 0; a < m; a++)
    for (int b = a + 1; b < m; b++)
      if (WiFi.RSSI(order[b]) > WiFi.RSSI(order[a])) { int t = order[a]; order[a] = order[b]; order[b] = t; }
  for (int k = 0; k < m; k++) {
    int i = order[k];
    int rssi = WiFi.RSSI(i);
    int lvl = rssi > -60 ? 3 : rssi > -72 ? 2 : 1;
#if defined(ESP32)
    bool open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
#else
    bool open = WiFi.encryptionType(i) == ENC_TYPE_NONE;
#endif
    out.print(F("<button type=\"button\" class=\"net\" data-s=\""));
    out.print(WiFi.SSID(i));
    out.print(F("\"><svg viewBox=\"0 0 24 24\"><path d=\"M4 11a13 13 0 0 1 16 0\""));
    if (lvl < 3) out.print(F(" opacity=\".28\""));
    out.print(F("/><path d=\"M7.5 14.5a8 8 0 0 1 9 0\""));
    if (lvl < 2) out.print(F(" opacity=\".28\""));
    out.print(F("/><path d=\"M11 18a3 3 0 0 1 2 0\"/></svg><span class=\"nm\">"));
    out.print(WiFi.SSID(i));
    out.print(F("</span>"));
    if (!open)
      out.print(F("<svg class=\"lock\" viewBox=\"0 0 24 24\"><rect x=\"5\" y=\"11\" width=\"14\" height=\"9\" rx=\"2\"/>"
                   "<path d=\"M8 11V8a4 4 0 0 1 8 0v3\"/></svg>"));
    out.print(F("<span class=\"check\"><svg viewBox=\"0 0 24 24\"><path d=\"M5 13l4 4 10-11\"/></svg></span></button>"));
  }
  out.print(F("</div><div class=\"pwrap\"><input class=\"inp\" id=\"pw\" type=\"password\" name=\"pass\" placeholder=\""));
  out.print(rloc(L_PASSWORD, _langCode));
  out.print(F("\" autocomplete=\"off\"><button type=\"button\" class=\"eye\" id=\"eye\" aria-label=\"show password\">"
               "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\">"
               "<path d=\"M2 12s3.6-7 10-7 10 7 10 7-3.6 7-10 7-10-7-10-7Z\"/><circle cx=\"12\" cy=\"12\" r=\"3\"/>"
               "<line class=\"sl\" x1=\"3.5\" y1=\"3.5\" x2=\"20.5\" y2=\"20.5\" style=\"display:none\"/></svg></button></div>"
               "<div class=\"l\">"));
  out.print(rloc(L_TIMEZONE, _langCode));
  out.print(F("</div><div class=\"tzc\" id=\"tzc\"><div class=\"tzsel\"><span id=\"tzval\">+03:00</span>"
               "<svg viewBox=\"0 0 24 24\"><path d=\"M6 9l6 6 6-6\"/></svg></div><div class=\"tzpop\" id=\"tzpop\"></div></div>"
               "<input type=\"hidden\" name=\"tz\" id=\"tzv\" value=\""));
  out.print(_tz);
  out.print(F("\"><button class=\"act\" type=\"submit\">"));
  out.print(rloc(L_CONNECT, _langCode));
  out.print(F("</button></form></div></div><script>"));
  out.print(FPSTR(RISAL_PORTAL_JS));
  out.print(F("</script></body></html>"));
}

void RisalUI::_handlePortal(AsyncWebServerRequest* req) {
  req->sendChunked("text/html", [this](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
    WindowPrint w(buf, maxLen, index);
    _renderPortal(w);
    return w.produced();
  });
}

void RisalUI::_handleConnect(AsyncWebServerRequest* req) {
  String ssid = req->hasParam("ssid") ? req->getParam("ssid")->value() : "";
  String pass = req->hasParam("pass") ? req->getParam("pass")->value() : "";
  if (!ssid.length()) { req->redirect("/"); return; }
  if (req->hasParam("tz")) _tz = req->getParam("tz")->value().toInt();  // persisted with the creds
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

FaceWidget& RisalUI::face(const char* name, int* mood) {
  FaceWidget* w = new FaceWidget(name, name, mood);
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

NumberWidget& RisalUI::number(const char* name, int* val, int mn, int mx, int step, NumberWidget::Cb cb) {
  NumberWidget* w = new NumberWidget(name, name, val, mn, mx, step, cb);
  _add(w);
  return *w;
}

SelectWidget& RisalUI::select(const char* name, const char* csvOptions, int* idx, SelectWidget::Cb cb) {
  SelectWidget* w = new SelectWidget(name, name, csvOptions, idx, cb);
  _add(w);
  return *w;
}

GroupWidget& RisalUI::group(const char* title) {
  GroupWidget* w = new GroupWidget(title);
  _add(w);
  return *w;
}

LayoutWidget& RisalUI::layout(const char* name, const char* icon) {
  LayoutWidget* w = new LayoutWidget(name);
  if (icon) w->icon(icon);
  _add(w);
  return *w;
}

LabelWidget& RisalUI::label(const char* name, String* val) {
  LabelWidget* w = new LabelWidget(name, name, val);
  _add(w);
  return *w;
}

TextWidget& RisalUI::text(const char* name, String* val, TextWidget::Cb cb) {
  TextWidget* w = new TextWidget(name, name, val, cb);
  _add(w);
  return *w;
}

LogWidget& RisalUI::log(const char* name, uint8_t lines) {
  LogWidget* w = new LogWidget(name, name, lines);
  _add(w);
  return *w;
}

PasswordWidget& RisalUI::password(const char* name, String* val, PasswordWidget::Cb cb) {
  PasswordWidget* w = new PasswordWidget(name, name, val, cb);
  _add(w);
  return *w;
}

TimeWidget& RisalUI::time(const char* name, String* val, TimeWidget::Cb cb) {
  TimeWidget* w = new TimeWidget(name, name, val, cb);
  _add(w);
  return *w;
}

DateWidget& RisalUI::date(const char* name, String* val, DateWidget::Cb cb) {
  DateWidget* w = new DateWidget(name, name, val, cb);
  _add(w);
  return *w;
}

ColorWidget& RisalUI::color(const char* name, String* val, ColorWidget::Cb cb) {
  ColorWidget* w = new ColorWidget(name, name, val, cb);
  _add(w);
  return *w;
}

ImageWidget& RisalUI::image(const char* name, String* url) {
  ImageWidget* w = new ImageWidget(name, name, url);
  _add(w);
  return *w;
}

TableWidget& RisalUI::table(const char* title) {
  TableWidget* w = new TableWidget(title, title);
  _add(w);
  return *w;
}

RadioWidget& RisalUI::radio(const char* name, const char* csvOptions, int* idx, RadioWidget::Cb cb) {
  RadioWidget* w = new RadioWidget(name, name, csvOptions, idx, cb);
  _add(w);
  return *w;
}

SeparatorWidget& RisalUI::separator(const char* title) {
  SeparatorWidget* w = new SeparatorWidget(title);
  _add(w);
  return *w;
}

TabWidget& RisalUI::tab(const char* title) {
  TabWidget* w = new TabWidget(title);
  _add(w);
  return *w;
}

AiWidget& RisalUI::ai(const char* name, String* note) {
  AiWidget* w = new AiWidget(name, name, note);
  _add(w);
  return *w;
}

// Full current state as JSON (shared by the WS connect snapshot and GET /api/state).
String RisalUI::_stateJson() {
  String s = "{";
  bool first = true;
  for (uint8_t i = 0; i < _count; i++) {
    if (!_widgets[i]->hasState()) continue;
    if (!first) s += ',';
    first = false;
    _widgets[i]->writeKV(s);
  }
  s += "}";
  return s;
}

// REST: apply one or more key=value params (query string or form body) to widgets.
void RisalUI::_handleSet(AsyncWebServerRequest* req) {
  size_t n = req->params();
  for (size_t i = 0; i < n; i++) {
    const AsyncWebParameter* p = req->getParam(i);
    for (uint8_t w = 0; w < _count; w++)
      if (p->name() == _widgets[w]->key()) { _widgets[w]->applyCommand(p->value()); break; }
  }
  req->send(200, "application/json", _stateJson());
}

// Prometheus exposition format — scrape the device straight into Grafana / Home Assistant.
// Derives metrics from the state JSON (numbers as-is, true/false -> 1/0).
void RisalUI::_handleMetrics(AsyncWebServerRequest* req) {
  String j = _stateJson();
  String out;
  int i = 1;
  while (i < (int)j.length()) {
    int k1 = j.indexOf('"', i);
    if (k1 < 0) break;
    int k2 = j.indexOf('"', k1 + 1);
    if (k2 < 0) break;
    String key = j.substring(k1 + 1, k2);
    int c = j.indexOf(':', k2);
    int e = j.indexOf(',', c);
    if (e < 0) e = j.indexOf('}', c);
    if (e < 0) e = j.length();
    String val = j.substring(c + 1, e);
    if (val == "true") val = "1";
    else if (val == "false") val = "0";
    String name = "risaldash_";
    for (uint16_t z = 0; z < key.length(); z++) {
      char ch = key[z];
      if (ch >= 'A' && ch <= 'Z') ch += 32;
      name += ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) ? ch : '_';
    }
    out += "# TYPE " + name + " gauge\n" + name + " " + val + "\n";
    i = e + 1;
  }
  req->send(200, "text/plain; version=0.0.4; charset=utf-8", out);
}

// MCP manifest: every widget becomes a get_* (read) and/or set_* (write) tool. The
// risal-mcp-server bridge reads this and proxies tool calls to /api/state and /api/set.
void RisalUI::_handleManifest(AsyncWebServerRequest* req) {
  if (!_mcpToken) { req->send(404); return; }
  String tok = req->hasParam("token") ? req->getParam("token")->value() : "";
  if (tok != _mcpToken) { req->send(403, "application/json", "{\"error\":\"unauthorized\"}"); return; }

  String s = "{\"device\":\"";
  s += rwJsonEsc(_title);
  s += "\",\"tools\":[";
  bool first = true;
  for (uint8_t i = 0; i < _count; i++) {
    const char* id = _widgets[i]->typeId();
    if (strcmp(id, "group") == 0) continue;
    const char* key = _widgets[i]->key();

    const char* type = "number";
    if (!strcmp(id, "toggle") || !strcmp(id, "led")) type = "bool";
    else if (!strcmp(id, "text") || !strcmp(id, "label") || !strcmp(id, "log")) type = "string";
    else if (!strcmp(id, "select")) type = "enum";
    else if (!strcmp(id, "button")) type = "action";

    bool readable = _widgets[i]->hasState();
    bool writable = !strcmp(id, "toggle") || !strcmp(id, "slider") || !strcmp(id, "button") ||
                    !strcmp(id, "number") || !strcmp(id, "select") || !strcmp(id, "text");

    String base;
    for (const char* c = key; *c; c++) {
      char ch = *c;
      if (ch >= 'A' && ch <= 'Z') ch += 32;
      base += ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) ? ch : '_';
    }
    String ek = rwJsonEsc(String(key));
    if (readable) {
      if (!first) s += ',';
      first = false;
      s += "{\"name\":\"get_" + base + "\",\"op\":\"read\",\"key\":\"" + ek + "\",\"type\":\"" + type + "\"}";
    }
    if (writable) {
      if (!first) s += ',';
      first = false;
      s += "{\"name\":\"set_" + base + "\",\"op\":\"write\",\"key\":\"" + ek + "\",\"type\":\"" + type + "\"}";
    }
  }
  s += "]}";
  req->send(200, "application/json", s);
}

void RisalUI::_startServer() {
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleRoot(req); });
  _server.on("/api/mcp/manifest", HTTP_GET, [this](AsyncWebServerRequest* req) { _handleManifest(req); });
  // REST API for integrations (Home Assistant, scripts, …).
  _server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", _stateJson());
  });
  _server.on("/api/set", HTTP_ANY, [this](AsyncWebServerRequest* req) { _handleSet(req); });
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

// Stream the page in segments (no giant String in RAM). CSS for each widget type is
// emitted once — Zero-Waste, since unused widget classes are dropped by the linker.
void RisalUI::_handleRoot(AsyncWebServerRequest* req) {
  // Effective language: ?lang= overrides the configured default (live appbar switcher).
  String want = req->hasParam("lang") ? req->getParam("lang")->value() : String(_langCode);
  const char* eff = "en";
  bool rtl = false;
  if (want == "ru") eff = "ru";
  else if (want == "ar") { eff = "ar"; rtl = true; }
  int active = req->hasParam("layout") ? req->getParam("layout")->value().toInt() : 0;
  req->sendChunked("text/html", [this, eff, rtl, active](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
    WindowPrint w(buf, maxLen, index);
    _renderRoot(w, eff, rtl, active);
    return w.produced();
  });
}

void RisalUI::_renderRoot(Print& out, const char* eff, bool rtl, int active) {
  out.print(F("<!DOCTYPE html><html class=\""));
  out.print(_effects ? "dark" : "dark flat");
  out.print(F("\" lang=\""));
  out.print(eff);
  out.print(F("\" dir=\""));
  out.print(rtl ? "rtl" : "ltr");
  out.print(F("\">"));
  out.print(FPSTR(RISAL_HEAD));
  out.print(FPSTR(RISAL_CSS));

  const char* seen[RISAL_MAX_WIDGETS];
  uint8_t sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* c = _widgets[i]->css();
    if (!c) continue;
    bool dup = false;
    for (uint8_t j = 0; j < sc; j++)
      if (seen[j] == c) { dup = true; break; }
    if (!dup) { seen[sc++] = c; out.print(FPSTR(c)); }
  }

  uint8_t groups = 0;
  for (uint8_t i = 0; i < _count; i++)
    if (strcmp(_widgets[i]->typeId(), "group") == 0) groups++;

  out.print(FPSTR(RISAL_BODY_OPEN));
  // Resolve theme before paint: saved choice → configured mode → prefers-color-scheme (AUTO).
  out.print(F("<script>(function(){var m='"));
  out.print(_theme == LIGHT ? "light" : _theme == AUTO ? "auto" : "dark");
  out.print(F("';var s=localStorage.getItem('rd-th')||m;"
               "var d=s==='dark'?true:s==='light'?false:matchMedia('(prefers-color-scheme:dark)').matches;"
               "var c=document.documentElement.classList;c.toggle('light',!d);c.toggle('dark',d);})();</script>"));
  out.print(FPSTR(RISAL_BODY_CHROME));
  out.print(_title);
  out.print(FPSTR(RISAL_BODY_MID));
  // Hamburger → opens the nav drawer (mobile only; only when there are groups to jump to).
  if (groups)
    out.print(F("<button class=\"burg\" onclick=\"R.openNav(true)\"><svg viewBox=\"0 0 24 24\" fill=\"none\" "
                "stroke=\"currentColor\" stroke-width=\"2\"><path d=\"M4 6h16M4 12h16M4 18h16\"/></svg></button>"));
  // Language / theme / accent now live in the Settings modal (appbar gear → RISAL_APPBAR_END).
  out.print(FPSTR(RISAL_APPBAR_END));
  // Nav drawer: scrim + off-canvas list of groups (anchors to each section header).
  if (groups) {
    out.print(F("<div class=\"scrim\" onclick=\"R.openNav(false)\"></div><aside class=\"drawer\"><h4>"));
    out.print(_title);
    out.print(F("</h4>"));
    for (uint8_t i = 0; i < _count; i++) {
      if (strcmp(_widgets[i]->typeId(), "group") != 0) continue;
      out.print(F("<a href=\"#g-"));
      rwSlug(out, _widgets[i]->key());
      out.print(F("\" onclick=\"R.openNav(false)\">"));
      out.print(_widgets[i]->key());
      out.print(F("</a>"));
    }
    out.print(F("</aside>"));
  }
  out.print(FPSTR(RISAL_DEFS));

  bool hasTabs = false;
  uint8_t layCount = 0;
  for (uint8_t i = 0; i < _count; i++) {
    if (strcmp(_widgets[i]->typeId(), "tab") == 0) hasTabs = true;
    if (strcmp(_widgets[i]->typeId(), "layout") == 0) layCount++;
  }

  if (_count == 0) {
    out.print(F("<main class=\"grid\">"));
    out.print(FPSTR(RISAL_EMPTY));
    out.print(F("</main>"));
  } else if (layCount > 0) {
    // Multi-page mode: swipe/arrow between pages. `active` (the selected page) is passed in — the
    // request object isn't available during chunked rendering.
    // Nav strip under the appbar: [‹]  PAGE NAME  [›]  (name taps open the tile sheet).
    out.print(F("<nav class=\"lnav\"><button class=\"lnav-a\" id=\"lnavL\" aria-label=\"prev\" onclick=\"RL.go(RL.cur()-1)\">"
                 "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2.4\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M15 18l-6-6 6-6\"/></svg></button>"
                 "<button class=\"lnav-name\" id=\"lnavN\" onclick=\"RL.open(1)\"></button>"
                 "<button class=\"lnav-a\" id=\"lnavR\" aria-label=\"next\" onclick=\"RL.go(RL.cur()+1)\">"
                 "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2.4\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M9 6l6 6-6 6\"/></svg></button></nav>"));
    uint8_t i = 0;
    // Cards before the first layout() are pinned (visible on every page).
    if (strcmp(_widgets[0]->typeId(), "layout") != 0) {
      out.print(F("<main class=\"grid\">"));
      for (; i < _count && strcmp(_widgets[i]->typeId(), "layout") != 0; i++)
        if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
      out.print(F("</main>"));
    }
    out.print(F("<div class=\"lays\" data-active=\""));
    out.print(active);
    out.print(F("\">"));
    int li = -1;
    bool open = false;
    for (; i < _count; i++) {
      if (strcmp(_widgets[i]->typeId(), "layout") == 0) {
        if (open) out.print(F("</main>"));
        li++;
        out.print(F("<main class=\"grid lay\" data-lay=\""));
        out.print(li);
        out.print(F("\">"));
        open = true;
      } else if (!_widgets[i]->isSetting()) {
        _widgets[i]->card(out);
      }
    }
    if (open) out.print(F("</main>"));
    out.print(F("</div>"));
    // Tile sheet (opened by tapping the page name in the nav strip) + its scrim.
    out.print(F("<div class=\"lscrim\" onclick=\"RL.open(0)\"></div><div class=\"lsheet\"><div class=\"lgrip\"></div>"
                 "<div class=\"ltiles\">"));
    li = -1;
    for (uint8_t k = 0; k < _count; k++) {
      if (strcmp(_widgets[k]->typeId(), "layout") != 0) continue;
      li++;
      LayoutWidget* lw = static_cast<LayoutWidget*>(_widgets[k]);
      out.print(F("<button class=\"ltile"));
      if (li == active) out.print(F(" on"));
      out.print(F("\" data-lay=\""));
      out.print(li);
      out.print(F("\"><svg viewBox=\"0 0 24 24\"><path d=\""));
      out.print(FPSTR(lw->iconPath() ? lw->iconPath() : RICON_HOME));
      out.print(F("\"/></svg><span>"));
      out.print(_widgets[k]->key());
      out.print(F("</span></button>"));
    }
    out.print(F("</div></div>"));
  } else if (!hasTabs) {
    out.print(F("<main class=\"grid\">"));
    for (uint8_t i = 0; i < _count; i++)
      if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
    out.print(F("</main>"));
  } else {
    // Cards before the first tab are always visible.
    uint8_t i = 0;
    out.print(F("<main class=\"grid\">"));
    for (; i < _count && strcmp(_widgets[i]->typeId(), "tab") != 0; i++)
      if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
    out.print(F("</main>"));
    // Tab bar.
    out.print(F("<div class=\"tabbar\">"));
    uint8_t ti = 0;
    for (uint8_t j = i; j < _count; j++)
      if (strcmp(_widgets[j]->typeId(), "tab") == 0) {
        out.print(F("<button class=\"tabbtn"));
        if (ti == 0) out.print(F(" on"));
        out.print(F("\" data-tab=\""));
        out.print(ti);
        out.print(F("\">"));
        out.print(_widgets[j]->key());
        out.print(F("</button>"));
        ti++;
      }
    out.print(F("</div>"));
    // One switchable panel per tab.
    ti = 0;
    uint8_t j = i;
    while (j < _count) {
      out.print(F("<main class=\"grid tabpanel"));
      if (ti == 0) out.print(F(" on"));
      out.print(F("\" data-panel=\""));
      out.print(ti);
      out.print(F("\">"));
      j++;  // skip the tab marker
      for (; j < _count && strcmp(_widgets[j]->typeId(), "tab") != 0; j++)
        if (!_widgets[j]->isSetting()) _widgets[j]->card(out);
      out.print(F("</main>"));
      ti++;
    }
  }
  out.print(FPSTR(RISAL_BODY_FOOT));
  out.print(rloc(L_SERVED, eff));
  out.print(FPSTR(RISAL_FOOT_END));

  // Client runtime + each widget type's JS (once) + init.
  out.print(FPSTR(RISAL_SCRIPT_OPEN));
  out.print(FPSTR(RISAL_RUNTIME_JS));
  out.print(F("R.L={on:'On',off:'Off'};"));
  if (strcmp(eff, "ru") == 0) out.print(F("R.L.on='Вкл';R.L.off='Выкл';"));
  else if (strcmp(eff, "ar") == 0) out.print(F("R.L.on='تشغيل';R.L.off='إيقاف';"));
  out.print(F("R.openNav=function(o){var s=document.querySelector('.scrim'),d=document.querySelector('.drawer');"
              "if(s)s.classList.toggle('open',o);if(d)d.classList.toggle('open',o);};"));
  out.print(F("window.RSB_TZ="));
  out.print(_tz);
  out.print(F(";"));
  if (WiFi.isConnected()) {  // real link RSSI (dBm), shown next to the Wi-Fi icon; only in STA mode
    out.print(F("window.RSB_RSSI="));
    out.print(WiFi.RSSI());
    out.print(F(";"));
  }
  if (_battery) {  // real battery % (dash.battery) -> status bar shows it, no cosmetic drift
    out.print(F("window.RSB_BAT="));
    out.print(*_battery);
    out.print(F(";window.RSB_BAT_REAL=1;"));
  }
  out.print(FPSTR(RISAL_STATUSBAR_JS));
  // Settings modal: localized labels (window.RSL) + default accent (window.RSACC) + the JS.
  if (strcmp(eff, "ru") == 0)
    out.print(F("window.RSL={set:'Настройки',lang:'Язык',theme:'Тема',accent:'Акцент',"
                 "note:'Сохранено \\u00b7 на всех экранах',dark:'Тёмная',light:'Светлая',auto:'Авто',"
                 "signal:'Сигнал (dBm)',on:'Вкл',off:'Выкл',battery:'Батарея'};"));
  else if (strcmp(eff, "ar") == 0)
    out.print(F("window.RSL={set:'الإعدادات',lang:'اللغة',theme:'السمة',accent:'اللون',"
                 "note:'محفوظ \\u00b7 في كل الشاشات',dark:'داكن',light:'فاتح',auto:'تلقائي',"
                 "signal:'الإشارة (dBm)',on:'تشغيل',off:'إيقاف',battery:'البطارية'};"));
  out.print(F("window.RSACC="));
  out.print(_accent);
  out.print(F(";"));
  // Device settings: .gear() widgets are pulled out of the grid and listed in the Settings modal,
  // which reads their value from /api/state and writes via /api/set. (Toggles for now.)
  out.print(F("window.RSDEV=["));
  {
    bool first = true;
    for (uint8_t i = 0; i < _count; i++) {
      if (!_widgets[i]->isSetting() || strcmp(_widgets[i]->typeId(), "toggle") != 0) continue;
      if (!first) out.print(',');
      out.print(F("{n:\""));
      out.print(_widgets[i]->key());
      out.print(F("\",k:\""));
      out.print(_widgets[i]->key());
      out.print(F("\"}"));
      first = false;
    }
  }
  out.print(F("];"));
  out.print(FPSTR(RISAL_SETTINGS_JS));
  if (layCount > 0) out.print(FPSTR(RISAL_LAYOUTS_JS));
  sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* j = _widgets[i]->js();
    if (!j) continue;
    bool dup = false;
    for (uint8_t k = 0; k < sc; k++)
      if (seen[k] == j) { dup = true; break; }
    if (!dup) { seen[sc++] = j; out.print(FPSTR(j)); }
  }
  out.print(FPSTR(RISAL_INIT_JS));
  out.print(FPSTR(RISAL_HTML_END));
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
  s += "}";
  if (any) _ws.textAll(s);
  _ws.cleanupClients();
}

#ifdef RISAL_ENABLE_MQTT
RisalUI* RisalUI::_self = nullptr;

RisalUI& RisalUI::mqtt(const char* host, uint16_t port, const char* baseTopic) {
  _mqttHost = host;
  _mqttPort = port;
  _mqttBase = baseTopic;
  _self = this;
  return *this;
}

// Inbound: <base>/<key>/set -> applyCommand(payload).
void RisalUI::_mqttCb(char* topic, uint8_t* payload, unsigned int len) {
  if (!_self) return;
  String t(topic);
  String base = String(_self->_mqttBase) + "/";
  if (!t.startsWith(base) || !t.endsWith("/set")) return;
  String key = t.substring(base.length(), t.length() - 4);  // strip base.../ and /set
  String val;
  for (unsigned int i = 0; i < len; i++) val += (char)payload[i];
  for (uint8_t i = 0; i < _self->_count; i++)
    if (key == _self->_widgets[i]->key()) { _self->_widgets[i]->applyCommand(val); break; }
}

// Outbound: publish a widget's current value (retained) to <base>/<key>.
void RisalUI::_mqttPublish(Widget* w) {
  if (!w->hasState()) return;
  String kv;
  w->writeKV(kv);  // "key":value or "key":"value"
  int c = kv.indexOf(':');
  if (c < 0) return;
  String val = kv.substring(c + 1);
  if (val.length() && val[0] == '"') val = val.substring(1, val.length() - 1);  // unquote strings
  String topic = String(_mqttBase) + "/" + w->key();
  _mqtt.publish(topic.c_str(), val.c_str(), true);
}

void RisalUI::_mqttLoop() {
  if (!_mqttHost) return;
  if (_mqtt.connected()) { _mqtt.loop(); return; }
  if (millis() < _mqttRetry) return;
  _mqttRetry = millis() + 3000;  // throttle reconnects
  _mqtt.setServer(_mqttHost, _mqttPort);
  _mqtt.setCallback(_mqttCb);
  if (_ha) _mqtt.setBufferSize(640);  // HA discovery configs exceed the 256-byte default
  String cid = "risaldash-" + WiFi.macAddress();
  if (!_mqtt.connect(cid.c_str())) return;
  for (uint8_t i = 0; i < _count; i++) {
    if (_widgets[i]->hasState()) {
      String sub = String(_mqttBase) + "/" + _widgets[i]->key() + "/set";
      _mqtt.subscribe(sub.c_str());
      _mqttPublish(_widgets[i]);  // seed retained state
    }
    if (_ha) _haDiscover(_widgets[i]);  // HA auto-discovery (also subscribes stateless controls)
  }
}

// Home Assistant MQTT discovery for one widget: maps the widget type to an HA component and
// publishes a retained config to homeassistant/<component>/<node>/<slug>/config. State/command
// topics reuse the existing <base>/<key> mirror, so HA shares the same topics as the dashboard.
void RisalUI::_haDiscover(Widget* w) {
  const char* ty = w->typeId();
  const char* comp;
  bool cmd = false, isSwitch = false, isBinary = false, isButton = false;
  if (!strcmp(ty, "metric") || !strcmp(ty, "gauge") || !strcmp(ty, "stat") ||
      !strcmp(ty, "chart") || !strcmp(ty, "progress") || !strcmp(ty, "badge")) comp = "sensor";
  else if (!strcmp(ty, "toggle")) { comp = "switch"; cmd = true; isSwitch = true; }
  else if (!strcmp(ty, "led")) { comp = "binary_sensor"; isBinary = true; }
  else if (!strcmp(ty, "slider") || !strcmp(ty, "number")) { comp = "number"; cmd = true; }
  else if (!strcmp(ty, "button")) { comp = "button"; cmd = true; isButton = true; }
  else return;  // type without a clean HA mapping → skip

  String key = w->key();
  String slug;  // HA-safe id: lowercase alnum, others → '_'
  for (size_t i = 0; i < key.length(); i++) {
    char c = key[i];
    if (c >= 'A' && c <= 'Z') slug += (char)(c + 32);
    else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) slug += c;
    else slug += '_';
  }
  String base = String(_mqttBase) + "/" + key;  // raw key — matches _mqttPublish/_mqttCb

  String cfg = "{\"name\":\"";
  cfg += key;
  cfg += "\",\"uniq_id\":\"";
  cfg += _haNode; cfg += "_"; cfg += slug;
  cfg += "\",";
  if (isButton) {
    cfg += "\"cmd_t\":\""; cfg += base; cfg += "/set\",\"pl_prs\":\"true\",";
  } else {
    cfg += "\"stat_t\":\""; cfg += base; cfg += "\",";
    if (cmd) { cfg += "\"cmd_t\":\""; cfg += base; cfg += "/set\","; }
    if (isSwitch) cfg += "\"pl_on\":\"true\",\"pl_off\":\"false\",\"stat_on\":\"true\",\"stat_off\":\"false\",";
    if (isBinary) cfg += "\"pl_on\":\"true\",\"pl_off\":\"false\",";
  }
  cfg += "\"dev\":{\"ids\":[\""; cfg += _haNode;
  cfg += "\"],\"name\":\""; cfg += _title;
  cfg += "\",\"mf\":\"RisalDash\"}}";

  if (cmd) { String sub = base + "/set"; _mqtt.subscribe(sub.c_str()); }  // covers stateless button

  String topic = "homeassistant/";
  topic += comp; topic += "/"; topic += _haNode; topic += "/"; topic += slug; topic += "/config";
  _mqtt.publish(topic.c_str(), cfg.c_str(), true);  // retained
}
#endif
