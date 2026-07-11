// Data sources for the 2.8" S3 showcase. Fake greenhouse by default (RisalFakeEnv), but a REAL
// BMP280 on the I2C connector (shared bus with the touch, addr 0x77) is auto-detected at boot
// and takes over temperature + pressure — plug it in, reboot, the dashboard is real.
//
// ⚠ AHT20 (the other half of the common AHT20+BMP280 combo) sits at 0x38 — the SAME address as
// this board's FT6336 touch controller. On the shared connector they corrupt each other; wire
// the combo to a second bus instead (Wire1, e.g. SDA=21 SCL=14) if you need humidity too.
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <RisalFake.h>

static RisalFakeEnv envSim;
static float temp = 24.0f, hum = 55;
static float pres = 1013.0f;
static int soil = 60;                 // fake-only (no real probe)
static bool bmpReal = false;          // true once a live BMP280 answered at 0x77

// Temperature history for the trend sparkline (newest at the end).
static const int THN = 48;
static float thist[THN] = {0};
static int thCount = 0;

// ── Minimal raw BMP280 driver (no library): calibration + Bosch compensation ──
namespace bmp {
static const uint8_t ADDR = 0x77;
static uint16_t T1, P1_;
static int16_t T2, T3, P2, P3, P4, P5, P6, P7, P8, P9;

inline uint8_t rd8(uint8_t reg) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)ADDR, 1);
  return Wire.read();
}

inline bool begin() {
  Wire.beginTransmission(ADDR);
  if (Wire.endTransmission() != 0) return false;
  if (rd8(0xD0) != 0x58) return false;  // chip-id: BMP280
  // Read the 24-byte calibration block at 0x88.
  Wire.beginTransmission(ADDR);
  Wire.write(0x88);
  Wire.endTransmission(false);
  Wire.requestFrom((int)ADDR, 24);
  uint8_t c[24];
  for (int i = 0; i < 24; i++) c[i] = Wire.read();
  T1 = c[0] | (c[1] << 8);  T2 = c[2] | (c[3] << 8);   T3 = c[4] | (c[5] << 8);
  P1_ = c[6] | (c[7] << 8); P2 = c[8] | (c[9] << 8);   P3 = c[10] | (c[11] << 8);
  P4 = c[12] | (c[13] << 8); P5 = c[14] | (c[15] << 8); P6 = c[16] | (c[17] << 8);
  P7 = c[18] | (c[19] << 8); P8 = c[20] | (c[21] << 8); P9 = c[22] | (c[23] << 8);
  Wire.beginTransmission(ADDR);  // config: standby 250ms, IIR filter x4
  Wire.write(0xF5); Wire.write(0x50);
  Wire.endTransmission();
  Wire.beginTransmission(ADDR);  // ctrl_meas: temp x2, press x16, normal mode
  Wire.write(0xF4); Wire.write(0x57);
  Wire.endTransmission();
  return true;
}

// Read + compensate (Bosch reference integer math). Returns false on a bus error.
inline bool read(float &tC, float &phPa) {
  Wire.beginTransmission(ADDR);
  Wire.write(0xF7);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom((int)ADDR, 6) != 6) return false;
  uint8_t d[6];
  for (int i = 0; i < 6; i++) d[i] = Wire.read();
  int32_t adcP = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
  int32_t adcT = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
  int32_t v1 = ((((adcT >> 3) - ((int32_t)T1 << 1))) * (int32_t)T2) >> 11;
  int32_t v2 = (((((adcT >> 4) - (int32_t)T1) * ((adcT >> 4) - (int32_t)T1)) >> 12) * (int32_t)T3) >> 14;
  int32_t tFine = v1 + v2;
  tC = ((tFine * 5 + 128) >> 8) / 100.0f;
  int64_t p1 = (int64_t)tFine - 128000;
  int64_t p2 = p1 * p1 * (int64_t)P6;
  p2 += (p1 * (int64_t)P5) << 17;
  p2 += ((int64_t)P4) << 35;
  p1 = ((p1 * p1 * (int64_t)P3) >> 8) + ((p1 * (int64_t)P2) << 12);
  p1 = ((((int64_t)1 << 47) + p1) * (int64_t)P1_) >> 33;
  if (p1 == 0) return false;
  int64_t p = 1048576 - adcP;
  p = (((p << 31) - p2) * 3125) / p1;
  p1 = ((int64_t)P9 * (p >> 13) * (p >> 13)) >> 25;
  p2 = ((int64_t)P8 * p) >> 19;
  p = ((p + p1 + p2) >> 8) + (((int64_t)P7) << 4);
  phPa = (p / 256.0f) / 100.0f;
  return true;
}
}  // namespace bmp

inline void sensorsBegin() {  // call AFTER touch::begin() — they share the bus Wire.begin
  envSim.begin();
  bmpReal = bmp::begin();
}

// Advance the models / read the real sensor (throttled to 4 Hz inside; call from loop()).
inline void sampleSensors() {
  static uint32_t last = 0;
  if (millis() - last < 250) return;
  last = millis();
  envSim.update();
  hum = envSim.humidity();
  soil = envSim.soil();
  if (bmpReal) {
    float t, p;
    if (bmp::read(t, p)) { temp = t; pres = p; }
  } else {
    temp = envSim.temperature();
    pres = envSim.pressure();
  }
  for (int i = 0; i < THN - 1; i++) thist[i] = thist[i + 1];
  thist[THN - 1] = temp;
  if (thCount < THN) thCount++;
}
