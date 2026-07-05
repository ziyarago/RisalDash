// LCD rendering for the Waveshare ESP32-C6-LCD-1.47 (ST7789 172x320), kept out of the main sketch so
// the .ino stays about the dashboard, not pixels. All drawing lives in the `lcd` namespace.
//
// ⚠ HEAT: never drive the backlight at 100% — it overheats the panel (Waveshare: keep <=50%).
// Use lcd::backlight(pct) with pct <= ~50.
#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

namespace lcd {

// Pins (Waveshare C6-LCD-1.47).
enum { PIN_DC = 15, PIN_CS = 14, PIN_SCK = 7, PIN_MOSI = 6, PIN_RST = 21, PIN_BL = 22, PIN_RGB = 8 };

// Palette (RGB565).
static const uint16_t C_BG = RGB565(20, 26, 40);
static const uint16_t C_CARD = RGB565(32, 40, 58);
static const uint16_t C_TRACK = RGB565(50, 58, 76);
static const uint16_t C_TEAL = RGB565(45, 222, 190);
static const uint16_t C_GREEN = RGB565(120, 230, 150);
static const uint16_t C_INK = RGB565(238, 242, 250);
static const uint16_t C_INK3 = RGB565(120, 132, 150);

static const int GX = 86, GY = 190, GR = 72;  // gauge centre + ring radius

static Arduino_DataBus *_bus = nullptr;
static Arduino_GFX *_gfx = nullptr;

// Thick arc as overlapping dots (reliable angles: 0deg=east, clockwise).
inline void arcSeg(int cx, int cy, float rmid, int dotR, float aStart, float aEnd, uint16_t color) {
  for (float a = aStart; a <= aEnd; a += 3.0f) {
    float rad = a * 0.01745329f;
    _gfx->fillCircle(cx + (int)(rmid * cosf(rad)), cy + (int)(rmid * sinf(rad)), dotR, color);
  }
}

inline void centerText(const char *s, int y, uint8_t size, uint16_t color) {
  _gfx->setTextSize(size);
  _gfx->setTextColor(color);
  _gfx->setCursor(86 - (int)(strlen(s) * 6 * size) / 2, y);
  _gfx->print(s);
}

inline void slideLabel(const char *t, uint16_t dot) {
  int w = strlen(t) * 12;
  int x = 86 - (w + 14) / 2;
  _gfx->fillCircle(x, 61, 3, dot);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + 12, 54);
  _gfx->print(t);
}

inline void carouselDots(int active) {
  int gap = 16, x0 = 86 - (3 * gap) / 2;
  for (int i = 0; i < 4; i++) {
    bool on = (i + 1) == active;
    _gfx->fillRoundRect(x0 + i * gap - (on ? 5 : 3), 306, on ? 12 : 6, 6, 3, on ? C_TEAL : C_TRACK);
  }
}

inline void chrome() {
  _gfx->fillScreen(C_BG);
  centerText("RisalDash", 16, 2, C_INK);
  _gfx->drawFastHLine(40, 40, 92, C_TRACK);
}

inline void begin() {
  _bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
  _gfx = new Arduino_ST7789(_bus, PIN_RST, 0, true, 172, 320, 34, 0, 34, 0);
  _gfx->begin();
  _gfx->fillScreen(C_BG);
}

inline void backlight(int pct) {
  if (pct < 5) pct = 5;
  if (pct > 100) pct = 100;
  analogWrite(PIN_BL, pct * 255 / 100);
}

inline void rgb(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN_RGB, r, g, b); }

// Slides: 1 Address · 2 Air temp gauge · 3 Humidity · 4 Soil.
inline void slideStatic(int s, const String &ip, const char *version) {
  chrome();
  carouselDots(s);
  if (s == 1) {
    slideLabel("ADDRESS", C_TEAL);
    centerText("open in browser", 108, 1, C_INK3);
    _gfx->fillRoundRect(12, 130, 148, 46, 12, C_CARD);
    centerText(ip.c_str(), 147, 2, C_GREEN);
    centerText(version, 284, 1, C_INK3);
  } else if (s == 2) {
    slideLabel("AIR TEMP", C_TEAL);
    arcSeg(GX, GY, GR, 9, 270, 630, C_TRACK);
  } else if (s == 3) {
    slideLabel("HUMIDITY", C_TEAL);
  } else if (s == 4) {
    slideLabel("SOIL MOISTURE", C_GREEN);
  }
}

inline void gaugeValue(float temp, float mn, float mx) {
  float frac = (temp - mn) / (mx - mn);
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  float end = 270 + 360 * frac;  // clockwise from top (matches the web gauge)
  // Draw the grey remainder first, then the teal value on top, so the rounded (capsule) ends belong
  // to the VALUE arc, not the track.
  arcSeg(GX, GY, GR, 9, end, 630, C_TRACK);
  arcSeg(GX, GY, GR, 9, 270, end, C_TEAL);
  _gfx->fillRect(GX - 54, GY - 18, 108, 40, C_BG);
  char b[12];
  snprintf(b, sizeof(b), "%.1f", temp);
  _gfx->setTextSize(4);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(GX - (int)(strlen(b) * 24 + 12) / 2, GY - 16);
  _gfx->print(b);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK3);
  _gfx->print("C");
}

inline void metricValue(int val, uint16_t accent) {
  _gfx->fillRect(0, 96, 172, 130, C_BG);
  char num[6];
  snprintf(num, sizeof(num), "%d", val);
  int nw = strlen(num) * 36;
  int x = 86 - (nw + 6 + 24) / 2;
  _gfx->setTextSize(6);
  _gfx->setTextColor(accent);
  _gfx->setCursor(x, 122);
  _gfx->print(num);
  _gfx->setTextSize(4);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + nw + 6, 146);
  _gfx->print("%");
  _gfx->fillRoundRect(14, 210, 144, 12, 6, C_TRACK);
  _gfx->fillRoundRect(14, 210, 144 * val / 100, 12, 6, accent);
}

}  // namespace lcd
