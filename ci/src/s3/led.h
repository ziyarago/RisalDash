// Onboard RGB LED (WS2812, GPIO42) for the Hosyond 2.8" ESP32-S3 module. Same four modes as
// the C6 showcase, driven from the web: Off · Manual (picked colour) · Per-widget (a colour
// per LCD slide) · Gradient (hue sweep). Colours kept dim — the LED is bright at close range.
#pragma once
#include <Arduino.h>
#include <math.h>

namespace led {

enum { PIN = 42 };
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

// Per-widget mode: a distinct dim colour per LCD slide.
inline void slideColor(int slide, uint8_t &r, uint8_t &g, uint8_t &b) {
  switch (slide) {
    case 2:  r = 0;  g = 100; b = 80;  break;  // temp = teal
    case 3:  r = 15; g = 40;  b = 120; break;  // humidity = blue
    case 4:  r = 20; g = 55;  b = 120; break;  // pressure = azure
    case 5:  r = 0;  g = 95;  b = 95;  break;  // trend = cyan
    case 6:  r = 90; g = 30;  b = 90;  break;  // robot = magenta
    case 7:  r = 110; g = 80; b = 10;  break;  // battery = amber
    case 8:  r = 30; g = 120; b = 30;  break;  // overview = green
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
