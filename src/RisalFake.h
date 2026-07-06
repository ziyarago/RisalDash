// RisalFake.h — realistic fake sensors so you can build and debug a whole dashboard with NO hardware
// attached. Opt-in: #include <RisalFake.h> only in examples/sketches that want it (it adds nothing to
// a normal build otherwise). Works on ESP8266 and ESP32.
//
// Two pieces:
//   RisalFake     — one realistic drifting value; bind it straight to any numeric widget.
//   RisalFakeEnv  — a ready environment bundle (temperature / humidity / pressure / soil / air).
//
// When the real hardware arrives, swap these reads for your driver's (AHT20/BMP280/analog...) with the
// same variable names and nothing else in your sketch changes.
#pragma once
#include <Arduino.h>
#include <math.h>

// One realistic signal: a slow drift + a faster wobble + a little random noise, centred on `center`
// and deviating up to about `amp`. Give each signal its own `phase` so several don't move in lockstep;
// `speed` scales how fast it wanders; `noise` is the jitter amplitude. read() advances the model and
// returns the new value — call it once per tick and store it if you need the value more than once.
class RisalFake {
 public:
  RisalFake(float center, float amp, float noise = 0, float speed = 1, float phase = 0)
      : _c(center), _a(amp), _n(noise), _s(speed), _t(phase) {}
  float read() {
    _t += 0.004f * _s;
    float v = _c + _a * (0.78f * sinf(_t) + 0.22f * sinf(_t * 6.3f));
    return v + _n * ((float)random(-1000, 1001) / 1000.0f);
  }

 private:
  float _c, _a, _n, _s, _t;
};

// A ready greenhouse-style environment with a realistic DAY/NIGHT cycle (Fake Engine 2.0). A virtual
// clock (one day ~3 min so it's visible) drives the readings: temperature peaks in the afternoon,
// humidity runs inverse, light follows the sun (0 at night), soil dries faster by day then gets
// "watered", air quality is worse midday. Call begin() once, update() each tick, then read the getters.
class RisalFakeEnv {
 public:
  void begin() { _soil = 62.0f; }

  void update() {
    _t += 0.033f;  // ~3 min per virtual day at 4 ticks/s
    float hour = fmodf(_t, 24.0f);
    _hour = hour;
    float sun = (hour > 6 && hour < 18) ? sinf(PI * (hour - 6.0f) / 12.0f) : 0;  // daylight 0..1

    _temp = 18.0f + 10.0f * sun + 0.5f * sinf(_t * 3.0f) + noise(0.12f);  // ~18 night .. ~28 noon
    _hum = 72.0f - 32.0f * sun + noise(0.6f);                             // inverse of temperature
    _lux = 95000.0f * sun * sun + noise(120.0f);                          // 0 night .. ~95k lux noon
    _pres = 1013.0f + 5.0f * sinf(_t * 0.05f) + noise(0.15f);             // slow weather drift

    _soil -= 0.02f + 0.06f * sun;      // evaporates faster in daylight...
    if (_soil < 34.0f) _soil = 66.0f;  // ...then an irrigation event tops it up

    int a = (int)(sun * 2.4f);
    _airq = a > 2 ? 2 : a;  // air a bit worse midday
  }

  float temperature() const { return _temp; }  // °C
  float humidity() const { return _hum; }       // %
  float pressure() const { return _pres; }      // hPa
  float light() const { return _lux; }          // lux (follows the sun)
  int soil() const { return (int)_soil; }       // %
  int airQuality() const { return _airq; }      // 0 GOOD · 1 FAIR · 2 POOR
  float hourOfDay() const { return _hour; }     // 0..24 virtual clock

 private:
  float noise(float amp) { return amp * ((float)random(-1000, 1001) / 1000.0f); }
  float _t = 8.0f, _temp = 22, _hum = 58, _pres = 1013, _lux = 0, _soil = 62, _hour = 8;
  int _airq = 0;
};

// Fake GPS — plays a fixed route on a loop (GPX-style), interpolating between waypoints. A stand-in
// for a real GPS module: drive a map widget, or lat/lon/speed stats, with no hardware. Pass a flat
// {lat,lon, lat,lon, ...} array to begin(); call update() each tick, then read the getters.
class RisalFakeGPS {
 public:
  void begin(const float *route, int points) { _r = route; _n = points; }

  void update() {
    if (_n < 2) return;
    _f += 0.02f;  // fraction along the current segment per tick
    if (_f >= 1.0f) { _f -= 1.0f; _seg = (_seg + 1) % (_n - 1); }
    int a = _seg * 2, b = (_seg + 1) * 2;
    _lat = _r[a] + (_r[b] - _r[a]) * _f;
    _lon = _r[a + 1] + (_r[b + 1] - _r[a + 1]) * _f;
    _heading = atan2f(_r[b + 1] - _r[a + 1], _r[b] - _r[a]) * 57.2958f;  // bearing of the segment
    if (_heading < 0) _heading += 360.0f;
    _speed = 32.0f + 14.0f * sinf(_seg + _f);  // km/h, varies along the route
  }

  float lat() const { return _lat; }
  float lon() const { return _lon; }
  float speed() const { return _speed; }    // km/h
  float heading() const { return _heading; }  // degrees, 0 = north

 private:
  const float *_r = nullptr;
  int _n = 0, _seg = 0;
  float _f = 0, _lat = 0, _lon = 0, _speed = 0, _heading = 0;
};

// Fake BLE scanner — a plausible list of nearby devices with wandering RSSI, including a Xiaomi
// temperature/humidity beacon (decoded, like a real scan). A stand-in for NimBLE scanning: build the
// scanner UI with no BLE hardware, then swap in a real scan. Call update(), then read the devices.
class RisalFakeBLE {
 public:
  static const int N = 5;
  void update() {
    _t += 0.25f;
    for (int i = 0; i < N; i++) _rssi[i] = _base[i] + (int)(7.0f * sinf(_t * 0.5f + i));
    _temp = 23.5f + 2.0f * sinf(_t * 0.1f);        // Xiaomi beacon temperature
    _hum = 55 + (int)(6.0f * sinf(_t * 0.08f));    // Xiaomi beacon humidity
  }
  int count() const { return N; }
  const char *name(int i) const { return _names[i]; }
  int rssi(int i) const { return _rssi[i]; }
  bool isSensor(int i) const { return i == 0; }  // index 0 is the Xiaomi temp/humidity beacon
  float sensorTemp() const { return _temp; }     // °C
  int sensorHum() const { return _hum; }         // %

 private:
  const char *_names[N] = {"Mi Temp/Humidity", "MJ_HT_V1", "Amazfit Band 5", "iPhone", "TV Remote"};
  int _base[N] = {-58, -66, -73, -81, -89};
  int _rssi[N] = {-58, -66, -73, -81, -89};
  float _t = 0, _temp = 23.5f;
  int _hum = 55;
};
