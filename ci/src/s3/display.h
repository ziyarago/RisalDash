// LCD rendering for the Hosyond 2.8" ESP32-S3 touch module (ILI9341 240x320), kept out of the
// main sketch so the .ino stays about the dashboard. All drawing lives in the `lcd` namespace.
// Slides: 1 Address(QR) · 2 Air temp gauge · 3 Humidity · 4 Pressure · 5 Trend · 6 Robot eyes ·
// 7 Battery · 8 Overview grid. The capacitive touch pages through them (see touch.h).
#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <qrcode.h>
#include <math.h>

namespace lcd {

// Pins (lcdwiki 2.8inch_ESP32-S3_Display). Touch is in touch.h, RGB LED in led.h.
enum { PIN_CS = 10, PIN_DC = 46, PIN_SCK = 12, PIN_MOSI = 11, PIN_MISO = 13, PIN_BL = 45 };

// Palette — the RisalDash BRAND palette (same accents as the served web dashboard), ported to
// RGB565, but grounded on PURE BLACK: this cheap TFT washes out the web's graphite #0F1115 into
// mud, so black maximises contrast and the accents pop. Depth still reads through three surface
// levels — black ground -> card #171A21 -> a 1px border #262B36. One accent (aqua) carries the
// hero value; text is soft-white, labels a dim blue-grey; amber/rose only for status.
static const uint16_t C_BG = 0x0000;    // ground — pure black (this panel needs the contrast)
static const uint16_t C_CARD = 0x10C4;  // card — #171A21
static const uint16_t C_LINE = 0x2146;  // 1px border / divider — #262B36 (structure)
static const uint16_t C_TRACK = 0x2146; // slider track base — #262B36
static const uint16_t C_TEAL = 0x269D;  // ACCENT — Aqua #22D3EE (hero value, progress, toggle)
static const uint16_t C_GREEN = 0x3693; // status OK — #34D399
static const uint16_t C_INK = 0xEF5E;   // primary text — #E8EAF0 (soft white)
static const uint16_t C_INK3 = 0x8C94;  // label / muted — #8A93A6
static const uint16_t C_BLUE = 0x3C1E;  // secondary accent — Blue #3B82F6
static const uint16_t C_AMBER = 0xF4E1;  // warn / second accent — #F59E0B
static const uint16_t C_RED = 0xF1EB;    // status error — Rose #F43F5E
static const uint16_t C_LOVE = 0x8AFE;   // violet accent — #8B5CF6 (radio)
static const uint16_t C_YELLOW = 0xF4E1; // = amber #F59E0B

static const int NUM_SLIDES = 8;
static const int CX = 120;                     // screen centre x (240 wide)
static const int GX = 120, GY = 178, GR = 86;  // gauge centre + ring radius

static Arduino_DataBus *_bus = nullptr;
static Arduino_GFX *_gfx = nullptr;

inline void begin() {
  _bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);
  // ips=true: this panel is colour-inverted — without it the dark navy theme renders WHITE.
  _gfx = new Arduino_ILI9341(_bus, GFX_NOT_DEFINED, 0 /* portrait */, true);
  _gfx->begin(60000000);  // 60 MHz SPI — fast full-screen repaints so the UI loop never stalls
  _gfx->fillScreen(C_BG);
}

inline void backlight(int pct) {  // PWM on the BL pin; no heat constraint on this panel
  if (pct < 5) pct = 5;
  if (pct > 100) pct = 100;
  analogWrite(PIN_BL, pct * 255 / 100);
}

// Thick arc as overlapping dots (reliable angles: 0deg=east, clockwise).
inline void arcSeg(int cx, int cy, float rmid, int dotR, float aStart, float aEnd, uint16_t color) {
  for (float a = aStart; a <= aEnd; a += 2.5f) {
    float rad = a * 0.01745329f;
    _gfx->fillCircle(cx + (int)(rmid * cosf(rad)), cy + (int)(rmid * sinf(rad)), dotR, color);
  }
}

inline void centerText(const char *s, int y, uint8_t size, uint16_t color) {
  _gfx->setTextSize(size);
  _gfx->setTextColor(color);
  _gfx->setCursor(CX - (int)(strlen(s) * 6 * size) / 2, y);
  _gfx->print(s);
}

inline void slideLabel(const char *t, uint16_t dot) {
  int w = strlen(t) * 12;
  int x = CX - (w + 14) / 2;
  _gfx->fillCircle(x, 65, 3, dot);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + 12, 58);
  _gfx->print(t);
}

inline void carouselDots(int active) {
  int gap = 200 / NUM_SLIDES, x0 = CX - (gap * (NUM_SLIDES - 1)) / 2;
  for (int i = 0; i < NUM_SLIDES; i++) {
    bool on = (i + 1) == active;
    _gfx->fillRoundRect(x0 + i * gap - (on ? 5 : 3), 306, on ? 10 : 6, 6, 3, on ? C_TEAL : C_TRACK);
  }
}

inline void chrome() {
  _gfx->fillScreen(C_BG);
  centerText("RisalDash", 16, 2, C_INK);
  _gfx->drawFastHLine(74, 40, 92, C_TRACK);
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

// ── Phone-style home: a web-like status bar + a grid of glass cards ───────────────────────────
// Mirrors the served dashboard (OKLCH liquid-glass): a top status bar (Wi-Fi + battery), the
// appbar brand row, then rounded cards with an accent dot + UPPERCASE title + big value.

// Wi-Fi glyph: an apex dot with three clean concentric arcs fanning up over it (dim arcs = weak
// signal). Arcs are drawn as continuous polylines (drawLine between sampled points) so they read
// as thin curves, not a blob. (cx,cy) is the apex/dot position.
inline void wifiGlyph(int cx, int cy, int lvl) {
  for (int ring = 0; ring < 3; ring++) {
    uint16_t col = ring < lvl ? C_INK : C_LINE;
    int r = 3 + ring * 4;
    int px = -999, py = 0;
    for (int a = 235; a <= 305; a += 7) {  // ~70deg fan centred on straight-up (270deg)
      float rad = a * 0.01745329f;
      int x = cx + (int)(r * cosf(rad));
      int y = cy + (int)(r * sinf(rad));
      if (px != -999) _gfx->drawLine(px, py, x, y, col);
      px = x; py = y;
    }
  }
  _gfx->fillCircle(cx, cy, 1, lvl > 0 ? C_INK : C_LINE);  // apex dot
}

// Top status bar — thin (18 px), like the web .rsb: brand left, Wi-Fi glyph + small battery right.
inline void statusBar(int rssi, int batPct) {
  _gfx->fillRect(0, 0, 240, 20, C_BG);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(10, 6);
  _gfx->print("RisalDash");
  int lvl = rssi > -60 ? 3 : rssi > -72 ? 2 : rssi < -100 ? 0 : 1;
  wifiGlyph(186, 10, lvl);
  // Battery capsule — bigger + vertically centred with the Wi-Fi glyph.
  int bx = 200, by = 3, bw = 27, bh = 14;
  _gfx->drawRoundRect(bx, by, bw, bh, 2, C_INK3);
  _gfx->fillRect(bx + bw, by + 4, 2, 6, C_INK3);
  int fw = (bw - 4) * (batPct < 0 ? 0 : batPct > 100 ? 100 : batPct) / 100;
  if (fw > 0) _gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 1, batPct <= 20 ? C_RED : C_GREEN);
}

// One glass card frame with an accent dot + UPPERCASE title. Value area is cleared/filled by cardValue.
inline void card(int x, int y, int w, int h, const char *title, uint16_t dot) {
  _gfx->fillRoundRect(x, y, w, h, 14, C_CARD);
  _gfx->drawRoundRect(x, y, w, h, 14, C_LINE);
  _gfx->fillCircle(x + 15, y + 17, 3, dot);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + 24, y + 14);
  _gfx->print(title);
}

// A card's big value + small unit, cleared each update so digits don't ghost.
inline void cardValue(int x, int y, int w, const char *val, const char *unit, uint16_t accent, uint8_t size = 4) {
  _gfx->fillRect(x + 6, y + 30, w - 12, 40, C_CARD);
  _gfx->setTextSize(size);
  _gfx->setTextColor(accent);
  _gfx->setCursor(x + 14, y + 36);
  _gfx->print(val);
  int vw = strlen(val) * 6 * size;
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + 16 + vw, y + 36 + (size - 1) * 8);
  _gfx->print(unit);
}

// Home card grid geometry: 2 columns × 2 rows of small cards below the status/appbar.
static const int HOME_N = 4;
static const int HX[HOME_N] = {10, 125, 10, 125};
static const int HY[HOME_N] = {66, 66, 164, 164};
static const int HW = 105, HH = 92;

// Draw the home once (status bar + brand + 4 empty cards). Values arrive via homeValue().
inline void homeStatic(int rssi, int batPct) {
  _gfx->fillScreen(C_BG);
  statusBar(rssi, batPct);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(12, 38);
  _gfx->print("Sensors");
  const char *titles[HOME_N] = {"AIR TEMP", "HUMIDITY", "PRESSURE", "BATTERY"};
  const uint16_t dots[HOME_N] = {C_TEAL, C_BLUE, C_AMBER, C_GREEN};
  for (int i = 0; i < HOME_N; i++) card(HX[i], HY[i], HW, HH, titles[i], dots[i]);
  // A hint that this pages to the detail carousel.
  centerText("swipe / tap for detail", 292, 1, C_INK3);
}

inline void homeValue(int i, const char *val, const char *unit, uint16_t accent) {
  if (i < 0 || i >= HOME_N) return;
  cardValue(HX[i], HY[i], HW, val, unit, accent, 4);
}

// Static chrome of each slide (labels, tracks); the live value goes on top via *Value() below.
inline void slideStatic(int s, const String &ip, const char *version) {
  chrome();
  carouselDots(s);
  switch (s) {
    // s == 1 is the phone-style card home — drawn by the sketch via homeStatic(), not here.
    case 2: slideLabel("AIR TEMP", C_TEAL); arcSeg(GX, GY, GR, 10, 270, 630, C_TRACK); break;
    case 3: slideLabel("HUMIDITY", C_BLUE); break;
    case 4: slideLabel("PRESSURE", C_BLUE); break;
    case 5: slideLabel("TEMP TREND", C_TEAL); break;
    case 6: slideLabel("ROBOT", C_TEAL); break;
    case 7: slideLabel("BATTERY", C_AMBER); break;
    case 8:
      slideLabel("SCAN TO OPEN", C_TEAL);
      drawQR("http://" + ip, CX, 168, 5);
      centerText(ip.c_str(), 258, 2, C_GREEN);
      centerText(version, 286, 1, C_INK3);
      break;
  }
}

inline void gaugeValue(float temp, float mn, float mx) {
  float frac = (temp - mn) / (mx - mn);
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  float end = 270 + 360 * frac;  // clockwise from top (matches the web gauge)
  arcSeg(GX, GY, GR, 10, end, 630, C_TRACK);
  arcSeg(GX, GY, GR, 10, 270, end, C_TEAL);
  _gfx->fillRect(GX - 62, GY - 20, 124, 44, C_BG);
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

// Big % + progress bar (humidity, battery...).
inline void metricValue(int val, uint16_t accent) {
  _gfx->fillRect(0, 100, 240, 150, C_BG);
  char num[6];
  snprintf(num, sizeof(num), "%d", val);
  int nw = strlen(num) * 36;
  int x = CX - (nw + 6 + 24) / 2;
  _gfx->setTextSize(6);
  _gfx->setTextColor(accent);
  _gfx->setCursor(x, 136);
  _gfx->print(num);
  _gfx->setTextSize(4);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + nw + 6, 160);
  _gfx->print("%");
  _gfx->fillRoundRect(30, 230, 180, 14, 7, C_TRACK);
  int w = 180 * val / 100;
  if (w > 0) _gfx->fillRoundRect(30, 230, w, 14, 7, accent);
}

// Big number + unit, no bar (pressure hPa...).
inline void statValue(const char *num, const char *unit, uint16_t accent) {
  _gfx->fillRect(0, 100, 240, 150, C_BG);
  int nw = strlen(num) * 30;
  _gfx->setTextSize(5);
  _gfx->setTextColor(accent);
  _gfx->setCursor(CX - nw / 2, 150);
  _gfx->print(num);
  centerText(unit, 210, 2, C_INK3);
}

// Sparkline: history[0..n) scaled to [mn,mx], latest value on top.
inline void chartValue(const float *h, int n, float mn, float mx, uint16_t accent) {
  const int x0 = 20, y0 = 130, w = 200, ht = 120;
  _gfx->fillRect(0, 96, 240, 170, C_BG);
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
      _gfx->drawLine(px, py + 1, x, y + 1, accent);
    }
    px = x;
    py = y;
  }
  if (n) _gfx->fillCircle(px, py, 3, C_INK);
  char b[12];
  snprintf(b, sizeof(b), "%.1f", n ? h[n - 1] : 0.0f);
  _gfx->setTextSize(3);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(CX - (int)(strlen(b) * 18) / 2, 98);
  _gfx->print(b);
}

// Robot eyes — same moods as the web face widget (0..9), scaled for the 240-wide panel.
inline void eyes(int mood, bool blink) {
  _gfx->fillRect(12, 100, 216, 106, C_BG);   // inside the face panel, above the emotion chips
  const int ex[2] = {75, 165}, cy = 150;
  uint16_t c = (mood == 6) ? C_LOVE : C_TEAL;
  for (int e = 0; e < 2; e++) {
    int cx = ex[e];
    if (mood == 9) cx += (int)(14 * sinf(millis() * 0.005f));
    if (blink) { _gfx->fillRoundRect(cx - 26, cy - 5, 52, 10, 5, c); continue; }
    switch (mood) {
      case 4: _gfx->fillCircle(cx, cy, 32, c); break;                               // surprised
      case 5: _gfx->fillRoundRect(cx - 26, cy - 6, 52, 14, 7, c); break;            // sleepy
      case 1:                                                                        // happy
        _gfx->fillRoundRect(cx - 26, cy - 30, 52, 60, 18, c);
        _gfx->fillCircle(cx, cy + 36, 36, C_BG);
        break;
      case 2:                                                                        // sad
        _gfx->fillRoundRect(cx - 24, cy - 22, 48, 54, 16, c);
        _gfx->fillCircle(cx + (e ? 24 : -24), cy - 30, 32, C_BG);
        break;
      case 3:                                                                        // angry
        _gfx->fillRoundRect(cx - 26, cy - 30, 52, 60, 18, c);
        _gfx->fillCircle(cx + (e ? -26 : 26), cy - 38, 36, C_BG);
        break;
      case 6:                                                                        // love
        _gfx->fillCircle(cx - 11, cy - 9, 14, c);
        _gfx->fillCircle(cx + 11, cy - 9, 14, c);
        _gfx->fillTriangle(cx - 24, cy - 2, cx + 24, cy - 2, cx, cy + 26, c);
        break;
      case 7:                                                                        // wink
        if (e) _gfx->fillRoundRect(cx - 26, cy - 5, 52, 10, 5, c);
        else _gfx->fillRoundRect(cx - 26, cy - 32, 52, 64, 18, c);
        break;
      case 8:                                                                        // dizzy
        _gfx->drawCircle(cx, cy, 25, c);
        _gfx->drawCircle(cx, cy, 17, c);
        _gfx->drawCircle(cx, cy, 9, c);
        break;
      default:                                                                       // neutral / look
        _gfx->fillRoundRect(cx - 26, cy - 32, 52, 64, 18, c);
        break;
    }
    if (mood == 0 || mood == 4) _gfx->fillCircle(cx - 9, cy - 16, 5, C_INK);  // catch-light
  }
}

// Battery slide: big % + a battery glyph filled proportionally.
inline void batteryValue(int pct, bool charging) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  _gfx->fillRect(0, 96, 240, 170, C_BG);
  char b[8];
  snprintf(b, sizeof(b), "%d%%", pct);
  centerText(b, 120, 4, pct <= 20 ? C_RED : C_GREEN);
  // battery outline + fill
  _gfx->drawRoundRect(60, 180, 110, 52, 8, C_INK3);
  _gfx->fillRoundRect(170, 194, 10, 24, 3, C_INK3);
  int w = 102 * pct / 100;
  if (w > 0) _gfx->fillRoundRect(64, 184, w, 44, 5, pct <= 20 ? C_RED : C_GREEN);
  if (charging) centerText("USB", 246, 2, C_INK3);
}

// ══════════ Phone-style app shell — a launcher of mini-apps in the web design language ══════════

// App-bar shared by every app screen: back chevron (left) + title + Wi-Fi/battery (right).
inline void appbar(const char *title, int rssi, int batPct) {
  _gfx->fillRect(0, 0, 240, 32, C_BG);
  _gfx->fillRoundRect(8, 5, 22, 22, 7, C_CARD);
  _gfx->drawRoundRect(8, 5, 22, 22, 7, C_LINE);
  // back chevron
  _gfx->drawLine(22, 11, 15, 16, C_INK);
  _gfx->drawLine(15, 16, 22, 21, C_INK);
  _gfx->drawLine(21, 11, 14, 16, C_INK);
  _gfx->drawLine(14, 16, 21, 21, C_INK);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(40, 9);
  _gfx->print(title);
  int lvl = rssi > -60 ? 3 : rssi > -72 ? 2 : rssi < -100 ? 0 : 1;
  wifiGlyph(184, 15, lvl);
  int bx = 198, by = 8, bw = 27, bh = 14;  // centred on the app-bar's 32px height
  _gfx->drawRoundRect(bx, by, bw, bh, 2, C_INK3);
  _gfx->fillRect(bx + bw, by + 4, 2, 6, C_INK3);
  int fw = (bw - 4) * (batPct < 0 ? 0 : batPct > 100 ? 100 : batPct) / 100;
  if (fw > 0) _gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 1, batPct <= 20 ? C_RED : C_GREEN);
}
inline bool backHit(int x, int y) { return x < 40 && y < 34; }

// Small mono-line icons drawn with primitives, tinted by the app accent.
inline void icon(int kind, int cx, int cy, uint16_t c) {
  switch (kind) {
    case 0:  // monitor — a pulse/activity line
      _gfx->drawLine(cx - 12, cy, cx - 6, cy, c);
      _gfx->drawLine(cx - 6, cy, cx - 2, cy - 8, c);
      _gfx->drawLine(cx - 2, cy - 8, cx + 3, cy + 8, c);
      _gfx->drawLine(cx + 3, cy + 8, cx + 7, cy - 3, c);
      _gfx->drawLine(cx + 7, cy - 3, cx + 12, cy, c);
      break;
    case 1:  // radio — concentric broadcast arcs + dot
      _gfx->fillCircle(cx, cy, 2, c);
      _gfx->drawCircle(cx, cy, 6, c);
      _gfx->drawCircle(cx, cy, 11, c);
      break;
    case 2:  // robot — rounded head + two eyes
      _gfx->drawRoundRect(cx - 11, cy - 8, 22, 16, 5, c);
      _gfx->fillRect(cx - 5, cy - 2, 3, 5, c);
      _gfx->fillRect(cx + 3, cy - 2, 3, 5, c);
      _gfx->drawFastVLine(cx, cy - 13, 5, c);
      break;
    case 3:  // settings — gear (ring + ticks)
      _gfx->drawCircle(cx, cy, 6, c);
      _gfx->fillCircle(cx, cy, 2, c);
      for (int a = 0; a < 360; a += 60) {
        float r = a * 0.01745329f;
        _gfx->drawLine(cx + (int)(7 * cosf(r)), cy + (int)(7 * sinf(r)),
                       cx + (int)(11 * cosf(r)), cy + (int)(11 * sinf(r)), c);
      }
      break;
  }
}

// ── Launcher (home) ──
static const int TILE_X[4] = {10, 125, 10, 125};
static const int TILE_Y[4] = {70, 70, 186, 186};
static const int TILE_W = 105, TILE_H = 104;

inline void launcher(int rssi, int batPct, const char *version) {
  _gfx->fillScreen(C_BG);
  statusBar(rssi, batPct);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(12, 32);
  _gfx->print("HOME");
  _gfx->setTextSize(3);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(12, 44);
  _gfx->print("Touch 2.8");
  const char *nm[4] = {"Monitor", "Radio", "Robot", "Settings"};
  const char *sub[4] = {"4 sensors", "internet", "companion", "display Wi-Fi"};
  const uint16_t col[4] = {C_TEAL, C_LOVE, C_GREEN, C_AMBER};
  for (int i = 0; i < 4; i++) {
    int x = TILE_X[i], y = TILE_Y[i];
    _gfx->fillRoundRect(x, y, TILE_W, TILE_H, 16, C_CARD);
    _gfx->drawRoundRect(x, y, TILE_W, TILE_H, 16, C_LINE);
    _gfx->fillRoundRect(x + 12, y + 12, 32, 32, 10, C_BG);  // icon plate
    icon(i, x + 28, y + 28, col[i]);
    _gfx->setTextSize(2);
    _gfx->setTextColor(C_INK);
    _gfx->setCursor(x + 12, y + 56);
    _gfx->print(nm[i]);
    _gfx->setTextSize(1);
    _gfx->setTextColor(C_INK3);
    _gfx->setCursor(x + 12, y + 80);
    _gfx->print(sub[i]);
  }
  centerText(version, 306, 1, C_INK3);
}
// Which tile was tapped (0..3) or -1.
inline int launcherHit(int x, int y) {
  for (int i = 0; i < 4; i++)
    if (x >= TILE_X[i] && x < TILE_X[i] + TILE_W && y >= TILE_Y[i] && y < TILE_Y[i] + TILE_H) return i;
  return -1;
}

// ── Monitor: a row list (label · big value · unit · source). Values always fit. ──
static const int MON_N = 4;
static const int MON_Y[MON_N] = {40, 108, 176, 244};
static const int MON_H = 62;
struct MonRow { const char *label; const char *source; uint16_t accent; };

inline void monitorStatic(int rssi, int batPct, const MonRow *rows) {
  _gfx->fillScreen(C_BG);
  appbar("Monitor", rssi, batPct);
  for (int i = 0; i < MON_N; i++) {
    int y = MON_Y[i];
    _gfx->fillRoundRect(10, y, 220, MON_H, 14, C_CARD);
    _gfx->drawRoundRect(10, y, 220, MON_H, 14, C_LINE);
    _gfx->fillCircle(24, y + 16, 3, rows[i].accent);
    _gfx->setTextSize(1);
    _gfx->setTextColor(C_INK3);
    _gfx->setCursor(34, y + 13);
    _gfx->print(rows[i].label);
    _gfx->setCursor(20, y + 44);
    _gfx->print(rows[i].source);
  }
}
// Live value on row i (cleared each update). value right-aligned, unit small.
inline void monitorValue(int i, const char *val, const char *unit, uint16_t accent) {
  int y = MON_Y[i];
  _gfx->fillRect(120, y + 6, 104, 34, C_CARD);
  int vw = strlen(val) * 18;        // size 3 ~ 18px/glyph
  int uw = strlen(unit) * 6;
  int x = 224 - 8 - uw - vw;
  _gfx->setTextSize(3);
  _gfx->setTextColor(accent);
  _gfx->setCursor(x, y + 12);
  _gfx->print(val);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(x + vw + 4, y + 24);
  _gfx->print(unit);
}

// ── Radio: now-playing + volume + station list ──
static const int STN_Y0 = 134, STN_H = 27, STN_GAP = 3;  // compact rows: fits 6 stations to y=311
static const int STN_MAX = 6;
// When the list overflows STN_MAX rows a ▲/▼ pager appears in a right-hand gutter; rows shrink to
// leave room for it. scroll = index of the first visible station.
static const int STN_RW = 220;         // full row width (no pager)
static const int STN_RW_S = 196;        // row width when the pager gutter is shown
static const int PGR_X = 210;           // pager column left edge
inline void radioStatic(int rssi, int batPct, const char *station, const char *meta,
                        const char *const *names, int n, int cur, int volPct, bool playing, int scroll) {
  _gfx->fillScreen(C_BG);
  appbar("Radio", rssi, batPct);
  // now-playing card
  _gfx->fillRoundRect(10, 38, 220, 56, 16, C_CARD);
  _gfx->drawRoundRect(10, 38, 220, 56, 16, C_LINE);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_INK);
  _gfx->setCursor(22, 50);
  _gfx->print(station);
  _gfx->setTextSize(1);
  _gfx->setTextColor(C_LOVE);
  _gfx->setCursor(22, 74);
  _gfx->print(meta);
  // play/stop indicator on the card's right edge: green triangle = live, two grey bars = stopped
  if (playing) { _gfx->fillTriangle(202, 52, 202, 68, 216, 60, C_GREEN); }
  else { _gfx->fillRoundRect(203, 52, 4, 16, 2, C_INK3); _gfx->fillRoundRect(211, 52, 4, 16, 2, C_INK3); }
  // volume bar
  _gfx->fillRoundRect(22, 108, 196, 6, 3, C_TRACK);
  int vw = 196 * (volPct < 0 ? 0 : volPct > 100 ? 100 : volPct) / 100;
  if (vw > 0) _gfx->fillRoundRect(22, 108, vw, 6, 3, C_TEAL);
  _gfx->fillCircle(22 + vw, 111, 6, C_INK);
  // station list — a window of up to STN_MAX rows starting at `scroll`
  bool pager = n > STN_MAX;
  int rw = pager ? STN_RW_S : STN_RW;
  if (scroll < 0) scroll = 0;
  for (int slot = 0; slot < STN_MAX; slot++) {
    int i = scroll + slot;
    if (i >= n) break;
    int y = STN_Y0 + slot * (STN_H + STN_GAP);
    bool on = i == cur;
    _gfx->fillRoundRect(10, y, rw, STN_H, 9, on ? C_CARD : C_BG);
    if (on) _gfx->drawRoundRect(10, y, rw, STN_H, 9, C_LOVE);
    _gfx->fillRoundRect(17, y + 5, 18, 17, 5, on ? C_LOVE : C_CARD);
    _gfx->setTextSize(1);
    _gfx->setTextColor(on ? C_BG : C_LOVE);
    _gfx->setCursor(i + 1 >= 10 ? 20 : 23, y + 9);
    _gfx->print(i + 1);
    _gfx->setTextSize(2);
    _gfx->setTextColor(C_INK);
    _gfx->setCursor(44, y + 6);
    _gfx->print(names[i]);
  }
  // pager: ▲ (top) / ▼ (bottom) chevrons, dimmed at the ends of the range
  if (pager) {
    int listBot = STN_Y0 + STN_MAX * (STN_H + STN_GAP) - STN_GAP;
    int midY = (STN_Y0 + listBot) / 2;
    int cx = PGR_X + 10;
    uint16_t up = scroll > 0 ? C_INK : C_LINE;
    uint16_t dn = (scroll + STN_MAX) < n ? C_INK : C_LINE;
    _gfx->fillTriangle(cx - 6, STN_Y0 + 18, cx + 6, STN_Y0 + 18, cx, STN_Y0 + 8, up);      // ▲
    _gfx->fillTriangle(cx - 6, listBot - 18, cx + 6, listBot - 18, cx, listBot - 8, dn);   // ▼
    // faint scroll track + thumb between the chevrons
    int trTop = STN_Y0 + 26, trBot = listBot - 26, trH = trBot - trTop;
    _gfx->drawFastVLine(cx, trTop, trH, C_LINE);
    int thH = trH * STN_MAX / n; if (thH < 8) thH = 8;
    int thY = trTop + (trH - thH) * scroll / (n - STN_MAX);
    _gfx->fillRoundRect(cx - 1, thY, 3, thH, 1, C_INK3);
    (void)midY;
  }
}
// Returns the visible row slot (0..STN_MAX-1) under y, or -1. Excludes the pager gutter.
inline int radioStationHit(int x, int y) {
  if (x >= PGR_X) return -1;
  for (int slot = 0; slot < STN_MAX; slot++) {
    int ry = STN_Y0 + slot * (STN_H + STN_GAP);
    if (y >= ry && y < ry + STN_H) return slot;
  }
  return -1;
}
// Pager hit: 0 = scroll up (▲), 1 = scroll down (▼), -1 = neither. Caller checks the list overflows.
inline int radioScrollHit(int x, int y) {
  if (x < PGR_X) return -1;
  int listBot = STN_Y0 + STN_MAX * (STN_H + STN_GAP) - STN_GAP;
  int midY = (STN_Y0 + listBot) / 2;
  if (y >= STN_Y0 && y < midY) return 0;
  if (y >= midY && y <= listBot) return 1;
  return -1;
}
inline bool radioVolHit(int x, int y) { return y >= 104 && y <= 126; }
inline int radioVolPct(int x) { int p = (x - 22) * 100 / 196; return p < 0 ? 0 : p > 100 ? 100 : p; }

// ── Settings: brightness slider · LED mode segments · Forget Wi-Fi ──
inline void seg3(int x, int y, int w, const char *a, const char *b, const char *c, int on) {
  int sw = w / 3;
  const char *t[3] = {a, b, c};
  for (int i = 0; i < 3; i++) {
    _gfx->fillRoundRect(x + i * sw + 1, y, sw - 2, 26, 7, i == on ? C_TEAL : C_CARD);
    _gfx->setTextSize(1);
    _gfx->setTextColor(i == on ? C_BG : C_INK3);
    _gfx->setCursor(x + i * sw + sw / 2 - strlen(t[i]) * 3, y + 9);
    _gfx->print(t[i]);
  }
}
inline void settingsStatic(int rssi, int batPct, int backlight, int ledMode) {
  _gfx->fillScreen(C_BG);
  appbar("Settings", rssi, batPct);
  // Brightness
  _gfx->setTextSize(1); _gfx->setTextColor(C_INK3);
  _gfx->setCursor(14, 46); _gfx->print("BRIGHTNESS");
  _gfx->fillRoundRect(14, 62, 212, 6, 3, C_TRACK);
  int bw = 212 * backlight / 100;
  if (bw > 0) _gfx->fillRoundRect(14, 62, bw, 6, 3, C_AMBER);
  _gfx->fillCircle(14 + bw, 65, 7, C_INK);
  // LED mode
  _gfx->setTextColor(C_INK3);
  _gfx->setCursor(14, 96); _gfx->print("LED MODE");
  seg3(14, 110, 212, "OFF", "MANUAL", "GRADIENT", ledMode == 0 ? 0 : ledMode == 1 ? 1 : 2);
  // Forget Wi-Fi
  _gfx->fillRoundRect(14, 250, 212, 40, 12, C_BG);
  _gfx->drawRoundRect(14, 250, 212, 40, 12, C_RED);
  _gfx->setTextSize(2);
  _gfx->setTextColor(C_RED);
  _gfx->setCursor(38, 262);
  _gfx->print("Forget Wi-Fi");
}
inline bool setBrightHit(int x, int y) { return y >= 54 && y <= 76; }
inline int setBrightPct(int x) { int p = (x - 14) * 100 / 212; return p < 5 ? 5 : p > 100 ? 100 : p; }
inline int setLedHit(int x, int y) { if (y >= 110 && y <= 136) return (x - 14) * 3 / 212; return -1; }
inline bool setForgetHit(int x, int y) { return y >= 250 && y <= 290 && x >= 14 && x <= 226; }

// ── Robot: animated eyes + a grid of emotion chips ──
static const int ROBO_N = 6;
static const int ROBO_MOOD[ROBO_N] = {0, 1, 5, 6, 7, 8};  // maps chip -> eyes() mood
static const char *const ROBO_NAME[ROBO_N] = {"Neutral", "Happy", "Sleepy", "Love", "Wink", "Dizzy"};
static const int CHIP_Y0 = 234, CHIP_H = 34, CHIP_GAP = 8;

// Voice status strip at the top of the face panel (above the eyes at y100). Redrawn live during a
// voice turn without clearing the whole screen.
inline void robotStatus(const char *s, uint16_t col) {
  _gfx->fillRect(12, 44, 216, 42, C_BG);
  centerText(s, 58, 2, col);
}
inline void robotStatic(int rssi, int batPct, int mood) {
  _gfx->fillScreen(C_BG);
  appbar("Robot", rssi, batPct);
  // face panel (eyes are drawn on top by eyes(), which clears y 96..266)
  _gfx->drawRoundRect(10, 40, 220, 168, 16, C_LINE);
  robotStatus("Tap face to talk", C_INK3);   // hint: the robot is the voice assistant
  // emotion chips: 3 columns x 2 rows
  int cw = (220 - 2 * CHIP_GAP) / 3;
  for (int i = 0; i < ROBO_N; i++) {
    int col = i % 3, row = i / 3;
    int x = 10 + col * (cw + CHIP_GAP), y = CHIP_Y0 + row * (CHIP_H + CHIP_GAP);
    bool on = ROBO_MOOD[i] == mood;
    _gfx->fillRoundRect(x, y, cw, CHIP_H, 10, on ? C_TEAL : C_CARD);
    if (!on) _gfx->drawRoundRect(x, y, cw, CHIP_H, 10, C_LINE);
    _gfx->setTextSize(1);
    _gfx->setTextColor(on ? C_BG : C_INK3);
    _gfx->setCursor(x + cw / 2 - strlen(ROBO_NAME[i]) * 3, y + 13);
    _gfx->print(ROBO_NAME[i]);
  }
}
inline int robotChipHit(int x, int y) {
  int cw = (220 - 2 * CHIP_GAP) / 3;
  for (int i = 0; i < ROBO_N; i++) {
    int col = i % 3, row = i / 3;
    int cx = 10 + col * (cw + CHIP_GAP), cy = CHIP_Y0 + row * (CHIP_H + CHIP_GAP);
    if (x >= cx && x < cx + cw && y >= cy && y < cy + CHIP_H) return i;
  }
  return -1;
}

}  // namespace lcd
