// FT6336 capacitive touch (I2C 0x38) for the Hosyond 2.8" module. Reports tap coordinates so the
// UI can hit-test tiles, buttons and sliders — one "down" event per finger press, plus the live
// held position for dragging a slider.
//
// ⚠ Address clash: an external AHT20 (also 0x38) on the same bus corrupts the touch chip's
// replies. Put external 0x38 sensors on a second bus (Wire1, e.g. SDA=21 SCL=14).
#pragma once
#include <Arduino.h>
#include <Wire.h>

namespace touch {

enum { SDA = 16, SCL = 15, RST = 18, INT_PIN = 17, ADDR = 0x38 };

static bool _ok = false;
static bool _down = false;

struct Point { bool tap; bool held; int x, y; };  // tap = first frame of a press; held = still touching

inline bool begin() {
  pinMode(RST, OUTPUT);
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(300);
  Wire.begin(SDA, SCL, 400000);
  Wire.beginTransmission(ADDR);
  Wire.write(0xA6);  // firmware version reg — any ACK'd read means the chip is alive
  _ok = (Wire.endTransmission(false) == 0 && Wire.requestFrom((int)ADDR, 1) == 1);
  if (_ok) Wire.read();
  if (_ok) {
    Wire.beginTransmission(ADDR);  // CTRL=0: stay in active scan (default drops to a slow monitor mode)
    Wire.write(0x86);
    Wire.write(0x00);
    Wire.endTransmission();
  }
  return _ok;
}

// Read the current finger. tap is true only on the first frame of a press; held stays true while
// touching (for slider drags). x in 0..239, y in 0..319 (portrait, matching the display).
inline Point read() {
  if (!_ok) { _down = false; return {false, false, 0, 0}; }
  Wire.beginTransmission(ADDR);
  Wire.write(0x02);  // TD_STATUS then P1 XH/XL/YH/YL
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom((int)ADDR, 5) != 5) { _down = false; return {false, false, 0, 0}; }
  uint8_t n = Wire.read() & 0x0F;
  uint8_t xh = Wire.read(), xl = Wire.read(), yh = Wire.read(), yl = Wire.read();
  if (n == 0 || n > 2) { _down = false; return {false, false, 0, 0}; }
  int x = ((xh & 0x0F) << 8) | xl;
  int y = ((yh & 0x0F) << 8) | yl;
  if (x < 0) x = 0; if (x > 239) x = 239;
  if (y < 0) y = 0; if (y > 319) y = 319;
  bool firstFrame = !_down;
  _down = true;
  return {firstFrame, true, x, y};
}

}  // namespace touch
