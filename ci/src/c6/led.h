// Onboard RGB LED (WS2812) for the Waveshare ESP32-C6-LCD-1.47, kept out of the main sketch so the
// .ino stays about the dashboard. Four modes, driven from the web:
//   Off · Manual (a picked colour) · Per-widget (a colour per LCD slide) · Gradient (a hue sweep).
// Colours are kept dim on purpose — the onboard LED is bright at close range.
#pragma once
#include <Arduino.h>
#include <math.h>

namespace led {

enum { PIN = 8 };  // onboard WS2812 data pin (Waveshare C6-LCD-1.47)
enum Mode { OFF = 0, MANUAL = 1, PER_WIDGET = 2, GRADIENT = 3 };

inline void write(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN, r, g, b); }

// hue (0..359) -> a dim RGB, used for the gradient sweep.
inline void hue(uint16_t h, uint8_t &r, uint8_t &g, uint8_t &b) {
  h %= 360;
  uint8_t c = 40, x = (uint8_t)(40 * (1 - fabsf(fmodf(h / 60.0f, 2) - 1)));
  if (h < 60) { r = c; g = x; b = 0; }
  else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; }
  else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; }
  else { r = c; g = 0; b = x; }
}

// Per-widget mode: a distinct dim colour per LCD slide (1 Address .. 7 Trend).
inline void slideColor(int slide, uint8_t &r, uint8_t &g, uint8_t &b) {
  switch (slide) {
    case 2:  r = 0;  g = 100; b = 80;  break;  // temp = teal
    case 3:  r = 15; g = 40;  b = 120; break;  // humidity = blue
    case 4:  r = 30; g = 120; b = 30;  break;  // soil = green
    case 5:  r = 20; g = 55;  b = 120; break;  // pressure = azure
    case 6:  r = 60; g = 110; b = 20;  break;  // air quality = lime
    case 7:  r = 0;  g = 95;  b = 95;  break;  // trend = cyan
    default: r = 90; g = 60;  b = 110; break;  // address = violet
  }
}

// Resolve the mode to a colour and write it. `hexColor` feeds Manual ("#rrggbb"); `slide` feeds
// Per-widget; Gradient advances its own hue on each call. Off leaves the LED dark.
inline void apply(int mode, const String &hexColor, int slide) {
  static uint16_t gh = 0;
  uint8_t r = 0, g = 0, b = 0;
  if (mode == MANUAL) {
    long v = strtol(hexColor.c_str() + (hexColor.startsWith("#") ? 1 : 0), nullptr, 16);
    r = ((v >> 16) & 0xFF) / 5;
    g = ((v >> 8) & 0xFF) / 5;
    b = (v & 0xFF) / 5;
  } else if (mode == PER_WIDGET) {
    slideColor(slide, r, g, b);
  } else if (mode == GRADIENT) {
    gh = (gh + 2) % 360;
    hue(gh, r, g, b);
  }
  write(r, g, b);
}

}  // namespace led
