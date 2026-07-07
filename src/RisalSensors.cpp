// Sensor presets: dash.sensor("id", &vars…) expands a known sensor into the right
// widgets (kind + unit + range per physical quantity).
#include "RisalUI.h"

// ── Sensor presets (quantity-based) ──
// A measure binds the right widget kind + unit + range for a physical quantity; a sensor
// is a composition of measures. dash.sensor("bme280", &t, &h, &p) expands to the proper
// widgets automatically. Curated common set; for an ultra-minimal build use widgets directly.
namespace {
enum RKind { K_METRIC, K_GAUGE, K_CHART };
struct RMeasure { const char* title; const char* unit; RKind kind; float lo; float hi; };
struct RSensor { const char* id; uint8_t n; RMeasure m[4]; };

const RSensor RSENSORS[] = {
  {"bme280", 3, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100},{"Pressure","hPa",K_CHART,300,1100}}},
  {"bmp280", 2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,85},{"Pressure","hPa",K_CHART,300,1100}}},
  {"dht22",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,80},{"Humidity","%",K_METRIC,0,100}}},
  {"dht11",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,0,50},{"Humidity","%",K_METRIC,20,90}}},
  {"sht3x",  2, {{"Temperature","\xC2\xB0""C",K_GAUGE,-40,125},{"Humidity","%",K_METRIC,0,100}}},
  {"ds18b20",1, {{"Temperature","\xC2\xB0""C",K_GAUGE,-55,125}}},
  {"bh1750", 1, {{"Illuminance","lx",K_METRIC,0,65535}}},
  {"ina219", 3, {{"Voltage","V",K_GAUGE,0,26},{"Current","A",K_METRIC,0,3.2f},{"Power","W",K_CHART,0,80}}},
  {"hcsr04", 1, {{"Distance","cm",K_METRIC,0,400}}},
  {"ccs811", 2, {{"CO2","ppm",K_GAUGE,400,8000},{"TVOC","ppb",K_METRIC,0,1100}}},
  {"scd40",  3, {{"CO2","ppm",K_GAUGE,400,5000},{"Temperature","\xC2\xB0""C",K_METRIC,-10,60},{"Humidity","%",K_METRIC,0,100}}},
  // Power pack
  {"acs712",  1, {{"Current","A",K_GAUGE,-30,30}}},
  {"pzem004t",4, {{"Voltage","V",K_GAUGE,0,260},{"Current","A",K_METRIC,0,100},{"Power","W",K_CHART,0,23000},{"Energy","kWh",K_METRIC,0,9999}}},
  // Distance / motion / gas pack
  {"vl53l0x", 1, {{"Distance","mm",K_METRIC,0,2000}}},
  {"mq135",   1, {{"Air quality","ppm",K_GAUGE,0,1000}}},
  {"soil",    1, {{"Soil moisture","%",K_GAUGE,0,100}}},
  // Presence pack (mmWave radar) — human presence, a differentiator vs. other ESP dashboards
  {"ld2410",  3, {{"Presence","",K_METRIC,0,1},{"Distance","cm",K_GAUGE,0,600},{"Motion","%",K_CHART,0,100}}},
  {"ld2450",  2, {{"Targets","",K_METRIC,0,3},{"Distance","cm",K_GAUGE,0,600}}},
  // IMU pack
  {"mpu6050", 3, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2}}},
  {"mpu9250", 3, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2}}},
  // Nav / audio pack
  {"neo-m10", 3, {{"Speed","km/h",K_GAUGE,0,200},{"Altitude","m",K_METRIC,-100,9000},{"Satellites","",K_METRIC,0,24}}},
  {"inmp441", 1, {{"Sound level","dB",K_GAUGE,30,120}}},
};
const uint8_t RSENSOR_COUNT = sizeof(RSENSORS) / sizeof(RSENSORS[0]);
}  // namespace

SensorGroup RisalUI::sensor(const char* id, float* p0, float* p1, float* p2, float* p3) {
  SensorGroup g(this);
  const RSensor* s = nullptr;
  for (uint8_t i = 0; i < RSENSOR_COUNT; i++)
    if (strcmp(id, RSENSORS[i].id) == 0) { s = &RSENSORS[i]; break; }
  if (!s) return g;  // unknown preset -> empty group (chaining is a no-op)
  float* p[4] = {p0, p1, p2, p3};
  for (uint8_t i = 0; i < s->n && i < 4; i++) {
    const RMeasure& m = s->m[i];
    Widget* w = nullptr;
    switch (m.kind) {
      case K_GAUGE: w = &gauge(m.title, p[i], m.lo, m.hi, m.unit); break;
      case K_CHART: w = &chart(m.title, p[i], m.unit); break;
      default:      w = &metric(m.title, p[i], m.unit); break;
    }
    g._w[i] = w;               // remember the readout so .size() / .chart() can act on the group
    g._p[i] = p[i];
    g._title[i] = m.title;
    g._unit[i] = m.unit;
    g._isChart[i] = (m.kind == K_CHART);
    g._n++;
  }
  return g;
}

// Add a history chart for every quantity that isn't already a chart (the readouts stay; the charts
// are appended after them). Bound to the same variables, so they trend live like any dash.chart().
SensorGroup& SensorGroup::chart() {
  for (uint8_t i = 0; i < _n; i++)
    if (!_isChart[i] && _p[i]) _ui->chart(_title[i], _p[i], _unit[i]);
  return *this;
}
