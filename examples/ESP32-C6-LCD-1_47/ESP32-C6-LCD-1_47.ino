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
#include "display.h"
#include "led.h"
#include <RisalUI.h>
#include <RisalFake.h>  // realistic fake sensors (library) — build/debug with no hardware attached
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
int airq = 0;          // air quality: 0 GOOD · 1 FAIR · 2 POOR
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

// Slides: 1 Address · 2 Air temp · 3 Humidity · 4 Soil · 5 Pressure · 6 Air quality · 7 Trend.
int curSlide = 1, lastSlide = -1;
float lastTemp = -999, lastPres = -1;
int lastHum = -1, lastSoil = -1, lastAirq = -1, lastMood = -1;
bool lastBlink = false;

void setup() {
  Serial.begin(115200);
  delay(120);
  setCpuFrequencyMhz(80);  // less heat; plenty for web + LCD

  sensors.begin();

  prefs.begin("rdisp", false);  // persisted settings survive reboots (defaults on first run)
  autoSlide = prefs.getInt("auto", 1);
  showSel = prefs.getInt("show", showSel);
  slideSec = prefs.getInt("sec", slideSec);
  backlight = prefs.getInt("bl", backlight);
  ledMode = prefs.getInt("led", ledMode);
  ledColor = prefs.getString("col", ledColor);
  mood = prefs.getInt("mood", mood);
  autoEmo = prefs.getInt("aemo", 0);

  lcd::begin();
  lcd::backlight(backlight);

  // Swipe pages (global nav strip + tile sheet) instead of one long scroll — dash.layout() per page.
  dash.layout("Sensors", RICON_LEAF);
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil moisture", &soil, "%");

  dash.layout("Robot", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_M);
  dash.select("Emotion", "Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look", &mood, [](int i) { (void)i; prefs.putInt("mood", mood); });
  // Auto emotion: cycle through all moods on a timer (the LCD "Robot" slide shows the same).
  dash.toggle("Auto emotion", &autoEmo, [](bool on) { (void)on; prefs.putInt("aemo", autoEmo); });

  dash.layout("Display", RICON_GAUGE);
  // Auto-slide lives in the Settings gear (.gear()), not the grid — it's a device setting.
  dash.toggle("Auto-slide", &autoSlide, [](bool on) { (void)on; prefs.putInt("auto", autoSlide); }).gear();
  dash.select("Show", "Address,Air temp,Humidity,Soil,Pressure,Air quality,Trend,Robot", &showSel, [](int i) { (void)i; prefs.putInt("show", showSel); });
  dash.number("Slide sec", &slideSec, 2, 30, 1, [](int i) { (void)i; prefs.putInt("sec", slideSec); });
  dash.slider("Backlight", &backlight, 5, 100, [](int i) { (void)i; prefs.putInt("bl", backlight); lcd::backlight(backlight); });

  dash.layout("Control", RICON_POWER);
  dash.toggle("Pump", &pump, [](bool on) { (void)on; });
  dash.select("Mode", "Off,Manual,Per-widget,Gradient", &ledMode, [](int i) { (void)i; prefs.putInt("led", ledMode); led::apply(ledMode, ledColor, curSlide); });
  dash.color("Color", &ledColor, [](const String &v) { (void)v; prefs.putString("col", ledColor); led::apply(ledMode, ledColor, curSlide); });
  dash.begin();
  WiFi.setSleep(true);  // Wi-Fi modem sleep — heat/power saver
  Serial.printf("[net] staIP=%s\n", WiFi.localIP().toString().c_str());
  led::apply(ledMode, ledColor, curSlide);
}

uint32_t lastAnim = 0, lastSwap = 0, lastLed = 0, lastHb = 0, lastChart = 0, lastEmo = 0;

// Pull the latest readings from `sensors` into the globals the slides/LED/dashboard use. The source
// (emulator vs real driver) lives entirely in sensors.h — this stays the same either way.
void sampleSensors() {
  sensors.update();
  temp = sensors.temperature();
  hum = sensors.humidity();
  soil = sensors.soil();
  pres = sensors.pressure();
  airq = sensors.airQuality();
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
  lastTemp = -999; lastPres = -1; lastHum = lastSoil = lastAirq = lastMood = -1;
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
  }
}

void loop() {
  dash.update();                                                   // serve web + push widget updates
  if (millis() - lastAnim > 250) { lastAnim = millis(); sampleSensors(); }
  if (autoEmo && millis() - lastEmo > 1500) { lastEmo = millis(); mood = (mood + 1) % 10; }  // cycle emotions
  updateSlide();                                                   // which slide + static chrome
  drawSlideValue();                                                // its live value
  if (ledMode == led::GRADIENT && millis() - lastLed > 60) { lastLed = millis(); led::apply(ledMode, ledColor, curSlide); }
  if (millis() - lastHb > 3000) {                                  // serial heartbeat
    lastHb = millis();
    Serial.printf("[hb] auto=%d slide=%d led=%d heap=%u\n", autoSlide, curSlide, ledMode, ESP.getFreeHeap());
  }
  delay(5);
}
