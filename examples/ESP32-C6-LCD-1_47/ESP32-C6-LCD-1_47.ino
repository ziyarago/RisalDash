// ESP32-C6 (Waveshare C6-LCD-1.47) web-controlled display carousel. The web dashboard picks what the
// LCD shows (Auto / a pinned widget / the address), the auto-slide interval, backlight and the RGB LED
// mode. All the pixel drawing lives in display.h — this file is just the dashboard + logic.
//
// ⚠ HEAT: on these Waveshare ESP32 LCD boards, running the backlight at 100% OVERHEATS the panel
// (Waveshare: keep <=50%). We PWM it (lcd::backlight, default 43%) and add Wi-Fi modem sleep + an
// 80 MHz CPU clock to keep the board cool. Never drive the backlight pin fully HIGH.
//
// Requires: the "GFX Library for Arduino" (Arduino_GFX) — Arduino Library Manager, or PlatformIO
// `lib_deps = moononournation/GFX Library for Arduino`. On first boot dash.begin() raises the Wi-Fi
// setup portal; after provisioning it serves the dashboard and drives the LCD. Board: ESP32-C6.
#include "display.h"
#include <RisalUI.h>
#include <WiFi.h>
#include <math.h>

RisalUI dash("Greenhouse");
float temp = 24.3f, hum = 62;
int soil = 48;
bool pump = false;

// Web-controlled state.
int dispMode = 0;    // Show:  0 Auto · 1 Address · 2 Air temp · 3 Humidity · 4 Soil
int slideSec = 4;    // Slide sec in Auto
int backlight = 43;  // Backlight % (keep <=50 — heat)
int ledMode = 2;     // LED:   0 Off · 1 Manual · 2 Per-widget · 3 Gradient
String ledColor = "#22d3ee";

int curSlide = 1, lastSlide = -1, lastMode = -1;
float lastTemp = -999;
int lastHum = -1, lastSoil = -1;

void hueRgb(uint16_t h, uint8_t &r, uint8_t &g, uint8_t &b) {
  h %= 360;
  uint8_t c = 40, x = (uint8_t)(40 * (1 - fabsf(fmodf(h / 60.0f, 2) - 1)));
  if (h < 60) { r = c; g = x; b = 0; }
  else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; }
  else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; }
  else { r = c; g = 0; b = x; }
}

void applyRgb() {
  static uint16_t gh = 0;
  uint8_t r = 0, g = 0, b = 0;
  if (ledMode == 1) {  // manual: the picked colour (dimmed)
    long v = strtol(ledColor.c_str() + (ledColor.startsWith("#") ? 1 : 0), nullptr, 16);
    r = ((v >> 16) & 0xFF) / 5; g = ((v >> 8) & 0xFF) / 5; b = (v & 0xFF) / 5;
  } else if (ledMode == 2) {  // per-widget: colour follows the current slide
    if (curSlide == 2) { r = 8; g = 44; b = 38; }         // temp = teal
    else if (curSlide == 3) { r = 8; g = 24; b = 48; }    // humidity = blue
    else if (curSlide == 4) { r = 22; g = 46; b = 28; }   // soil = green
    else { r = g = b = 22; }                              // address = white
  } else if (ledMode == 3) {  // gradient
    gh = (gh + 2) % 360;
    hueRgb(gh, r, g, b);
  }  // ledMode 0 (Off) -> stays 0,0,0
  lcd::rgb(r, g, b);
}

void setup() {
  Serial.begin(115200);
  delay(120);
  setCpuFrequencyMhz(80);  // less heat; plenty for web + LCD
  lcd::begin();
  lcd::backlight(backlight);

  dash.group("Climate");
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil moisture", &soil, "%");
  dash.group("Control");
  dash.toggle("Pump", &pump, [](bool on) { (void)on; });
  dash.group("Display");
  dash.select("Show", "Auto,Address,Air temp,Humidity,Soil", &dispMode, [](int i) { (void)i; });
  dash.number("Slide sec", &slideSec, 2, 30, 1, [](int i) { (void)i; });
  dash.slider("Backlight", &backlight, 5, 100, [](int i) { (void)i; lcd::backlight(backlight); });
  dash.group("LED");
  dash.select("Mode", "Off,Manual,Per-widget,Gradient", &ledMode, [](int i) { (void)i; applyRgb(); });
  dash.color("Color", &ledColor, [](const String &v) { (void)v; applyRgb(); });
  dash.begin();
  WiFi.setSleep(true);  // Wi-Fi modem sleep — heat/power saver
  Serial.printf("[net] staIP=%s\n", WiFi.localIP().toString().c_str());
  applyRgb();
}

uint32_t lastAnim = 0, lastSwap = 0, lastLed = 0, lastHb = 0;
float ph = 0;
void loop() {
  dash.update();

  if (millis() - lastAnim > 250) {  // sample values (swap for real sensor reads)
    lastAnim = millis();
    ph += 0.18f;
    temp = 24 + 4 * sinf(ph);
    hum = 60 + 9 * sinf(ph * 0.7f);
    soil = 48 + 8 * sinf(ph * 0.5f);
  }

  if (dispMode == 0) {
    uint32_t iv = (uint32_t)(slideSec < 2 ? 2 : slideSec) * 1000;
    if (millis() - lastSwap > iv) { lastSwap = millis(); curSlide = curSlide >= 4 ? 1 : curSlide + 1; }
  } else {
    curSlide = dispMode;
  }

  if (curSlide != lastSlide || dispMode != lastMode) {
    lcd::slideStatic(curSlide, WiFi.localIP().toString(), "RisalDash v" RISALDASH_VERSION);
    lastSlide = curSlide;
    lastMode = dispMode;
    lastTemp = -999;
    lastHum = lastSoil = -1;
    lastSwap = millis();
    if (ledMode == 2) applyRgb();
  }

  if (curSlide == 2 && fabsf(temp - lastTemp) >= 0.1f) { lcd::gaugeValue(temp, 0, 50); lastTemp = temp; }
  else if (curSlide == 3 && (int)hum != lastHum) { lcd::metricValue((int)hum, lcd::C_TEAL); lastHum = (int)hum; }
  else if (curSlide == 4 && soil != lastSoil) { lcd::metricValue(soil, lcd::C_GREEN); lastSoil = soil; }

  if (ledMode == 3 && millis() - lastLed > 60) { lastLed = millis(); applyRgb(); }

  if (millis() - lastHb > 3000) {
    lastHb = millis();
    Serial.printf("[hb] mode=%d slide=%d led=%d heap=%u\n", dispMode, curSlide, ledMode, ESP.getFreeHeap());
  }
  delay(5);
}
