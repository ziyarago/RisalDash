// Wi-Fi site survey — walk a site and map where the signal actually is. The board joins your network
// and reports LIVE RSSI (that part is real on any ESP!); a GPS position drops a marker on the map
// widget so every reading has a place. RisalFakeGPS replays a walk route here so the whole survey —
// moving marker + RSSI trend + weak-zone log — works on the bench; swap in TinyGPS++ (NEO-6M on
// Serial2) for a real outdoor survey, or just carry the board and watch RSSI room by room indoors.
// Set WIFI_SSID/PASS; falls back to the setup portal if they're wrong.
#include <RisalUI.h>
#include <RisalFake.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

const char *WIFI_SSID = "YOUR_WIFI";      // the network you're surveying
const char *WIFI_PASS = "YOUR_PASSWORD";

RisalUI dash("WiFi Survey");
RisalFakeGPS gps;

float rssi = -60, quality = 0, lat = 41.311, lon = 69.240, speed = 0;
int   grade = 0;                          // 0 excellent · 1 ok · 2 weak  -> badge
LogWidget* evlog = nullptr;

// A short walk route (Tashkent) as {lat, lon} pairs for the fake GPS.
const float route[] = {
  41.3110, 69.2401,  41.3155, 69.2490,  41.3120, 69.2585,
  41.3050, 69.2560,  41.3030, 69.2450,  41.3075, 69.2390,
};

void setup() {
  dash.timezone(300);

  dash.layout("Survey", RICON_WIFI);
  dash.map("Position", &lat, &lon);              // where this reading was taken
  dash.stat("RSSI", &rssi, "dBm");
  dash.gauge("Quality", &quality, 0, 100, "%").variant("bar");
  dash.badge("Signal", &grade).labels("Excellent", "OK", "WEAK");
  dash.chart("RSSI trend", &rssi, "dBm");        // the survey trace
  evlog = &dash.log("Weak zones", 5);

  dash.begin(WIFI_SSID, WIFI_PASS);              // STA — we're measuring THIS link
  gps.begin(route, sizeof(route) / sizeof(float) / 2);
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 500) {
    last = millis();
    gps.update();
    lat = gps.lat(); lon = gps.lon(); speed = gps.speed();  // real: TinyGPS++ feed

    // The real measurement: this board's link to the AP, right where it stands.
    long r = WiFi.RSSI();
    if (r != 0 && r > -127) rssi = r;
    // Map dBm to an intuitive 0-100 % (−50 dBm and better = 100, −100 dBm = 0).
    quality = rssi >= -50 ? 100 : (rssi <= -100 ? 0 : 2.0f * (rssi + 100));

    int g = rssi > -60 ? 0 : (rssi > -75 ? 1 : 2);
    if (g == 2 && grade != 2 && evlog)
      evlog->print("WEAK " + String((int)rssi) + " dBm @ " + String(lat, 4) + "," + String(lon, 4));
    grade = g;
  }
}
