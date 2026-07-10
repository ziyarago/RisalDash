// Control a Tasmota device (e.g. a reflashed SONOFF Basic) from a RisalDash dashboard — no cloud, no
// broker, just plain HTTP on your LAN. A toggle drives the real relay via Tasmota's command API
// (http://<ip>/cm?cmnd=Power%20Toggle); a 3 s poll reads the relay back so the UI always matches the
// physical state, even if someone flips it in eWeLink/Tasmota or by the wall button.
//
// Runs on ESP32 / ESP32-C6 (this sketch targets the C6). The board joins YOUR Wi-Fi (station mode) so
// it sits on the same LAN as the Sonoff. Add more devices by copying the pattern per IP.
#include <RisalUI.h>
#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#define WIFI_SSID   "your-wifi"
#define WIFI_PASS   "your-pass"
#define TASMOTA_IP  "192.168.8.140"   // the Sonoff Basic (running Tasmota) on your LAN

RisalUI dash("Sonoff Control");
bool relay = false, online = false;

// Send a Tasmota command, return its JSON response ("" on failure). Updates the reachability flag.
String tasmota(const String& cmnd) {
#if defined(ESP32)
  if (WiFi.status() != WL_CONNECTED) { online = false; return ""; }
  HTTPClient http;
  http.begin(String("http://") + TASMOTA_IP + "/cm?cmnd=" + cmnd);
  http.setTimeout(1500);
  int code = http.GET();
  String body = (code == 200) ? http.getString() : "";
  http.end();
  online = (code == 200);
  return body;
#else
  return "";
#endif
}

void setup() {
  dash.timezone(300);

  dash.layout("Devices", RICON_HOME);
  dash.toggle("Sonoff Basic", &relay, [](bool on) {
    tasmota(on ? "Power%20On" : "Power%20Off");   // drive the real relay
  });
  dash.led("Online", &online);

  dash.begin(WIFI_SSID, WIFI_PASS);   // station mode — same LAN as the Sonoff
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 3000) {       // keep the UI in sync with the physical state
    last = millis();
    String r = tasmota("Power");      // -> {"POWER":"ON"} / {"POWER":"OFF"}
    if (r.indexOf("\"ON\"") >= 0) relay = true;
    else if (r.indexOf("\"OFF\"") >= 0) relay = false;
  }
}
