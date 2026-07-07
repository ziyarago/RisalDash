// LCD rendering for the Waveshare ESP32-C6-LCD-1.47 (ST7789 172x320), kept out of the main sketch so
// the .ino stays about the dashboard, not pixels. All drawing lives in the `lcd` namespace.
//
// ⚠ HEAT: never drive the backlight at 100% — it overheats the panel (Waveshare: keep <=50%).
// Use lcd::backlight(pct) with pct <= ~50.
#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <qrcode.h>
#include <math.h>

namespace lcd {

// Pins (Waveshare C6-LCD-1.47). The onboard RGB LED (GPIO8) is handled in led.h.
enum { PIN_DC = 15, PIN_CS = 14, PIN_SCK = 7, PIN_MOSI = 6, PIN_RST = 21, PIN_BL = 22 };

// Palette (RGB565).
static const uint16_t C_BG = RGB565(20, 26, 40);
static const uint16_t C_CARD = RGB565(32, 40, 58);
static const uint16_t C_TRACK = RGB565(50, 58, 76);
static const uint16_t C_TEAL = RGB565(45, 222, 190);
static const uint16_t C_GREEN = RGB565(120, 230, 150);
static const uint16_t C_INK = RGB565(238, 242, 250);
static const uint16_t C_INK3 = RGB565(120, 132, 150);
static const uint16_t C_BLUE = RGB565(110, 170, 250);
static const uint16_t C_AMBER = RGB565(240, 190, 90);
static const uint16_t C_RED = RGB565(240, 120, 110);
static const uint16_t C_LOVE = RGB565(255, 92, 138);

static const int NUM_SLIDES = 13;             // …Weather · Pump(LED) · Backlight(progress) · Overview(multi) · Thermal(heatmap)
static const int GX = 86, GY = 190, GR = 72;  // gauge centre + ring radius

// ── LCD localization ─────────────────────────────────────────────────────────
// The stock Arduino_GFX 5x7 font is Latin-only: it cannot draw Cyrillic or Arabic
// glyphs (those need a bundled font pack + RTL shaping). So the LCD follows the
// device language for Latin scripts (English, Oʻzbek) and falls back to English
// for ru/ar. The *web* dashboard still serves all four languages in full.
enum Lang { LANG_EN, LANG_UZ };
static Lang _lang = LANG_EN;

inline void setLang(const char *code) {  // pass the device language ("en"/"ru"/"uz"/"ar")
  _lang = (code && code[0] == 'u' && code[1] == 'z') ? LANG_UZ : LANG_EN;
}

struct Tr { const char *en; const char *uz; };
inline const char *tr(const Tr &t) { return _lang == LANG_UZ ? t.uz : t.en; }

// Slide labels — keep SHORT & UPPERCASE (panel is 172 px, ~14 chars at size 2).
static const Tr T_SCAN     = {"SCAN TO OPEN", "OCHISH UCHUN"};
static const Tr T_AIRTEMP  = {"AIR TEMP", "HARORAT"};
static const Tr T_HUMIDITY = {"HUMIDITY", "NAMLIK"};
static const Tr T_SOIL     = {"SOIL MOISTURE", "TUPROQ NAMI"};
static const Tr T_PRESSURE = {"PRESSURE", "BOSIM"};
static const Tr T_AIRQ     = {"AIR QUALITY", "HAVO SIFATI"};
static const Tr T_TREND    = {"TEMP TREND", "TREND"};
static const Tr T_ROBOT    = {"ROBOT", "ROBOT"};
static const Tr T_WEATHER  = {"WEATHER", "OB-HAVO"};
static const Tr T_CONNECT  = {"connecting...", "ulanmoqda..."};
static const Tr T_PUMP      = {"PUMP", "NASOS"};
static const Tr T_BACKLIGHT = {"BACKLIGHT", "YORQINLIK"};
static const Tr T_OVERVIEW  = {"OVERVIEW", "UMUMIY"};
static const Tr T_THERMAL   = {"THERMAL", "TERMAL"};

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
  int gap = 148 / NUM_SLIDES, x0 = 86 - (gap * (NUM_SLIDES - 1)) / 2;
  for (int i = 0; i < NUM_SLIDES; i++) {
    bool on = (i + 1) == active;
    _gfx->fillRoundRect(x0 + i * gap - (on ? 5 : 3), 306, on ? 10 : 6, 6, 3, on ? C_TEAL : C_TRACK);
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

// QR code (dark modules on a white quiet-zone) so a phone camera opens the dashboard URL.
inline void drawQR(const String &text, int cx, int cy, int scale) {
  const int ver = 3;  // 29x29 — fits http://<ip>[:port]
  QRCode qr;
  uint8_t data[qrcode_getBufferSize(ver)];
  qrcode_initText(&qr, data, ver, ECC_LOW, text.c_str());
  int px = qr.size * scale, x0 = cx - px / 2, y0 = cy - px / 2, q = scale * 3;
  _gfx->fillRect(x0 - q, y0 - q, px + 2 * q, px + 2 * q, RGB565_WHITE);
  for (uint8_t y = 0; y < qr.size; y++)
    for (uint8_t x = 0; x < qr.size; x++)
      if (qrcode_getModule(&qr, x, y)) _gfx->fillRect(x0 + x * scale, y0 + y * scale, scale, scale, RGB565_BLACK);
}

// Slides: 1 Address · 2 Air temp gauge · 3 Humidity · 4 Soil.
inline void slideStatic(int s, const String &ip, const char *version) {
  chrome();
  carouselDots(s);
  if (s == 1) {
    slideLabel(tr(T_SCAN), C_TEAL);
    drawQR("http://" + ip, 86, 146, 4);
    centerText(ip.c_str(), 236, 2, C_GREEN);  // IP bigger + prominent
    centerText(version, 274, 1, C_INK3);       // copyright raised so the dots don't overlap it
  } else if (s == 2) {
    slideLabel(tr(T_AIRTEMP), C_TEAL);
    arcSeg(GX, GY, GR, 9, 270, 630, C_TRACK);
  } else if (s == 3) {
    slideLabel(tr(T_HUMIDITY), C_TEAL);
  } else if (s == 4) {
    slideLabel(tr(T_SOIL), C_GREEN);
  } else if (s == 5) {
    slideLabel(tr(T_PRESSURE), C_BLUE);
  } else if (s == 6) {
    slideLabel(tr(T_AIRQ), C_GREEN);
  } else if (s == 7) {
    slideLabel(tr(T_TREND), C_TEAL);
  } else if (s == 8) {
    slideLabel(tr(T_ROBOT), C_TEAL);
  } else if (s == 9) {
    slideLabel(tr(T_WEATHER), C_BLUE);
  } else if (s == 10) {
    slideLabel(tr(T_PUMP), C_TEAL);
  } else if (s == 11) {
    slideLabel(tr(T_BACKLIGHT), C_AMBER);
  } else if (s == 12) {
    slideLabel(tr(T_OVERVIEW), C_TEAL);
  } else if (s == 13) {
    slideLabel(tr(T_THERMAL), C_RED);
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

// Big number + unit, no bar — for a plain reading like pressure (hPa) or CO2 (ppm).
inline void statValue(const char *num, const char *unit, uint16_t accent) {
  _gfx->fillRect(0, 96, 172, 130, C_BG);
  int nw = strlen(num) * 30;  // text size 5 -> ~30px per glyph
  _gfx->setTextSize(5);
  _gfx->setTextColor(accent);
  _gfx->setCursor(86 - nw / 2, 138);
  _gfx->print(num);
  centerText(unit, 192, 2, C_INK3);
}

// Status pill (0 GOOD / 1 FAIR / 2 POOR) — a coloured capsule with dark centred text.
inline void badgeValue(int level) {
  static const char *TXT[] = {"GOOD", "FAIR", "POOR"};
  const uint16_t COL[] = {C_GREEN, C_AMBER, C_RED};
  if (level < 0) level = 0;
  if (level > 2) level = 2;
  _gfx->fillRect(0, 96, 172, 130, C_BG);
  _gfx->fillRoundRect(26, 148, 120, 48, 24, COL[level]);
  const char *s = TXT[level];
  int tw = strlen(s) * 18;  // size 3 -> ~18px per glyph
  _gfx->setTextSize(3);
  _gfx->setTextColor(C_BG);
  _gfx->setCursor(86 - tw / 2, 160);
  _gfx->print(s);
}

// Sparkline: connect history[0..n) scaled to [mn,mx], with a baseline and the latest value on top.
inline void chartValue(const float *h, int n, float mn, float mx, uint16_t accent) {
  const int x0 = 14, y0 = 120, w = 144, ht = 96;
  _gfx->fillRect(0, 96, 172, 140, C_BG);
  _gfx->drawFastHLine(x0, y0 + ht, w, C_TRACK);
  float rng = mx - mn;
  if (rng < 1e-3f) rng = 1;
  int px = 0, py = 0;
  for (int i = 0; i < n; i++) {
    float f = (h[i] - mn) / rng;
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    int x = x0 + (n > 1 ? w * i / (n - 1) : 0);
    int y = y0 + ht - (int)(f * ht);
    if (i) {
      _gfx->drawLine(px, py, x, y, accent);
      _gfx->drawLine(px, py + 1, x, y + 1, accent);  // 2px for visibility
    }
    px = x;
    py = y;
  }
  if (n) _gfx->fillCircle(px, py, 3, C_INK);
  char b[12];
  snprintf(b, sizeof(b), "%.1f", n ? h[n - 1] : 0.0f);
  _gfx->setTextSize(3);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(86 - (int)(strlen(b) * 18) / 2, 96);
  _gfx->print(b);
}

// Robot face: two glowing eyes showing an emotion (0 Neutral · 1 Happy · 2 Sad · 3 Angry · 4 Surprised
// · 5 Sleepy · 6 Love · 7 Wink · 8 Dizzy · 9 Look), or blinked (thin bars). Carves shapes with C_BG
// circles, same idea as the web widget. Call each tick with the current mood + a blink flag.
inline void eyes(int mood, bool blink) {
  _gfx->fillRect(0, 96, 172, 130, C_BG);
  const int ex[2] = {56, 116}, cy = 158;
  uint16_t c = (mood == 6) ? C_LOVE : C_TEAL;
  for (int e = 0; e < 2; e++) {
    int cx = ex[e];
    if (mood == 9) cx += (int)(12 * sinf(millis() * 0.005f));  // look — drift left/right
    if (blink) { _gfx->fillRoundRect(cx - 22, cy - 4, 44, 8, 4, c); continue; }
    switch (mood) {
      case 4:  // surprised — big round
        _gfx->fillCircle(cx, cy, 27, c);
        break;
      case 5:  // sleepy — half-closed bar
        _gfx->fillRoundRect(cx - 22, cy - 5, 44, 12, 6, c);
        break;
      case 1:  // happy — carve the bottom into a smile
        _gfx->fillRoundRect(cx - 22, cy - 26, 44, 52, 16, c);
        _gfx->fillCircle(cx, cy + 30, 30, C_BG);
        break;
      case 2:  // sad — carve the top-outer corner
        _gfx->fillRoundRect(cx - 20, cy - 18, 40, 46, 14, c);
        _gfx->fillCircle(cx + (e ? 20 : -20), cy - 26, 28, C_BG);
        break;
      case 3:  // angry — carve the top-inner corner (brow slant)
        _gfx->fillRoundRect(cx - 22, cy - 26, 44, 52, 16, c);
        _gfx->fillCircle(cx + (e ? -22 : 22), cy - 32, 30, C_BG);
        break;
      case 6:  // love — a heart (two lobes + a point)
        _gfx->fillCircle(cx - 9, cy - 8, 12, c);
        _gfx->fillCircle(cx + 9, cy - 8, 12, c);
        _gfx->fillTriangle(cx - 20, cy - 2, cx + 20, cy - 2, cx, cy + 22, c);
        break;
      case 7:  // wink — right eye closed, left open
        if (e) _gfx->fillRoundRect(cx - 22, cy - 4, 44, 8, 4, c);
        else _gfx->fillRoundRect(cx - 22, cy - 28, 44, 56, 16, c);
        break;
      case 8:  // dizzy — hollow swirl rings
        _gfx->drawCircle(cx, cy, 21, c);
        _gfx->drawCircle(cx, cy, 14, c);
        _gfx->drawCircle(cx, cy, 7, c);
        break;
      case 9:  // look — neutral eye, drifting (cx offset above)
        _gfx->fillRoundRect(cx - 22, cy - 28, 44, 56, 16, c);
        break;
      default:  // neutral — tall rounded
        _gfx->fillRoundRect(cx - 22, cy - 28, 44, 56, 16, c);
        break;
    }
    if (mood == 0 || mood == 4) _gfx->fillCircle(cx - 8, cy - 14, 4, C_INK);  // catch-light shine
  }
}

// Weather slide: big temperature + the sky condition, from the background weather task. Shows a
// placeholder until the first successful fetch (valid == false).
inline void weatherValue(float temp, const char *city, const char *desc, bool valid) {
  _gfx->fillRect(0, 96, 172, 132, C_BG);
  centerText(valid ? city : "...", 104, 2, C_INK);  // resolved city name
  char b[8];
  if (valid) snprintf(b, sizeof(b), "%.0f", temp);
  else strcpy(b, "--");
  int nw = strlen(b) * 30;  // size-5 glyph ~30px wide
  int x = 86 - (nw + 20) / 2;
  _gfx->setTextSize(5);
  _gfx->setTextColor(C_BLUE);
  _gfx->setCursor(x, 146);
  _gfx->print(b);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + nw + 4, 148);
  _gfx->print("C");
  centerText(valid ? desc : tr(T_CONNECT), 206, 2, C_INK3);
}

// ── More display widgets (read-only styles that suit a small non-touch panel) ──

// On/off indicator: a big glowing dot (lit accent when on, hollow track when off) + ON/OFF.
inline void ledValue(bool on, uint16_t accent) {
  _gfx->fillRect(0, 96, 172, 150, C_BG);  // tall enough to also clear the ON/OFF label below
  _gfx->fillCircle(86, 148, 46, C_CARD);  // base plate
  if (on) {
    _gfx->fillCircle(86, 148, 34, accent);
  } else {
    _gfx->fillCircle(86, 148, 34, C_TRACK);
    _gfx->fillCircle(86, 148, 24, C_CARD);  // hollow ring
  }
  centerText(on ? "ON" : "OFF", 210, 3, on ? accent : C_INK3);
}

// Progress: a percentage over a thick rounded bar (brightness, fill level, battery…).
inline void progressValue(int pct, uint16_t accent) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  _gfx->fillRect(0, 96, 172, 132, C_BG);
  char b[6];
  snprintf(b, sizeof(b), "%d%%", pct);
  centerText(b, 128, 4, accent);
  _gfx->fillRoundRect(14, 190, 144, 24, 12, C_TRACK);
  _gfx->fillRoundRect(14, 190, 144 * pct / 100, 24, 12, accent);
}

// Several readings on one screen — a compact stat grid (label + value cards).
struct MiniCell { const char *label; const char *value; uint16_t accent; };
inline void multiValue(const MiniCell *cells, int n) {
  _gfx->fillRect(0, 78, 172, 210, C_BG);
  const int top = 84, rowH = 46, gap = 6;
  for (int i = 0; i < n && i < 4; i++) {
    int y = top + i * (rowH + gap);
    _gfx->fillRoundRect(12, y, 148, rowH, 12, C_CARD);
    _gfx->setTextSize(1);
    _gfx->setTextColor(C_INK3);
    _gfx->setCursor(24, y + 9);
    _gfx->print(cells[i].label);
    _gfx->setTextSize(3);
    _gfx->setTextColor(cells[i].accent);
    _gfx->setCursor(24, y + 20);
    _gfx->print(cells[i].value);
  }
}

// 0..255 -> RGB565 on a jet colormap (blue -> cyan -> green -> yellow -> red).
inline uint16_t jet(uint8_t v) {
  float t = v / 255.0f;
  int r = (int)(255 * constrain(1.5f - fabsf(4 * t - 3), 0.0f, 1.0f));
  int g = (int)(255 * constrain(1.5f - fabsf(4 * t - 2), 0.0f, 1.0f));
  int b = (int)(255 * constrain(1.5f - fabsf(4 * t - 1), 0.0f, 1.0f));
  return RGB565(r, g, b);
}

// Thermal camera / any 2D field: a jet-coloured grid (e.g. MLX90640 24x18) mapped to [mn,mx].
inline void heatmapValue(const float *buf, int cols, int rows, float mn, float mx) {
  int cell = 144 / cols;
  if (cell < 1) cell = 1;
  int gw = cell * cols, gh = cell * rows;
  int x0 = 86 - gw / 2, y0 = 96 + (140 - gh) / 2;
  float rng = mx - mn;
  if (rng < 1e-3f) rng = 1;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++) {
      float f = (buf[r * cols + c] - mn) / rng;
      if (f < 0) f = 0;
      if (f > 1) f = 1;
      _gfx->fillRect(x0 + c * cell, y0 + r * cell, cell, cell, jet((uint8_t)(f * 255)));
    }
}

}  // namespace lcd
