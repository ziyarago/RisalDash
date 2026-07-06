// ESP32-C6 (Waveshare C6-LCD-1.47) web-controlled display carousel. Seven slides showcase the render
// styles on the LCD — QR address, gauge, metric+bar, plain stat, status badge and a trend sparkline.
// The web dashboard picks what the LCD shows, the auto-slide interval, backlight and the RGB LED mode.
// All the pixel drawing lives in display.h — this file is just the dashboard + logic.
//
// ⚠ HEAT: on these Waveshare ESP32 LCD boards, running the backlight at 100% OVERHEATS the panel
// (Waveshare: keep <=50%). We PWM it (lcd::backlight, default 43%) and add Wi-Fi modem sleep + an
// 80 MHz CPU clock to keep the board cool. Never drive the backlight pin fully HIGH.
//
// Requires: "GFX Library for Arduino" (Arduino_GFX) + "QRCode" (ricmoo) — via Arduino Library Manager,
// or PlatformIO `lib_deps = moononournation/GFX Library for Arduino` + `ricmoo/QRCode`. On first boot
// dash.begin() raises the Wi-Fi setup portal; after provisioning it serves the dashboard and drives
// the LCD (address slide shows a QR to the dashboard URL). Board: ESP32-C6.
// NOTE: this showcase has ~50 widgets across its pages — more than the default cap of 32. Raise it
// with a BUILD FLAG so the whole library sees it (a #define in the sketch can't reach RisalUI.cpp):
// PlatformIO: build_flags = -D RISAL_MAX_WIDGETS=64  ·  Arduino IDE: edit RISAL_MAX_WIDGETS in RisalUI.h.
#include "display.h"
#include "led.h"
#include <RisalUI.h>
#include <RisalFake.h>     // realistic fake sensors (library) — build/debug with no hardware attached
#include <RisalWeather.h>  // live weather (Open-Meteo, library) fetched in a background task
#include <WiFi.h>
#include <Preferences.h>
#include <math.h>

Preferences prefs;  // persists the display/LED settings to NVS (survive reboots)

RisalUI dash("Greenhouse");
RisalFakeEnv sensors;  // realistic emulator — swap for a real AHT20/BMP280 driver without touching this file
// Latest sensor readings (filled by sampleSensors() from `sensors`).
float temp = 24.3f, hum = 62;
int soil = 48;
bool pump = false;
float pres = 1013.0f;  // barometric pressure, hPa (BMP280)
float light = 0;       // ambient light, lux (follows the emulator's day/night cycle)
int airq = 0;          // air quality: 0 GOOD · 1 FAIR · 2 POOR
float presence = 0, presDist = 250, presMotion = 0;  // fake mmWave (LD2410) presence / distance / motion

// Fake GPS driving the map widget — plays this loop of waypoints (no GPS module needed).
float gpsLat = 41.311f, gpsLon = 69.279f, gpsSpeed = 0, gpsHeading = 0;
RisalFakeGPS gps;
const float ROUTE[] = {41.311f, 69.279f, 41.318f, 69.286f, 41.315f, 69.298f,
                       41.306f, 69.296f, 41.302f, 69.283f, 41.311f, 69.279f};

// Fake energy meter (stand-in for a PZEM-004T): power draw, accumulated kWh and cost.
float power = 0, energyKwh = 0, cost = 0;
int tariff = 12;  // price per kWh, cents — editable on the dashboard
RisalFake powerFake(850, 650, 40, 1.3f);  // watts, wandering

// Fake BLE scanner (stand-in for a NimBLE scan) — nearby devices incl. a Xiaomi temp/humidity beacon.
RisalFakeBLE ble;
LogWidget *bleLog = nullptr;

// Record & replay the temperature: capture the live signal, then loop the recording (Replay toggle).
RisalRecorder rec;
bool replayMode = false;

// Fake IMU orientation (degrees) for the 3D cube widget.
float imuP = 0, imuR = 0, imuY = 0;
TerminalWidget *term = nullptr;  // WebSocket console

// Fake thermal camera (stand-in for MLX90640) — a moving hotspot over a warm field.
HeatmapWidget *heat = nullptr;
static const int TW = 24, TH = 18;
float thermal[TW * TH];

// Fake Zigbee coordinator — a few paired smart-home devices (real path: the esp-zigbee stack on C6).
bool zbLight = false, zbPlug = true;
int zbDoor = 0, zbMotion = 0;  // badges: 0 ok · 1 warn
float zbDevices = 4;
static const int THN = 40;
float thist[THN] = {0};  // temperature history for the trend sparkline
int thCount = 0;         // valid samples in thist (newest at the end)

// Web-controlled state.
bool autoSlide = true;  // Auto-slide (Settings gear): cycle the LCD widgets on a timer
int showSel = 0;        // Show, when auto is off: 0 Address · 1 Temp · 2 Humidity · 3 Soil · 4 Pressure · 5 Air · 6 Trend
int slideSec = 4;       // Slide sec between widgets in auto-slide
int backlight = 43;     // Backlight % (keep <=50 — heat)
int ledMode = 2;        // LED:   0 Off · 1 Manual · 2 Per-widget · 3 Gradient
String ledColor = "#22d3ee";
int mood = 1;           // Robot face: 0 Neutral · 1 Happy · 2 Sad · 3 Angry · 4 Surprised · 5 Sleepy · 6 Love
bool autoEmo = false;   // Auto emotion: cycle through all moods (web face + LCD eyes follow)

// ── Weather. The HTTPS fetch blocks (~1-2 s), so it runs in a background FreeRTOS task and publishes
//    into wxShared under a mutex; loop() copies that into these display globals and never blocks.
const float WX_LAT = 41.3111f, WX_LON = 69.2797f;  // default location (Tashkent) until a city is entered
String cityInput = "Tashkent";  // web "City" field — the task re-geocodes it whenever it changes
float wxTemp = 0;     // last values shown by the widgets / LCD (copied from the task under wxMux)
int wxCode = -1;
float wxWind = 0;
bool wxValid = false;
String wxDesc = "...", wxCity = "...";
#if defined(ESP32)
RisalWeather weather;
SemaphoreHandle_t wxMux;
volatile bool cityDirty = false;  // set by the City field's callback, consumed by the task
struct { float temp; int code; float wind; bool valid; String city; } wxShared;

// Its own task (pinned to core 0; loop() runs on core 1 on dual-core parts). All blocking work —
// geocoding a new city AND fetching weather — stays here, off loop(), and is published under wxMux.
void weatherTask(void *) {
  weather.setLocation(WX_LAT, WX_LON, cityInput);
  weather.geocode(cityInput);  // resolve the (possibly persisted) city before the first fetch
  bool need = true;
  uint32_t last = 0;
  for (;;) {
    if (cityDirty) { cityDirty = false; weather.geocode(cityInput); need = true; }  // resolve city -> lat/lon
    if (need || millis() - last > 600000UL) {                                        // fetch on demand / every 10 min
      if (weather.poll()) {
        last = millis();
        need = false;
        xSemaphoreTake(wxMux, portMAX_DELAY);
        wxShared.temp = weather.temperature();
        wxShared.code = weather.code();
        wxShared.wind = weather.wind();
        wxShared.city = weather.city();
        wxShared.valid = true;
        xSemaphoreGive(wxMux);
        Serial.printf("[wx] %s %.1fC %s wind=%.0f heap=%u\n", weather.city().c_str(), weather.temperature(), weather.description(), weather.wind(), ESP.getFreeHeap());
      } else {
        need = false;
        last = millis() - 570000UL;  // failed -> retry in ~30 s
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));  // wake often enough to react to a city change
  }
}
#endif

// Slides: 1 Address · 2 Temp · 3 Humidity · 4 Soil · 5 Pressure · 6 Air · 7 Trend · 8 Robot · 9 Weather.
int curSlide = 1, lastSlide = -1;
float lastTemp = -999, lastPres = -1, lastWxT = -999;
int lastHum = -1, lastSoil = -1, lastAirq = -1, lastMood = -1;
String lastWxCityStr = "";
bool lastBlink = false;

void setup() {
  Serial.begin(115200);
  delay(120);
  setCpuFrequencyMhz(80);  // less heat; plenty for web + LCD

  sensors.begin();
  gps.begin(ROUTE, 6);

  prefs.begin("rdisp", false);  // persisted settings survive reboots (defaults on first run)
  autoSlide = prefs.getInt("auto", 1);
  showSel = prefs.getInt("show", showSel);
  slideSec = prefs.getInt("sec", slideSec);
  backlight = prefs.getInt("bl", backlight);
  ledMode = prefs.getInt("led", ledMode);
  ledColor = prefs.getString("col", ledColor);
  mood = prefs.getInt("mood", mood);
  autoEmo = prefs.getInt("aemo", 0);
  cityInput = prefs.getString("city", cityInput);

  lcd::begin();
  lcd::backlight(backlight);

  // Swipe pages (global nav strip + tile sheet) instead of one long scroll — dash.layout() per page.
  dash.layout("Sensors", RICON_LEAF);
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil moisture", &soil, "%");
  dash.stat("Light", &light, "lux");
  dash.toggle("Replay temp", &replayMode, [](bool on) { (void)on; });  // loop the recorded temperature
  dash.sensor("ld2410", &presence, &presDist, &presMotion);  // mmWave presence preset (Presence/Distance/Motion)

  dash.layout("Weather", RICON_WATER);
  dash.text("City", &cityInput, [](const String &v) { (void)v; prefs.putString("city", cityInput); cityDirty = true; });  // type a city -> geocode + persist
  dash.label("Place", &wxCity);
  dash.stat("Outside", &wxTemp, "C");
  dash.label("Sky", &wxDesc);
  dash.stat("Wind", &wxWind, "km/h");

  dash.layout("Route", RICON_MOTION);
  dash.map("Track", &gpsLat, &gpsLon).size(RSIZE_L);  // needs internet on the client (Leaflet + tiles)
  dash.stat("Speed", &gpsSpeed, "km/h");
  dash.stat("Heading", &gpsHeading, "deg");

  dash.layout("Energy", RICON_FLASH);
  dash.gauge("Power", &power, 0, 2000, "W");
  dash.stat("Today", &energyKwh, "kWh").decimals(2);
  dash.stat("Cost", &cost, "$").decimals(2);
  dash.number("Tariff c/kWh", &tariff, 1, 100, 1, [](int i) { (void)i; });

  dash.layout("BLE", RICON_SIGNAL);
  bleLog = &dash.log("Nearby", 5);  // fake BLE scan feed (swap for a real NimBLE scan)

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
  heat = &dash.heatmap("Thermal cam", TW, TH);  // MLX90640-style (fake frames)

  dash.layout("Zigbee", RICON_HOME);  // fake coordinator panel (real path: esp-zigbee)
  dash.stat("Devices", &zbDevices).decimals(0);
  dash.toggle("Living light", &zbLight, [](bool on) { (void)on; });
  dash.toggle("Smart plug", &zbPlug, [](bool on) { (void)on; });
  dash.badge("Door", &zbDoor);
  dash.badge("Motion", &zbMotion);

  dash.layout("Robot", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_L);  // big, full-width face
  dash.select("Emotion", "Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look", &mood, [](int i) { (void)i; prefs.putInt("mood", mood); });
  // Auto emotion: cycle through all moods on a timer (the LCD "Robot" slide shows the same).
  dash.toggle("Auto emotion", &autoEmo, [](bool on) { (void)on; prefs.putInt("aemo", autoEmo); });

  // Device settings — all live in the Settings gear (.gear()), not on a dashboard page: brightness,
  // the LCD slide picker + auto-slide interval, and the RGB LED mode/colour.
  dash.toggle("Auto-slide", &autoSlide, [](bool on) { (void)on; prefs.putInt("auto", autoSlide); }).gear();
  dash.select("Show", "Address,Air temp,Humidity,Soil,Pressure,Air quality,Trend,Robot,Weather", &showSel, [](int i) { (void)i; prefs.putInt("show", showSel); }).gear();
  dash.number("Slide sec", &slideSec, 2, 30, 1, [](int i) { (void)i; prefs.putInt("sec", slideSec); }).gear();
  dash.slider("Backlight", &backlight, 5, 100, [](int i) { (void)i; prefs.putInt("bl", backlight); lcd::backlight(backlight); }).gear();
  dash.select("LED mode", "Off,Manual,Per-widget,Gradient", &ledMode, [](int i) { (void)i; prefs.putInt("led", ledMode); led::apply(ledMode, ledColor, curSlide); }).gear();
  dash.color("LED colour", &ledColor, [](const String &v) { (void)v; prefs.putString("col", ledColor); led::apply(ledMode, ledColor, curSlide); }).gear();

  dash.layout("Control", RICON_POWER);
  dash.toggle("Pump", &pump, [](bool on) { (void)on; });
  dash.begin();
  WiFi.setSleep(true);  // Wi-Fi modem sleep — heat/power saver
  Serial.printf("[net] staIP=%s\n", WiFi.localIP().toString().c_str());
  led::apply(ledMode, ledColor, curSlide);

#if defined(ESP32)
  // Start the weather fetcher on its own task so its blocking HTTPS call never stalls loop().
  wxMux = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(weatherTask, "weather", 12288, nullptr, 1, nullptr, 0);
#endif
}

uint32_t lastAnim = 0, lastSwap = 0, lastLed = 0, lastHb = 0, lastChart = 0, lastEmo = 0, lastWx = 0, lastBle = 0, lastHeat = 0, lastZb = 0;

// Pull the latest readings from `sensors` into the globals the slides/LED/dashboard use. The source
// (emulator vs real driver) lives entirely in sensors.h — this stays the same either way.
void sampleSensors() {
  sensors.update();
  if (replayMode && rec.size() > 8) {
    temp = rec.replay();  // loop the recorded temperature trace
  } else {
    temp = sensors.temperature();
    rec.record(temp);     // capture the live signal
  }
  hum = sensors.humidity();
  soil = sensors.soil();
  pres = sensors.pressure();
  light = sensors.light();
  airq = sensors.airQuality();
  gps.update();
  gpsLat = gps.lat(); gpsLon = gps.lon(); gpsSpeed = gps.speed(); gpsHeading = gps.heading();
  power = powerFake.read();
  if (power < 0) power = 0;
  energyKwh += power / 1000.0f * (0.25f / 3600.0f) * 200.0f;  // 200x speedup so kWh climbs visibly
  cost = energyKwh * tariff / 100.0f;
  presence = ((millis() / 8000) % 2) ? 1 : 0;                 // fake mmWave: person present ~every 8 s
  presDist = 200.0f + 130.0f * sinf(millis() * 0.0007f);
  presMotion = presence ? 40.0f + 40.0f * fabsf(sinf(millis() * 0.002f)) : 0.0f;
  imuP = 30.0f * sinf(millis() * 0.0009f);                    // fake IMU orientation for the 3D cube
  imuR = 25.0f * sinf(millis() * 0.0013f);
  imuY = fmodf(millis() * 0.03f, 360.0f);
  for (int i = 0; i < THN - 1; i++) thist[i] = thist[i + 1];  // push temp into the history ring
  thist[THN - 1] = temp;
  if (thCount < THN) thCount++;
}

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
    case 9: if (fabsf(wxTemp - lastWxT) >= 0.1f || wxCity != lastWxCityStr) { lcd::weatherValue(wxTemp, wxCity.c_str(), wxDesc.c_str(), wxValid); lastWxT = wxTemp; lastWxCityStr = wxCity; } break;
  }
}

void loop() {
  dash.update();                                                   // serve web + push widget updates
  if (millis() - lastAnim > 250) { lastAnim = millis(); sampleSensors(); }
  if (autoEmo && millis() - lastEmo > 1500) { lastEmo = millis(); mood = (mood + 1) % 10; }  // cycle emotions
  if (heat && millis() - lastHeat > 500) {  // fake thermal frame — a moving hotspot
    lastHeat = millis();
    float ph = millis() * 0.001f;
    float hx = TW / 2.0f + TW / 3.0f * sinf(ph), hy = TH / 2.0f + TH / 3.0f * cosf(ph * 0.7f);
    for (int y = 0; y < TH; y++)
      for (int x = 0; x < TW; x++) {
        float dx = x - hx, dy = y - hy;
        thermal[y * TW + x] = 22.0f + 15.0f * expf(-(dx * dx + dy * dy) / 12.0f) + 1.5f * sinf(x * 0.5f + ph);
      }
    heat->frame(thermal);
  }
  if (millis() - lastZb > 3000) {  // fake Zigbee sensor events
    lastZb = millis();
    zbDoor = (millis() / 15000) % 2;                  // door opens/closes
    zbMotion = ((millis() / 6000) % 3 == 0) ? 1 : 0;  // motion detected
  }
  if (bleLog && millis() - lastBle > 2500) {  // refresh the fake BLE scan feed
    lastBle = millis();
    ble.update();
    for (int i = ble.count() - 1; i >= 0; i--) {
      String line = String(ble.name(i)) + "  " + ble.rssi(i) + "dBm";
      if (ble.isSensor(i)) line += "  " + String(ble.sensorTemp(), 1) + "C " + ble.sensorHum() + "%";
      bleLog->print(line);
    }
  }
#if defined(ESP32)
  if (millis() - lastWx > 1000) {  // copy the weather task's latest result (quick, non-blocking)
    lastWx = millis();
    xSemaphoreTake(wxMux, portMAX_DELAY);
    wxTemp = wxShared.temp; wxCode = wxShared.code; wxWind = wxShared.wind; wxValid = wxShared.valid;
    if (wxValid) wxCity = wxShared.city;
    xSemaphoreGive(wxMux);
    wxDesc = wxValid ? RisalWeather::codeText(wxCode) : String("...");
  }
#endif
  updateSlide();                                                   // which slide + static chrome
  drawSlideValue();                                                // its live value
  if (ledMode == led::GRADIENT && millis() - lastLed > 60) { lastLed = millis(); led::apply(ledMode, ledColor, curSlide); }
  if (millis() - lastHb > 3000) {                                  // serial heartbeat
    lastHb = millis();
    Serial.printf("[hb] auto=%d slide=%d led=%d heap=%u\n", autoSlide, curSlide, ledMode, ESP.getFreeHeap());
  }
  delay(5);
}
