// Provisioning & persistence: saved Wi-Fi credentials, UI preferences (NVS/EEPROM),
// the STA join helper and the first-boot captive portal.
#include "RisalUI.h"
#include "RisalAssets.h"
#include "RisalInternal.h"

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

// ── Saved UI preferences: theme / accent / language / timezone. ESP32 only (Preferences has
//    arbitrary keys); on ESP8266 these stay per-browser (localStorage) so the EEPROM creds layout
//    is left untouched. Loaded at boot as the served defaults; written by /api/pref. ──
void RisalUI::_loadPrefs() {
#if defined(ESP32)
  Preferences pr;
  pr.begin("risaldash", true);
  _theme = (Theme)pr.getInt("theme", (int)_theme);
  _accent = pr.getInt("accent", _accent);
  _tz = pr.getInt("tz", _tz);
  String lg = pr.getString("lang", "");
  pr.end();
  if (lg.length()) {  // point _langCode at stable storage (_langStore), like a saved default
    _langStore = lg;
    _langCode = _langStore.c_str();
    _rtl = (lg[0] == 'a' && lg[1] == 'r');
  }
#endif
}

// Persist a Settings choice to NVS and apply it in-memory (so the next render reflects it without a
// reboot). Called from the Settings modal (sendBeacon) for theme/accent/language.
void RisalUI::_handlePref(AsyncWebServerRequest* req) {
#if defined(ESP32)
  Preferences pr;
  pr.begin("risaldash", false);
  if (req->hasParam("theme")) {
    String t = req->getParam("theme")->value();
    _theme = t == "light" ? LIGHT : t == "auto" ? AUTO : DARK;
    pr.putInt("theme", (int)_theme);
  }
  if (req->hasParam("accent")) { _accent = req->getParam("accent")->value().toInt(); pr.putInt("accent", _accent); }
  if (req->hasParam("tz")) { _tz = req->getParam("tz")->value().toInt(); pr.putInt("tz", _tz); }
  if (req->hasParam("lang")) {
    String lg = req->getParam("lang")->value();
    if (lg == "en" || lg == "ru" || lg == "uz" || lg == "ar") {
      _langStore = lg;
      _langCode = _langStore.c_str();
      _rtl = (lg[0] == 'a' && lg[1] == 'r');
      pr.putString("lang", lg);
    }
  }
  pr.end();
#else
  (void)req;  // ESP8266: UI prefs stay client-side (localStorage)
#endif
  req->send(200, "text/plain", "ok");
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
  out.print(_title);
  out.print(FPSTR(RISAL_HEAD_END));
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

