// Every (fake) data source in one place: the greenhouse emulator, GPS route playback, the
// energy meter, mmWave presence, IMU wobble, the temperature history ring and the thermal
// frame generator. All values live in plain globals the sketch binds to widgets — when real
// hardware arrives, swap the reads inside sampleSensors() for your drivers' and nothing else
// in the sketch changes.
#pragma once
#include <Arduino.h>
#include <RisalUI.h>    // HeatmapWidget
#include <RisalFake.h>  // realistic fake sensors (library) — build/debug with no hardware attached
#include <math.h>

// Latest sensor readings (filled by sampleSensors()).
static RisalFakeEnv envSim;  // realistic emulator — swap for a real AHT20/BMP280 driver here only
static float temp = 24.3f, hum = 62;
static int soil = 48;
static float pres = 1013.0f;  // barometric pressure, hPa (BMP280)
static float light = 0;       // ambient light, lux (follows the emulator's day/night cycle)
static int airq = 0;          // air quality: 0 GOOD · 1 FAIR · 2 POOR
static float presence = 0, presDist = 250, presMotion = 0;  // fake mmWave (LD2410)

// Fake GPS driving the map widget — plays this loop of waypoints (no GPS module needed).
static RisalFakeGPS gps;
static const float ROUTE[] = {41.311f, 69.279f, 41.318f, 69.286f, 41.315f, 69.298f,
                              41.306f, 69.296f, 41.302f, 69.283f, 41.311f, 69.279f};
static float gpsLat = 41.311f, gpsLon = 69.279f, gpsSpeed = 0, gpsHeading = 0;

// Fake energy meter (stand-in for a PZEM-004T): power draw, accumulated kWh and cost.
static RisalFake powerFake(850, 650, 40, 1.3f);  // watts, wandering
static float power = 0, energyKwh = 0, cost = 0;
static int tariff = 12;  // price per kWh, cents — editable on the dashboard

// Fake IMU orientation (degrees) for the 3D cube widget.
static float imuP = 0, imuR = 0, imuY = 0;

// Record & replay the temperature: capture the live signal, then loop it (Replay toggle).
static RisalRecorder rec;
static bool replayMode = false;

// Temperature history for the trend sparkline (newest at the end).
static const int THN = 40;
static float thist[THN] = {0};
static int thCount = 0;

// Fake thermal camera (stand-in for MLX90640) — a moving hotspot over a warm field.
static const int TW = 24, TH = 18;
static float thermal[TW * TH];

inline void sensorsBegin() {
  envSim.begin();
  gps.begin(ROUTE, 6);
}

// Pull the latest readings into the globals (throttled to 4 Hz inside; call from loop()).
// The source (emulator vs real driver) lives entirely here — the sketch stays the same either way.
inline void sampleSensors() {
  static uint32_t last = 0;
  if (millis() - last < 250) return;
  last = millis();
  envSim.update();
  if (replayMode && rec.size() > 8) {
    temp = rec.replay();  // loop the recorded temperature trace
  } else {
    temp = envSim.temperature();
    rec.record(temp);     // capture the live signal
  }
  hum = envSim.humidity();
  soil = envSim.soil();
  pres = envSim.pressure();
  light = envSim.light();
  airq = envSim.airQuality();
  gps.update();
  gpsLat = gps.lat(); gpsLon = gps.lon(); gpsSpeed = gps.speed(); gpsHeading = gps.heading();
  power = powerFake.read();
  if (power < 0) power = 0;
  energyKwh += power / 1000.0f * (0.25f / 3600.0f) * 200.0f;  // 200x speedup so kWh climbs visibly
  cost = energyKwh * tariff / 100.0f;
  presence = ((millis() / 8000) % 2) ? 1 : 0;                 // fake mmWave: person ~every 8 s
  presDist = 200.0f + 130.0f * sinf(millis() * 0.0007f);
  presMotion = presence ? 40.0f + 40.0f * fabsf(sinf(millis() * 0.002f)) : 0.0f;
  imuP = 30.0f * sinf(millis() * 0.0009f);                    // fake IMU for the 3D cube
  imuR = 25.0f * sinf(millis() * 0.0013f);
  imuY = fmodf(millis() * 0.03f, 360.0f);
  for (int i = 0; i < THN - 1; i++) thist[i] = thist[i + 1];  // push temp into the history ring
  thist[THN - 1] = temp;
  if (thCount < THN) thCount++;
}

// Fake thermal frame — a moving hotspot pushed to the web heatmap (throttled to 2 Hz inside).
inline void updateThermal(HeatmapWidget *heat) {
  static uint32_t last = 0;
  if (!heat || millis() - last < 500) return;
  last = millis();
  float ph = millis() * 0.001f;
  float hx = TW / 2.0f + TW / 3.0f * sinf(ph), hy = TH / 2.0f + TH / 3.0f * cosf(ph * 0.7f);
  for (int y = 0; y < TH; y++)
    for (int x = 0; x < TW; x++) {
      float dx = x - hx, dy = y - hy;
      thermal[y * TW + x] = 22.0f + 15.0f * expf(-(dx * dx + dy * dy) / 12.0f) + 1.5f * sinf(x * 0.5f + ph);
    }
  heat->frame(thermal);
}
