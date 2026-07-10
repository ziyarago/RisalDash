// ESP32-C6 (Waveshare C6-LCD-1.47) web-controlled display carousel. Thirteen LCD slides showcase
// the render styles — QR address, gauge, metric+bar, stat, badge, sparkline, robot eyes, weather,
// LED, progress, stat grid and a thermal heatmap. The web dashboard picks what the LCD shows, the
// auto-slide interval, backlight and the RGB LED mode. This file is just the dashboard + the slide
// logic; everything else lives in its own header:
//   display.h — LCD pixel drawing (lcd::)     led.h     — onboard RGB LED (led::)
//   i18n.h    — dashboard translations        sensors.h — every (fake) data source
//   weather.h — Open-Meteo background task    ble.h     — real NimBLE scanner
//
// ⚠ HEAT: on these Waveshare ESP32 LCD boards, running the backlight at 100% OVERHEATS the panel
// (Waveshare: keep <=50%). We PWM it (lcd::backlight, default 43%) and add Wi-Fi modem sleep + an
// 80 MHz CPU clock to keep the board cool. Never drive the backlight pin fully HIGH.
//
// Requires: "GFX Library for Arduino" (Arduino_GFX) + "QRCode" (ricmoo) + "U8g2" (Cyrillic LCD
// font) — via Arduino Library Manager, or PlatformIO lib_deps. On first boot dash.begin() raises
// the Wi-Fi setup portal; after provisioning it serves the dashboard and drives the LCD (the
// address slide shows a QR to the dashboard URL). Board: ESP32-C6.
// NOTE: this showcase has ~60 widgets across its pages — more than the default cap of 48. Raise it
// with a BUILD FLAG so the whole library sees it (a #define in the sketch can't reach RisalUI.cpp):
// PlatformIO: build_flags = -D RISAL_MAX_WIDGETS=80  ·  Arduino IDE: edit RISAL_MAX_WIDGETS in RisalUI.h.
#include "display.h"
#include "led.h"
#include "i18n.h"
#include "sensors.h"
#include "weather.h"
#include "ble.h"
#include <RisalUI.h>
#include <WiFi.h>
#include <Preferences.h>

Preferences prefs;  // persists the display/LED settings to NVS (survive reboots)

RisalUI dash("Greenhouse");

// Control / UI state (persisted to NVS).
bool pump = false;
bool autoSlide = true;  // Auto-slide (Settings gear): cycle the LCD widgets on a timer
int showSel = 0;        // Show, when auto is off (0-based slide index)
int slideSec = 4;       // Slide sec between widgets in auto-slide
int backlight = 43;     // Backlight % (keep <=50 — heat)
int ledMode = 2;        // LED:   0 Off · 1 Manual · 2 Per-widget · 3 Gradient
String ledColor = "#22d3ee";
int mood = 1;           // Robot face: 0 Neutral · 1 Happy · … · 9 Look
bool autoEmo = false;   // Auto emotion: cycle through all moods (web face + LCD eyes follow)

// Fake Zigbee coordinator — a few paired smart-home devices (real path: the esp-zigbee stack on C6).
bool zbLight = false, zbPlug = true;
int zbDoor = 0, zbMotion = 0;  // badges: 0 ok · 1 warn
float zbDevices = 4;

// Widget handles filled in setup().
LogWidget *bleLog = nullptr;
TerminalWidget *term = nullptr;
HeatmapWidget *heat = nullptr;

// Slide state: which LCD slide is visible + last-drawn caches so we repaint only on change.
int curSlide = 1, lastSlide = -1;
float lastTemp = -999, lastPres = -1, lastWxT = -999;
int lastHum = -1, lastSoil = -1, lastAirq = -1, lastMood = -1;
int lastPump = -1, lastBl = -1;
String lastWxCity = "";
bool lastBlink = false;

void setup() {
  Serial.begin(115200);
  delay(120);
  setCpuFrequencyMhz(80);  // less heat; plenty for web + LCD

  sensorsBegin();

  prefs.begin("rdisp", false);  // persisted settings survive reboots (defaults on first run)
  autoSlide = prefs.getInt("auto", 1);
  showSel = prefs.getInt("show", showSel);
  slideSec = prefs.getInt("sec", slideSec);
  backlight = prefs.getInt("bl", backlight);
  ledMode = prefs.getInt("led", ledMode);
  ledColor = prefs.getString("col", ledColor);
  mood = prefs.getInt("mood", mood);
  autoEmo = prefs.getInt("aemo", 0);
  wx::city = prefs.getString("city", wx::city);

  lcd::begin();
  lcd::backlight(backlight);

  dash.translate(trWeb);  // localize our page/widget/sensor titles for the served dashboard (ru/uz/ar)

  // Swipe pages (global nav strip + tile sheet) instead of one long scroll — dash.layout() per page.
  dash.layout("Sensors", RICON_LEAF);
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil moisture", &soil, "%");
  dash.stat("Light", &light, "lux");
  dash.toggle("Replay temp", &replayMode);  // loop the recorded temperature (RisalRecorder)
  dash.sensor("ld2410", &presence, &presDist, &presMotion);  // mmWave preset (Presence/Distance/Motion)

  dash.layout("Weather", RICON_WATER);
  dash.text("City", &wx::city, [](const String &v) { (void)v; prefs.putString("city", wx::city); wx::dirty = true; });  // type a city -> geocode + persist
  dash.label("Place", &wx::place);
  dash.stat("Outside", &wx::temp, "C");
  dash.label("Sky", &wx::desc);
  dash.stat("Wind", &wx::wind, "km/h");

  dash.layout("Route", RICON_MOTION);
  dash.map("Track", &gpsLat, &gpsLon).size(RSIZE_L);  // needs internet on the client (Leaflet + tiles)
  dash.stat("Speed", &gpsSpeed, "km/h");
  dash.stat("Heading", &gpsHeading, "deg");

  dash.layout("Energy", RICON_FLASH);
  dash.gauge("Power", &power, 0, 2000, "W");
  dash.stat("Today", &energyKwh, "kWh").decimals(2);
  dash.stat("Cost", &cost, "$").decimals(2);
  dash.number("Tariff c/kWh", &tariff, 1, 100, 1);

  dash.layout("BLE", RICON_SIGNAL);
  bleLog = &dash.log("Nearby", 5);  // fed by the real NimBLE scan (ble.h)

  dash.layout("Motion", RICON_MOTION);
  dash.cube("Orientation", &imuP, &imuR, &imuY).size(RSIZE_L);  // 3D IMU cube (fake angles)
  dash.stat("Pitch", &imuP, "deg").decimals(0);
  dash.stat("Roll", &imuR, "deg").decimals(0);

  dash.layout("Console", RICON_WIFI);
  term = &dash.terminal("Shell", [](const String &cmd) {  // type commands from the dashboard
    if (!term) return;
    if (cmd == "help") term->print("cmds: help, heap, ip, temp, uptime");
    else if (cmd == "heap") term->print(String(ESP.getFreeHeap()) + " bytes free");
    else if (cmd == "ip") term->print(WiFi.localIP().toString());
    else if (cmd == "temp") term->print(String(temp, 1) + " C");
    else if (cmd == "uptime") term->print(String(millis() / 1000) + " s");
    else term->print("unknown: " + cmd + " (try 'help')");
  });
  term->print("RisalDash console - type 'help'");

  dash.layout("Thermal", RICON_THERMOMETER);
  heat = &dash.heatmap("Thermal cam", TW, TH);  // MLX90640-style (fake frames from sensors.h)

  dash.layout("Zigbee", RICON_HOME);  // fake coordinator panel (real path: esp-zigbee)
  dash.stat("Devices", &zbDevices).decimals(0);
  dash.toggle("Living light", &zbLight);
  dash.toggle("Smart plug", &zbPlug);
  dash.badge("Door", &zbDoor);
  dash.badge("Motion", &zbMotion);

  dash.layout("Robot", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_L);  // big, full-width face
  dash.select("Emotion", "Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look", &mood, [](int i) { (void)i; prefs.putInt("mood", mood); });
  // Auto emotion: cycle through all moods on a timer (the LCD "Robot" slide shows the same).
  dash.toggle("Auto emotion", &autoEmo, [](bool on) { (void)on; prefs.putInt("aemo", autoEmo); });

  // Device settings — all live in the Settings gear (.gear()), not on a dashboard page: brightness,
  // the LCD slide picker + auto-slide interval, and the RGB LED mode/colour. (Language is the
  // library's built-in Settings switcher; the LCD follows it via dash.language() in loop().)
  dash.toggle("Auto-slide", &autoSlide, [](bool on) { (void)on; prefs.putInt("auto", autoSlide); }).gear();
  dash.select("Show", "Address,Air temp,Humidity,Soil,Pressure,Air quality,Trend,Robot,Weather,Pump,Backlight,Overview,Thermal,BLE", &showSel, [](int i) { (void)i; prefs.putInt("show", showSel); }).gear();
  dash.number("Slide sec", &slideSec, 2, 30, 1, [](int i) { (void)i; prefs.putInt("sec", slideSec); }).gear();
  dash.slider("Backlight", &backlight, 5, 100, [](int i) { (void)i; prefs.putInt("bl", backlight); lcd::backlight(backlight); }).gear();
  dash.select("LED mode", "Off,Manual,Per-widget,Gradient", &ledMode, [](int i) { (void)i; prefs.putInt("led", ledMode); led::apply(ledMode, ledColor, curSlide); }).gear();
  dash.color("LED colour", &ledColor, [](const String &v) { (void)v; prefs.putString("col", ledColor); led::apply(ledMode, ledColor, curSlide); }).gear();

  dash.layout("Control", RICON_POWER);
  dash.toggle("Pump", &pump);

  dash.begin();
  lcd::setLang(dash.language());  // LCD follows the device language (restored from NVS by begin())
  bleScan::begin();               // start the real BLE scan once Wi-Fi is up (they share the radio)
  WiFi.setSleep(true);            // Wi-Fi modem sleep — heat/power saver
  Serial.printf("[net] staIP=%s\n", WiFi.localIP().toString().c_str());
  led::apply(ledMode, ledColor, curSlide);
  wx::begin();                    // weather fetcher task — its blocking HTTPS never stalls loop()
}

uint32_t lastSwap = 0, lastLed = 0, lastHb = 0, lastChart = 0, lastEmo = 0, lastZb = 0, lastMulti = 0, lastHeatDraw = 0, lastBleDraw = 0;

// Choose the visible slide (auto-cycle, or the pinned one), and repaint its static chrome on change.
void updateSlide() {
  if (autoSlide) {
    uint32_t iv = (uint32_t)(slideSec < 2 ? 2 : slideSec) * 1000;
    if (millis() - lastSwap > iv) { lastSwap = millis(); curSlide = curSlide >= lcd::NUM_SLIDES ? 1 : curSlide + 1; }
  } else {
    curSlide = showSel + 1;
  }
  if (curSlide == lastSlide) return;
  lcd::slideStatic(curSlide, WiFi.localIP().toString(), "RisalDash v" RISALDASH_VERSION);
  lastSlide = curSlide;
  lastTemp = -999; lastPres = -1; lastWxT = -999; lastHum = lastSoil = lastAirq = lastMood = -1;
  lastPump = lastBl = -1;  // force the new slides to paint on entry
  lastSwap = millis();
  if (ledMode == led::PER_WIDGET) led::apply(ledMode, ledColor, curSlide);
}

// Repaint only the live value on the current slide, and only when it actually changed.
void drawSlideValue() {
  switch (curSlide) {
    case 2: if (fabsf(temp - lastTemp) >= 0.1f) { lcd::gaugeValue(temp, 0, 50); lastTemp = temp; } break;
    case 3: if ((int)hum != lastHum) { lcd::metricValue((int)hum, lcd::C_TEAL); lastHum = (int)hum; } break;
    case 4: if (soil != lastSoil) { lcd::metricValue(soil, lcd::C_GREEN); lastSoil = soil; } break;
    case 5: if ((int)pres != (int)lastPres) { char b[8]; snprintf(b, sizeof(b), "%d", (int)pres); lcd::statValue(b, "hPa", lcd::C_BLUE); lastPres = pres; } break;
    case 6: if (airq != lastAirq) { lcd::badgeValue(airq); lastAirq = airq; } break;
    case 7: if (millis() - lastChart > 500) { lastChart = millis(); lcd::chartValue(&thist[THN - thCount], thCount, 18, 30, lcd::C_TEAL); } break;
    case 8: {  // robot eyes — redraw on mood change, blink edge, or each frame of Look
      bool blink = (millis() % 3800) < 130;
      bool anim = (mood == 9);  // "Look" drifts, so it needs continuous redraw
      if (mood != lastMood || blink != lastBlink || (anim && millis() - lastChart > 180)) {
        lcd::eyes(mood, blink);
        lastMood = mood;
        lastBlink = blink;
        if (anim) lastChart = millis();
      }
    } break;
    case 9: if (fabsf(wx::temp - lastWxT) >= 0.1f || wx::place != lastWxCity) { lcd::weatherValue(wx::temp, wx::place.c_str(), wx::desc.c_str(), wx::valid); lastWxT = wx::temp; lastWxCity = wx::place; } break;
    case 10: if ((int)pump != lastPump) { lcd::ledValue(pump, lcd::C_TEAL); lastPump = pump; } break;  // LED indicator
    case 11: if (backlight != lastBl) { lcd::progressValue(backlight, lcd::C_AMBER); lastBl = backlight; } break;  // progress bar
    case 12: if (millis() - lastMulti > 700) {  // several readings at once (stat grid)
      lastMulti = millis();
      static char v0[12], v1[12], v2[12], v3[12];
      snprintf(v0, sizeof(v0), "%.1f C", temp);
      snprintf(v1, sizeof(v1), "%d %%", (int)hum);
      snprintf(v2, sizeof(v2), "%d %%", soil);
      snprintf(v3, sizeof(v3), "%d hPa", (int)pres);
      lcd::MiniCell cells[4] = {
        {lcd::tr(lcd::T_AIRTEMP), v0, lcd::C_TEAL},
        {lcd::tr(lcd::T_HUMIDITY), v1, lcd::C_BLUE},
        {lcd::tr(lcd::T_SOIL), v2, lcd::C_GREEN},
        {lcd::tr(lcd::T_PRESSURE), v3, lcd::C_AMBER},
      };
      lcd::multiValue(cells, 4);
    } break;
    case 13: if (millis() - lastHeatDraw > 450) { lastHeatDraw = millis(); lcd::heatmapValue(thermal, TW, TH, 20, 40); } break;  // thermal cam
    case 14: if (millis() - lastBleDraw > 2000) {  // real BLE scan: nearby devices + decoded beacon
      lastBleDraw = millis();
      static bleScan::Dev dv[6];
      int m = bleScan::snapshot(dv, 6);
      const char *nm[6]; int rs[6]; bool sn[6];
      bool haveB = false; float bt = 0; int bh = 0, bb = 0;
      for (int i = 0; i < m; i++) {
        nm[i] = dv[i].name; rs[i] = dv[i].rssi; sn[i] = dv[i].sensor;
        if (dv[i].sensor && !haveB) { haveB = true; bt = dv[i].temp; bh = dv[i].hum; bb = dv[i].batt; }
      }
      lcd::bleSlideValue(nm, rs, sn, m, haveB, bt, bh, bb);
    } break;
  }
}

void loop() {
  dash.update();               // serve web + push widget updates
  sampleSensors();             // advance every fake data source (4 Hz inside)
  updateThermal(heat);         // fake thermal frames -> web heatmap (2 Hz inside)
  bleScan::updateLog(bleLog);  // rebuild "Nearby" from the live BLE scan (2.5 s inside)
  wx::sync();                  // copy the weather task's latest result (1 s inside)

  if (autoEmo && millis() - lastEmo > 1500) { lastEmo = millis(); mood = (mood + 1) % 10; }  // cycle emotions
  if (millis() - lastZb > 3000) {  // fake Zigbee sensor events
    lastZb = millis();
    zbDoor = (millis() / 15000) % 2;                  // door opens/closes
    zbMotion = ((millis() / 6000) % 3 == 0) ? 1 : 0;  // motion detected
  }

  // Keep the LCD language in sync with the library's Settings switcher (en/ru/uz/ar differ by first
  // letter). No polling cost beyond a char compare; repaints the label when it changes.
  static char lastLangC = 0;
  if (dash.language()[0] != lastLangC) { lastLangC = dash.language()[0]; lcd::setLang(dash.language()); lastSlide = -1; }

  updateSlide();     // which slide + static chrome
  drawSlideValue();  // its live value
  if (ledMode == led::GRADIENT && millis() - lastLed > 60) { lastLed = millis(); led::apply(ledMode, ledColor, curSlide); }
  if (millis() - lastHb > 3000) {  // serial heartbeat
    lastHb = millis();
    Serial.printf("[hb] auto=%d slide=%d led=%d heap=%u\n", autoSlide, curSlide, ledMode, ESP.getFreeHeap());
  }
  delay(5);
}
