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

// Record & replay — capture a real signal once, then loop it back forever with no hardware. record()
// a live value each tick while capturing (fills up to CAP samples, ~30 s at 4 Hz); replay() reads the
// recording back on a loop. Great for reproducing a real sensor trace in a demo or a test.
class RisalRecorder {
 public:
  static const int CAP = 120;
  void record(float v) { if (_n < CAP) _buf[_n++] = v; }  // append until full
  void reset() { _n = 0; _r = 0; }
  bool full() const { return _n >= CAP; }
  int size() const { return _n; }
  float replay() {
    if (_n == 0) return 0;
    float v = _buf[_r % _n];
    _r++;
    return v;
  }

 private:
  float _buf[CAP];
  int _n = 0;
  uint32_t _r = 0;
};

// Shared jitter helper for the sensor-bundle fakes below.
static inline float rfNoise(float amp) { return amp * ((float)random(-1000, 1001) / 1000.0f); }

// ── Fake power meter (pzem004t / hlw8012 / ina226 / ct-clamp) — a mains circuit whose load cycles
// as "appliances" switch on/off. Energy accumulates realistically. Call update() ~20x/s for smooth
// power; the kWh total assumes ~50 ms ticks (scale energyPerTick if you tick differently).
class RisalFakePower {
 public:
  void update() {
    _t += 0.05f;
    float base = 120.0f + 700.0f * (0.5f + 0.5f * sinf(_t * 0.11f));   // slow base load
    float appliance = (fmodf(_t, 9.0f) < 2.0f) ? 1600.0f : 0.0f;      // kettle/oven bursts
    _w = base + appliance + rfNoise(25.0f);
    _v = 230.0f + 3.0f * sinf(_t * 0.7f) + rfNoise(0.8f);
    _pf = 0.88f + 0.08f * sinf(_t * 0.2f);
    _a = _w / (_v * _pf);
    _hz = 50.0f + 0.04f * sinf(_t * 0.5f);
    _wh += _w * (0.05f / 3600.0f);   // W * hours-per-tick
  }
  float voltage() const { return _v; }       // V
  float current() const { return _a; }        // A
  float power() const { return _w; }           // W
  float energy() const { return _wh / 1000.0f; }  // kWh (accumulating)
  float powerFactor() const { return _pf; }
  float frequency() const { return _hz; }      // Hz
 private:
  float _t = 0, _v = 230, _a = 0, _w = 0, _pf = 0.9f, _hz = 50, _wh = 0;
};

// ── Fake air-quality station (sen55 / pms5003 / scd40 / sgp4x) — a room that fills with CO2/VOC and
// particulates while "occupied", then a ventilation event clears it. All quantities are correlated.
class RisalFakeAir {
 public:
  void update() {
    _t += 0.02f;
    float occ = 0.5f + 0.5f * sinf(_t * 0.09f);        // occupancy 0..1
    bool vent = fmodf(_t, 20.0f) < 3.0f;               // periodic airing
    float f = vent ? 0.25f : 1.0f;
    _co2 = 420.0f + 1300.0f * occ * f + rfNoise(15.0f);
    _tvoc = 40.0f + 600.0f * occ * f + rfNoise(10.0f);
    _pm25 = 5.0f + 45.0f * occ * f + rfNoise(1.5f);
    _pm10 = _pm25 * 1.5f + rfNoise(2.0f);
    _pm1 = _pm25 * 0.7f;
    _pm4 = _pm25 * 1.25f;
    _voc = 100.0f + 250.0f * occ * f;                  // VOC index 0..500
    _nox = 1.0f + 60.0f * occ * f;                     // NOx index
    _hcho = 5.0f + 40.0f * occ * f;                    // formaldehyde ppb
    _temp = 22.0f + 2.0f * occ + rfNoise(0.1f);
    _hum = 45.0f + 12.0f * occ + rfNoise(0.5f);
  }
  float co2() const { return _co2; }       // ppm
  float tvoc() const { return _tvoc; }      // ppb
  float pm1() const { return _pm1; }        // µg/m³
  float pm25() const { return _pm25; }
  float pm4() const { return _pm4; }
  float pm10() const { return _pm10; }
  float vocIndex() const { return _voc; }
  float noxIndex() const { return _nox; }
  float formaldehyde() const { return _hcho; }  // ppb
  float temperature() const { return _temp; }    // °C
  float humidity() const { return _hum; }         // %
 private:
  float _t = 0, _co2 = 420, _tvoc = 40, _pm1 = 4, _pm25 = 6, _pm4 = 7, _pm10 = 9,
        _voc = 100, _nox = 1, _hcho = 5, _temp = 22, _hum = 45;
};

// ── Fake IMU (mpu6050 / mpu9250 / bno055 / qmc5883l) — a slowly tumbling board: gravity on the accel
// axes, angular rate on the gyro, integrated pitch/roll/yaw, a wandering compass heading, step count.
class RisalFakeIMU {
 public:
  void update() {
    _t += 0.04f;
    _pitch = 22.0f * sinf(_t * 0.6f);
    _roll = 30.0f * sinf(_t * 0.43f + 1.0f);
    _yaw = fmodf(_yaw + 1.4f, 360.0f);
    float pr = _pitch * 0.01745f, rr = _roll * 0.01745f;
    _ax = -sinf(pr) + rfNoise(0.02f);
    _ay = sinf(rr) * cosf(pr) + rfNoise(0.02f);
    _az = cosf(rr) * cosf(pr) + rfNoise(0.02f);
    _gx = 13.0f * cosf(_t * 0.6f) + rfNoise(1.0f);
    _gy = 12.9f * cosf(_t * 0.43f + 1.0f) + rfNoise(1.0f);
    _gz = 80.0f + rfNoise(1.0f);
    _heading = _yaw;
    if (fmodf(_t, 0.8f) < 0.04f) _steps++;
  }
  float accelX() const { return _ax; }  float accelY() const { return _ay; }  float accelZ() const { return _az; }
  float gyroX() const { return _gx; }   float gyroY() const { return _gy; }   float gyroZ() const { return _gz; }
  float pitch() const { return _pitch; } float roll() const { return _roll; } float yaw() const { return _yaw; }
  float heading() const { return _heading; }  // ° (compass)
  int steps() const { return _steps; }
 private:
  float _t = 0, _ax = 0, _ay = 0, _az = 1, _gx = 0, _gy = 0, _gz = 0,
        _pitch = 0, _roll = 0, _yaw = 0, _heading = 0;
  int _steps = 0;
};

// ── Fake load cell (hx711 / nau7802) — settles onto a target weight with a little drift; every few
// seconds an "item" is added or removed so the scale looks alive. read() returns kg.
class RisalFakeWeight {
 public:
  void update() {
    _t += 0.05f;
    if (fmodf(_t, 6.0f) < 0.05f) _target = (float)random(0, 25000) / 1000.0f;  // new item 0..25 kg
    _w += (_target - _w) * 0.08f;                                              // settle toward it
    _w += rfNoise(0.004f);
  }
  float weight() const { return _w < 0 ? 0 : _w; }  // kg
  float grams() const { return weight() * 1000.0f; }
 private:
  float _t = 0, _w = 0, _target = 0;
};

// ── Fake weather station (anemometer + vane + tipping-bucket + UV) — gusty wind, slowly veering
// direction, occasional rain showers that accumulate, UV that follows a day cycle.
class RisalFakeWeather {
 public:
  void update() {
    _t += 0.03f;
    float base = 8.0f + 10.0f * (0.5f + 0.5f * sinf(_t * 0.06f));
    _wind = base + fabsf(rfNoise(6.0f));
    _gust = _wind + fabsf(rfNoise(12.0f));
    _dir = fmodf(_dir + 0.6f + rfNoise(2.0f), 360.0f);
    bool shower = fmodf(_t, 30.0f) < 6.0f;
    if (shower) _rain += 0.04f + fabsf(rfNoise(0.03f));
    float hour = fmodf(_t * 0.4f, 24.0f);
    float sun = (hour > 6 && hour < 18) ? sinf(PI * (hour - 6.0f) / 12.0f) : 0;
    _uv = 11.0f * sun * sun + rfNoise(0.1f);
  }
  float windSpeed() const { return _wind; }   // km/h
  float gust() const { return _gust; }          // km/h
  float direction() const { return _dir; }       // ° (0=N)
  float rain() const { return _rain; }            // mm (accumulating)
  float uvIndex() const { return _uv < 0 ? 0 : _uv; }
 private:
  float _t = 0, _wind = 8, _gust = 8, _dir = 180, _rain = 0, _uv = 0;
};

// ── Fake battery / BMS (daly / jk) — discharges under load then recharges; pack voltage follows a
// Li-ion-ish SoC curve, current flips sign on charge, temperature rises a little under load.
class RisalFakeBattery {
 public:
  void update() {
    _t += 0.05f;
    _charging = sinf(_t * 0.05f) > 0;              // alternate charge/discharge phases
    _soc += _charging ? 0.05f : -0.04f;
    if (_soc > 100) _soc = 100; if (_soc < 5) _soc = 5;
    _v = 3.0f + 1.2f * (_soc / 100.0f) + rfNoise(0.01f);  // ~3.0..4.2 per cell * (assume ~13S here)
    _packV = _v * 13.0f;
    _a = _charging ? 18.0f + rfNoise(1.0f) : -(22.0f + rfNoise(1.5f));
    _temp = 26.0f + (_charging ? 4.0f : 8.0f) * (1.0f - _soc / 100.0f) + rfNoise(0.2f);
  }
  float soc() const { return _soc; }           // %
  float packVoltage() const { return _packV; }  // V
  float cellVoltage() const { return _v; }       // V
  float current() const { return _a; }            // A (+charge / -discharge)
  float temperature() const { return _temp; }      // °C
  bool charging() const { return _charging; }
 private:
  float _t = 0, _soc = 72, _v = 3.8f, _packV = 49, _a = -10, _temp = 27;
  bool _charging = false;
};

// ── Fake flow meter (YF-S201 pulse flow) — flow turns on/off; rate in L/min, a running total, and
// the raw pulse frequency (≈7.5 Hz per L/min for this sensor).
class RisalFakeFlow {
 public:
  void update() {
    _t += 0.05f;
    bool on = fmodf(_t, 12.0f) < 7.0f;
    float target = on ? (9.0f + 4.0f * sinf(_t * 0.3f)) : 0.0f;
    _rate += (target - _rate) * 0.15f + rfNoise(0.05f);
    if (_rate < 0) _rate = 0;
    _total += _rate * (0.05f / 60.0f);   // L
    _freq = _rate * 7.5f;
    _count += (uint32_t)(_freq * 0.05f);
  }
  float rate() const { return _rate; }        // L/min
  float total() const { return _total; }        // L (accumulating)
  float frequency() const { return _freq; }      // Hz
  float count() const { return (float)_count; }
 private:
  float _t = 0, _rate = 0, _total = 0, _freq = 0;
  uint32_t _count = 0;
};

// ── Fake pulse-oximeter (max30102) — a resting heart rate with beat-to-beat variability and a
// healthy SpO2 that dips slightly now and then.
class RisalFakeHealth {
 public:
  void update() {
    _t += 0.05f;
    _hr = 72.0f + 8.0f * sinf(_t * 0.15f) + rfNoise(1.2f);
    _spo2 = 98.0f - 1.5f * (0.5f + 0.5f * sinf(_t * 0.07f)) + rfNoise(0.2f);
  }
  float heartRate() const { return _hr; }   // bpm
  float spo2() const { return _spo2; }        // %
 private:
  float _t = 0, _hr = 72, _spo2 = 98;
};

// ── Fake gas sensor (mq-2 / mq-7 / mq-9) — mostly clean air with the occasional spike over the
// alarm threshold, so you can build and test a gas-alarm UI. read() is ppm; alarm() is the trip.
class RisalFakeGas {
 public:
  explicit RisalFakeGas(float threshold = 400.0f) : _th(threshold) {}
  void update() {
    _t += 0.05f;
    bool leak = fmodf(_t, 25.0f) < 4.0f;                 // periodic "leak" event
    float target = leak ? (700.0f + 300.0f * sinf(_t)) : (80.0f + 40.0f * sinf(_t * 0.2f));
    _ppm += (target - _ppm) * 0.1f + rfNoise(5.0f);
    if (_ppm < 0) _ppm = 0;
  }
  float ppm() const { return _ppm; }
  bool alarm() const { return _ppm > _th; }
 private:
  float _t = 0, _ppm = 80, _th = 400;
};
