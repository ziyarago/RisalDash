// RisalDash GPS Tracker — a production-shaped vehicle/asset tracker on an ESP with a NEO-6M.
// Live map + speed + satellites on the dashboard, a home geofence with alarm, day-by-day CSV track
// logs (with GPX export), and hardware telemetry. Works on ESP32 (any) and ESP8266 (Wemos D1 mini).
//
// STORAGE — USE_SD 1 = SD card, USE_SD 0 = LittleFS in the controller's flash. Logging to flash is
// safe if done right: points are batched in a RAM buffer and written in one block per FLUSH_PERIOD
// (logLen is cleared only after a successful write, so a storage hiccup never loses data), a point is
// only written when actually moving (> LOG_MIN_MOVE_M, > MIN_SPEED_KMH, not faster than
// LOG_MIN_PERIOD), old day-files auto-purge below FS_MIN_FREE, and the flush period stretches to
// 5 min when parked. CSV (not packed binary) is chosen on purpose — human-readable, and the day-file
// footprint stays tiny.
//
// GPS NOISE — a cheap NEO-6M drifts 5-20 m near buildings. Guards: log only above MIN_SPEED_KMH
// (Doppler speed is quieter than position), the geofence alarm arms only after FENCE_DEBOUNCE fixes
// outside the zone, and clears with 10% hysteresis so it can't chatter on the boundary.
//
// TIME — ISO 8601 from the GPS (UTC). millis() is never used as a timestamp; before a date fix we
// write "boot+NNNs".
//
// ESP8266: set Tools -> Flash Size to a layout with a filesystem (e.g. "4MB (FS:1MB)"), or
// LittleFS.begin() fails. ESP32: pick a partition scheme that has a filesystem region (the default
// "Default 4MB with spiffs" is fine; a no-SPIFFS scheme like huge_app would fail LittleFS.begin()).
//
// WIRING                ESP32              ESP8266 (D1 mini)
//   NEO-6M TX            GPIO16 (RX2)       GPIO14 / D5 (SoftSerial RX)
//   NEO-6M RX            GPIO17 (TX2)       GPIO12 / D6 (SoftSerial TX)
//   ALERT                GPIO25             GPIO5  / D1
//   SD CS                GPIO5              GPIO15 / D8
//
// Libraries: RisalDash, TinyGPSPlus, ESPAsyncWebServer (pulled in by RisalDash).

// ======================= CONFIG =======================
#define USE_SD         0   // 1 = SD card, 0 = LittleFS (preprocessor flag — must be #define, not constexpr)
#define ALERT_MODE     1   // 0 off | 1 vibration | 2 LED | 3 active buzzer | 4 passive buzzer (tone)
#define GPS_POWERSAVE  0   // 1 = UBX Power-Save when parked (~30 mA saved)
#define TG_ENABLED     0   // 1 = Telegram alerts (needs Wi-Fi with internet)
#define USE_PORTAL     1   // 1 = join YOUR Wi-Fi via a captive setup portal (dashboard on your LAN);
                           // 0 = standalone — the tracker is its own AP "GPS-Tracker" @ 192.168.4.1

#if TG_ENABLED
  constexpr char WIFI_SSID[] = "HomeWiFi";
  constexpr char WIFI_PASS[] = "password";
  constexpr char TG_TOKEN[]  = "123456:ABC-DEF...";   // bot token
  constexpr char TG_CHAT[]   = "123456789";           // chat_id
#endif

#if defined(ESP32)
  constexpr uint8_t GPS_RX_PIN = 16;   // NEO-6M TX -> here (UART2 RX)
  constexpr uint8_t GPS_TX_PIN = 17;   // NEO-6M RX -> here (UART2 TX)
  constexpr uint8_t ALERT_PIN  = 25;
  constexpr uint8_t SD_CS_PIN  = 5;
#elif defined(ESP8266)
  constexpr uint8_t GPS_RX_PIN = 14;   // D5
  constexpr uint8_t GPS_TX_PIN = 12;   // D6
  constexpr uint8_t ALERT_PIN  = 5;    // D1
  constexpr uint8_t SD_CS_PIN  = 15;   // D8
#endif

constexpr float    MIN_SPEED_KMH   = 2.0f;      // parked-drift filter
constexpr float    LOG_MIN_MOVE_M  = 10.0f;
constexpr uint32_t LOG_MIN_PERIOD  = 5000UL;
constexpr uint32_t FLUSH_PERIOD    = 60000UL;   // buffer flush while moving
constexpr uint32_t FLUSH_PARKED    = 300000UL;  // ...and when parked (5 min)
constexpr uint32_t PARK_TIMEOUT    = 300000UL;  // parked = 5 min without movement
constexpr uint8_t  FENCE_DEBOUNCE  = 3;         // consecutive fixes outside the zone
constexpr uint32_t FS_MIN_FREE     = 100 * 1024;
constexpr uint16_t BUZZER_FREQ     = 2000;

// ======================= LIBRARIES =======================
#include <TinyGPSPlus.h>
#include <RisalDash.h>

#if defined(ESP32)
  #include <WiFi.h>
  HardwareSerial gpsSerial(2);
#else
  #include <ESP8266WiFi.h>
  #include <SoftwareSerial.h>
  SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
#endif

#if TG_ENABLED
  #if defined(ESP32)
    #include <HTTPClient.h>
  #else
    #include <ESP8266HTTPClient.h>
  #endif
  #include <WiFiClientSecure.h>
#endif

#if USE_SD
  #include <SPI.h>
  #include <SD.h>
  #define FS_IMPL SD
#else
  #include <LittleFS.h>
  #define FS_IMPL LittleFS
#endif

// ======================= STATE =======================
TinyGPSPlus gps;
RisalUI dash("GPS Tracker");
LogWidget* evlog = nullptr;                // event feed on the dashboard

float lat = 0, lon = 0, speedKmh = 0, altM = 0, hdop = 99;
int   sats = 0;
float distHome = 0;
int   fenceRadius = 85;                     // metres; driven by the dashboard slider
bool  fenceOn = true, alarmOn = false, homeSet = false, storageOk = false;
double homeLat = 0, homeLon = 0;
uint8_t outsideCount = 0;                   // geofence debounce

float freeHeapKb = 0, wifiRssi = 0;         // hardware telemetry

char   logBuf[1024];
size_t logLen = 0;
float  lastLogLat = 0, lastLogLon = 0;
unsigned long lastLogMs = 0, lastFlushMs = 0, lastMoveMs = 0, lastTelemMs = 0;

// ======================= TIME =======================
void isoTimestamp(char* out, size_t n) {
  if (gps.date.isValid() && gps.time.isValid() && gps.date.year() > 2020) {
    snprintf(out, n, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gps.date.year(), gps.date.month(), gps.date.day(),
             gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    snprintf(out, n, "boot+%lus", (unsigned long)(millis() / 1000));
  }
}

void todayFilename(char* buf, size_t n) {
  if (gps.date.isValid() && gps.date.year() > 2020) {
    snprintf(buf, n, "/%04d-%02d-%02d.csv",
             gps.date.year(), gps.date.month(), gps.date.day());
  } else {
    strncpy(buf, "/no_date.csv", n);
    buf[n - 1] = '\0';
  }
}

// ======================= STORAGE =======================
bool storageBegin() {
#if USE_SD
  return SD.begin(SD_CS_PIN);
#else
  #if defined(ESP32)
    return LittleFS.begin(true);
  #else
    return LittleFS.begin();
  #endif
#endif
}

template <typename CB>
void forEachCsv(CB cb) {
#if defined(ESP8266) && !USE_SD
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String n = "/" + dir.fileName();
    if (n.endsWith(".csv")) cb(n, dir.fileSize());
  }
#else
  File root = FS_IMPL.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    String n = String(f.name());
    if (!n.startsWith("/")) n = "/" + n;
    if (n.endsWith(".csv")) cb(n, (size_t)f.size());
    f.close();
  }
  root.close();
#endif
}

#if !USE_SD
size_t fsFreeBytes() {
  #if defined(ESP32)
    return LittleFS.totalBytes() - LittleFS.usedBytes();
  #else
    FSInfo i; LittleFS.info(i);
    return i.totalBytes - i.usedBytes;
  #endif
}

void cleanupOldLogs(const char* keep) {
  while (fsFreeBytes() < FS_MIN_FREE) {
    String oldest;
    forEachCsv([&](const String& n, size_t) {
      if (oldest.isEmpty() || n < oldest) oldest = n;
    });
    if (oldest.isEmpty() || oldest == keep) break;
    FS_IMPL.remove(oldest);
    Serial.printf("FS cleanup: removed %s\n", oldest.c_str());
  }
}
#endif

void flushLog() {
  if (!storageOk || logLen == 0) return;
  char fname[20];
  todayFilename(fname, sizeof(fname));
#if !USE_SD
  cleanupOldLogs(fname);
#endif
  File f = FS_IMPL.open(fname, "a");
  if (f) {
    f.write((uint8_t*)logBuf, logLen);
    f.close();
    logLen = 0;                            // only on a successful write
  }
}

void bufferPoint() {
  if (sizeof(logBuf) - logLen < 100) {
    flushLog();
    if (sizeof(logBuf) - logLen < 100) {   // storage dead and buffer full:
      logLen = 0;                          // drop the oldest rather than hang
      Serial.println("Log buffer dropped: storage unavailable");
    }
  }
  char ts[28];
  isoTimestamp(ts, sizeof(ts));
  int written = snprintf(logBuf + logLen, sizeof(logBuf) - logLen,
    "%s,%.6f,%.6f,%.1f,%.0f,%d\n", ts, lat, lon, speedKmh, altM, sats);
  if (written > 0 && (size_t)written < sizeof(logBuf) - logLen) {
    logLen += written;                     // guard the snprintf return
  }
}

// ======================= GPS POWER SAVE =======================
#if GPS_POWERSAVE
// UBX-CFG-RXM binary command (NOT $PUBX,41 — that configures the port, not the power mode).
const uint8_t UBX_PSM[]     = {0xB5,0x62,0x06,0x11,0x02,0x00,0x08,0x01,0x22,0x92};
const uint8_t UBX_MAXPERF[] = {0xB5,0x62,0x06,0x11,0x02,0x00,0x08,0x00,0x21,0x91};
bool gpsSleeping = false;

void gpsPower(bool save) {
  if (save == gpsSleeping) return;
  gpsSerial.write(save ? UBX_PSM : UBX_MAXPERF, 10);
  gpsSleeping = save;
  if (evlog) evlog->print(save ? "GPS: power save (parked)" : "GPS: max performance");
}
#endif

// ======================= TELEGRAM =======================
#if TG_ENABLED
void sendTelegramAlert(const char* text) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();                    // no CA check; fine for an alert
  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + TG_TOKEN +
               "/sendMessage?chat_id=" + TG_CHAT + "&text=" + text;
  if (http.begin(client, url)) {           // blocks the loop ~1-2 s — rare event, acceptable
    http.GET();
    http.end();
  }
}
#else
void sendTelegramAlert(const char*) {}
#endif

// ======================= WEB ENDPOINTS =======================
bool safePath(const String& p) {           // root only, .csv only, no ..
  return p.startsWith("/") && p.endsWith(".csv") &&
         p.indexOf("..") < 0 && p.indexOf('/', 1) < 0;
}

struct GpxState {
  File f;
  String pending;
  bool headerDone = false, footerDone = false;
};

void setupEndpoints() {
  AsyncWebServer& srv = dash.server();

  srv.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* r) {
    String json = "[";
    forEachCsv([&](const String& n, size_t sz) {
      if (json.length() > 1) json += ",";
      json += "{\"file\":\"" + n + "\",\"bytes\":" + String(sz) + "}";
    });
    json += "]";
    r->send(200, "application/json", json);
  });

  srv.on("/download", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!r->hasParam("file")) { r->send(400, "text/plain", "Missing 'file'"); return; }
    String path = r->getParam("file")->value();
    if (!safePath(path))       { r->send(400, "text/plain", "Bad path");  return; }
    if (!FS_IMPL.exists(path)) { r->send(404, "text/plain", "Not found"); return; }
    r->send(FS_IMPL, path, "text/csv", true);
  });

  // Stream CSV -> GPX without buffering the file in RAM.
  srv.on("/gpx", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!r->hasParam("file")) { r->send(400, "text/plain", "Missing 'file'"); return; }
    String path = r->getParam("file")->value();
    if (!safePath(path))       { r->send(400, "text/plain", "Bad path");  return; }
    if (!FS_IMPL.exists(path)) { r->send(404, "text/plain", "Not found"); return; }

    auto st = std::make_shared<GpxState>();
    st->f = FS_IMPL.open(path, "r");

    auto resp = r->beginChunkedResponse("application/gpx+xml",
      [st](uint8_t* out, size_t maxLen, size_t) -> size_t {
        while (st->pending.length() < maxLen) {
          if (!st->headerDone) {
            st->pending += F("<?xml version=\"1.0\"?>\n"
              "<gpx version=\"1.1\" creator=\"RisalDash GPS Tracker\" "
              "xmlns=\"http://www.topografix.com/GPX/1/1\">\n<trk><trkseg>\n");
            st->headerDone = true;
            continue;
          }
          if (st->f && st->f.available()) {
            String line = st->f.readStringUntil('\n');
            int c1 = line.indexOf(','), c2 = line.indexOf(',', c1 + 1);
            int c3 = line.indexOf(',', c2 + 1), c4 = line.indexOf(',', c3 + 1);
            if (c1 > 0 && c2 > c1 && c3 > c2 && c4 > c3) {
              st->pending += "<trkpt lat=\"" + line.substring(c1 + 1, c2) +
                             "\" lon=\"" + line.substring(c2 + 1, c3) + "\">" +
                             "<ele>" + line.substring(c3 + 1, c4) + "</ele>" +
                             "<time>" + line.substring(0, c1) + "</time></trkpt>\n";
            }
            continue;
          }
          if (!st->footerDone) {
            st->pending += F("</trkseg></trk></gpx>\n");
            st->footerDone = true;
            if (st->f) st->f.close();
          }
          break;
        }
        size_t n = min((size_t)st->pending.length(), maxLen);
        memcpy(out, st->pending.c_str(), n);
        st->pending.remove(0, n);
        return n;
      });
    r->send(resp);
  });
}

// ======================= ALERT SIGNAL =======================
void alertTask() {
#if ALERT_MODE != 0
  bool phase = alarmOn && (millis() % 700 < 200);
  #if ALERT_MODE == 4
    static bool wasOn = false;
    if (phase && !wasOn)  tone(ALERT_PIN, BUZZER_FREQ);
    if (!phase && wasOn)  noTone(ALERT_PIN);
    wasOn = phase;
  #else
    digitalWrite(ALERT_PIN, phase ? HIGH : LOW);
  #endif
#endif
}

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
#if ALERT_MODE != 0
  pinMode(ALERT_PIN, OUTPUT);
  digitalWrite(ALERT_PIN, LOW);
#endif

#if defined(ESP32)
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#else
  gpsSerial.begin(9600);
#endif

  storageOk = storageBegin();
  Serial.printf("Storage (%s): %s\n", USE_SD ? "SD" : "LittleFS",
                storageOk ? "OK" : "FAIL");

  // --- dashboard widgets ---
  dash.map("Track", &lat, &lon);
  dash.gauge("Speed", &speedKmh, 0, 40, "km/h");
  dash.badge("Satellites", &sats);
  dash.metric("HDOP", &hdop);
  dash.chart("Altitude", &altM, "m");
  dash.metric("Distance home", &distHome, "m");
  dash.toggle("Geofence", &fenceOn);
  dash.slider("Fence radius (m)", &fenceRadius, 20, 500);
  dash.button("Home", "Set home here", []() {
    if (sats >= 4) {
      homeLat = lat; homeLon = lon; homeSet = true;
      outsideCount = 0;
      if (evlog) evlog->print("New home set");
    } else if (evlog) {
      evlog->print("No fix — home unchanged");
    }
  });
  dash.metric("Free RAM", &freeHeapKb, "KB");     // heap-leak diagnostics
  dash.metric("Wi-Fi RSSI", &wifiRssi, "dBm");    // link quality
  evlog = &dash.log("Events", 6);

  setupEndpoints();   // register custom routes before begin() starts the server

  dash.apName("GPS-Tracker");   // SSID of the captive setup portal on first boot
#if TG_ENABLED
  dash.begin(WIFI_SSID, WIFI_PASS);   // Telegram needs internet -> join home Wi-Fi (portal on failure)
#elif USE_PORTAL
  dash.begin();                        // first boot -> "GPS-Tracker" setup portal -> joins your Wi-Fi
#else
  dash.beginAP("GPS-Tracker", "12345678");  // standalone AP dashboard, always reachable in the field
#endif

  // Print where to reach the dashboard.
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
  else
    Serial.printf("Setup portal: http://%s/  (join the \"GPS-Tracker\" Wi-Fi)\n",
                  WiFi.softAPIP().toString().c_str());

  lastMoveMs = millis();
}

// ======================= LOOP =======================
void loop() {
  while (gpsSerial.available()) gps.encode(gpsSerial.read());

  if (gps.location.isUpdated() && gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
    speedKmh = gps.speed.kmph();
    altM = gps.altitude.meters();
    sats = gps.satellites.value();
    hdop = gps.hdop.hdop();

    if (!homeSet && sats >= 5 && hdop < 2.5) {
      homeLat = lat;      homeLon = lon;
      lastLogLat = lat;   lastLogLon = lon;
      homeSet = true;
      if (evlog) evlog->print("Home fixed");
    }

    // --- geofence with debounce + hysteresis ---
    if (homeSet) {
      distHome = TinyGPSPlus::distanceBetween(lat, lon, homeLat, homeLon);
      if (fenceOn && distHome > fenceRadius) {
        if (outsideCount < FENCE_DEBOUNCE) outsideCount++;
        if (outsideCount == FENCE_DEBOUNCE && !alarmOn) {
          alarmOn = true;
          if (evlog) evlog->print("ALARM: outside the zone");
          sendTelegramAlert("ALARM: tracker left the geofence!");
        }
      } else if (!fenceOn || distHome < fenceRadius * 0.9f) {  // 10% hysteresis
        outsideCount = 0;
        if (alarmOn) { alarmOn = false; if (evlog) evlog->print("Back in the zone"); }
      }
    }

    // --- log: moving + speed (drift filter) + period ---
    bool isMoving = speedKmh > MIN_SPEED_KMH;
    if (isMoving) lastMoveMs = millis();
    float moved = TinyGPSPlus::distanceBetween(lat, lon, lastLogLat, lastLogLon);
    if (isMoving && moved > LOG_MIN_MOVE_M && millis() - lastLogMs > LOG_MIN_PERIOD) {
      bufferPoint();
      lastLogLat = lat; lastLogLon = lon; lastLogMs = millis();
    }
  }

  // --- dynamic flush + parked power save ---
  bool parked = millis() - lastMoveMs > PARK_TIMEOUT;
  uint32_t flushEvery = parked ? FLUSH_PARKED : FLUSH_PERIOD;
  if (millis() - lastFlushMs > flushEvery) { flushLog(); lastFlushMs = millis(); }
#if GPS_POWERSAVE
  gpsPower(parked && !alarmOn);            // in an alarm, keep GPS at max performance
#endif

  // --- hardware telemetry every 2 s ---
  if (millis() - lastTelemMs > 2000) {
    freeHeapKb = ESP.getFreeHeap() / 1024.0f;
    wifiRssi = WiFi.RSSI();
    lastTelemMs = millis();
    // GPS health — chars=0 means no NMEA is arriving (check wiring / swap TX-RX); chars rising but
    // sats=0 means it's still acquiring (give it clear sky and a minute).
    Serial.printf("GPS: chars=%lu sats=%d fix=%s\n", gps.charsProcessed(),
                  (int)gps.satellites.value(), gps.location.isValid() ? "yes" : "no");
  }

  // GPS status on the dashboard's Events feed, so you can tell it's alive without a serial cable:
  // logs on every state change, repeats while searching / no-data, then goes quiet once fixed.
  static uint32_t lastGpsLog = 0;
  static int gpsState = -1;
  int state = gps.charsProcessed() < 10 ? 0
            : (gps.location.isValid() && gps.location.age() < 8000 ? 2 : 1);
  if (state != gpsState || (state != 2 && millis() - lastGpsLog > 12000)) {
    lastGpsLog = millis();
    gpsState = state;
    char m[64];
    if (state == 0)      snprintf(m, sizeof(m), "GPS: no data - check wiring (module TX -> RX pin)");
    else if (state == 1) snprintf(m, sizeof(m), "GPS: searching - %d sats, %lu chars", (int)gps.satellites.value(), gps.charsProcessed());
    else                 snprintf(m, sizeof(m), "GPS: fix acquired - %d sats, HDOP %.1f", (int)gps.satellites.value(), hdop);
    if (evlog) evlog->print(m);
  }

  alertTask();
  dash.update();

#if defined(ESP8266)
  yield();
#endif
}
