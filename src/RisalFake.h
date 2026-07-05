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

// A ready greenhouse-style environment. Call begin() once, update() each sample tick, then read the
// getters. It aims to look measured rather than a clean sine: humidity tracks inversely to
// temperature, soil slowly dries then gets "watered", air quality is mostly good with occasional dips.
class RisalFakeEnv {
 public:
  void begin() { _soil = 62.0f; }

  void update() {
    _t += 0.004f;                  // slow clock: a full "day" cycle every few minutes
    float day = sinf(_t);          // -1 night .. +1 midday
    float wob = sinf(_t * 22.0f);  // faster local wobble

    _temp = 24.0f + 4.0f * day + 0.6f * wob + noise(0.12f);
    _hum = 58.0f - 10.0f * day + 1.5f * sinf(_t * 15.0f) + noise(0.5f);  // inverse of temp
    _pres = 1013.0f + 5.0f * sinf(_t * 0.4f) + noise(0.15f);             // slow weather drift

    _soil -= 0.05f + noise(0.02f);     // dries steadily...
    if (_soil < 34.0f) _soil = 66.0f;  // ...then an irrigation event tops it up

    float aq = sinf(_t * 3.3f) + 0.5f * sinf(_t * 7.7f);  // -1.5 .. 1.5
    _airq = aq > 1.1f ? 2 : (aq > 0.4f ? 1 : 0);          // mostly GOOD, occasional FAIR/POOR
  }

  float temperature() const { return _temp; }  // °C
  float humidity() const { return _hum; }       // %
  float pressure() const { return _pres; }      // hPa
  int soil() const { return (int)_soil; }       // %
  int airQuality() const { return _airq; }      // 0 GOOD · 1 FAIR · 2 POOR

 private:
  float noise(float amp) { return amp * ((float)random(-1000, 1001) / 1000.0f); }
  float _t = 0, _temp = 24, _hum = 58, _pres = 1013, _soil = 62;
  int _airq = 0;
};
