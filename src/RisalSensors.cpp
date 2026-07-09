// Sensor presets: dash.sensor("id", &vars…) expands a known sensor into the right
// widgets (kind + unit + range per physical quantity). Pair each preset with a RisalFake* bundle
// (RisalFake.h) and you can build a full dashboard for any of these with NO hardware attached.
#include "RisalUI.h"

// ── Sensor presets (quantity-based) ──
// A measure binds the right widget kind + unit + range for a physical quantity; a sensor is a
// composition of up to 8 measures. Kinds: metric (compact number), gauge (dial+range), chart (trend),
// stat (big number — totals/counts). dash.sensor("bme280", &t, &h, &p) expands automatically.
namespace {
enum RKind { K_METRIC, K_GAUGE, K_CHART, K_STAT };
struct RMeasure { const char* title; const char* unit; RKind kind; float lo; float hi; };
struct RSensor { const char* id; uint8_t n; RMeasure m[8]; };

#define DEG "\xC2\xB0""C"

const RSensor RSENSORS[] = {
  // ── Climate / environment ──
  {"bme280", 3, {{"Temperature",DEG,K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100},{"Pressure","hPa",K_CHART,300,1100}}},
  {"bme680", 4, {{"Temperature",DEG,K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100},{"Pressure","hPa",K_METRIC,300,1100},{"Gas","k\xCE\xA9",K_CHART,0,500}}},
  {"bmp280", 2, {{"Temperature",DEG,K_GAUGE,-40,85},{"Pressure","hPa",K_CHART,300,1100}}},
  {"bmp388", 3, {{"Temperature",DEG,K_GAUGE,-40,85},{"Pressure","hPa",K_METRIC,300,1250},{"Altitude","m",K_CHART,-500,9000}}},
  {"dht22",  2, {{"Temperature",DEG,K_GAUGE,-40,80},{"Humidity","%",K_METRIC,0,100}}},
  {"dht11",  2, {{"Temperature",DEG,K_GAUGE,0,50},{"Humidity","%",K_METRIC,20,90}}},
  {"aht20",  2, {{"Temperature",DEG,K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100}}},
  {"sht3x",  2, {{"Temperature",DEG,K_GAUGE,-40,125},{"Humidity","%",K_METRIC,0,100}}},
  {"sht4x",  2, {{"Temperature",DEG,K_GAUGE,-40,125},{"Humidity","%",K_METRIC,0,100}}},
  {"si7021", 2, {{"Temperature",DEG,K_GAUGE,-40,85},{"Humidity","%",K_METRIC,0,100}}},
  {"htu21d", 2, {{"Temperature",DEG,K_GAUGE,-40,125},{"Humidity","%",K_METRIC,0,100}}},
  {"ds18b20",1, {{"Temperature",DEG,K_GAUGE,-55,125}}},
  {"mlx90614",2,{{"Object",DEG,K_GAUGE,-70,380},{"Ambient",DEG,K_METRIC,-40,125}}},
  {"bh1750", 1, {{"Illuminance","lx",K_METRIC,0,65535}}},

  // ── Air quality (CO2 / VOC / particulate) ──
  {"scd40",  3, {{"CO2","ppm",K_GAUGE,400,5000},{"Temperature",DEG,K_METRIC,-10,60},{"Humidity","%",K_METRIC,0,100}}},
  {"scd41",  3, {{"CO2","ppm",K_GAUGE,400,5000},{"Temperature",DEG,K_METRIC,-10,60},{"Humidity","%",K_METRIC,0,100}}},
  {"mh-z19", 1, {{"CO2","ppm",K_GAUGE,400,5000}}},
  {"ccs811", 2, {{"eCO2","ppm",K_GAUGE,400,8000},{"TVOC","ppb",K_METRIC,0,1100}}},
  {"sgp30",  2, {{"eCO2","ppm",K_GAUGE,400,60000},{"TVOC","ppb",K_METRIC,0,60000}}},
  {"sgp40",  1, {{"VOC index","",K_GAUGE,0,500}}},
  {"sgp41",  2, {{"VOC index","",K_GAUGE,0,500},{"NOx index","",K_METRIC,0,500}}},
  {"ens160", 3, {{"eCO2","ppm",K_GAUGE,400,5000},{"TVOC","ppb",K_METRIC,0,65000},{"AQI","",K_METRIC,1,5}}},
  {"sfa30",  3, {{"Formaldehyde","ppb",K_GAUGE,0,1000},{"Temperature",DEG,K_METRIC,-40,85},{"Humidity","%",K_METRIC,0,100}}},
  {"pms5003",3, {{"PM1.0","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,500},{"PM2.5","\xC2\xB5g/m\xC2\xB3",K_GAUGE,0,500},{"PM10","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,500}}},
  {"sds011", 2, {{"PM2.5","\xC2\xB5g/m\xC2\xB3",K_GAUGE,0,500},{"PM10","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,500}}},
  {"sps30",  4, {{"PM1.0","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000},{"PM2.5","\xC2\xB5g/m\xC2\xB3",K_GAUGE,0,1000},{"PM4.0","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000},{"PM10","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000}}},
  {"sen55",  8, {{"PM1.0","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000},{"PM2.5","\xC2\xB5g/m\xC2\xB3",K_GAUGE,0,1000},{"PM4.0","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000},{"PM10","\xC2\xB5g/m\xC2\xB3",K_METRIC,0,1000},{"VOC index","",K_METRIC,0,500},{"NOx index","",K_METRIC,0,500},{"Temperature",DEG,K_METRIC,-40,85},{"Humidity","%",K_METRIC,0,100}}},
  {"mq135",  1, {{"Air quality","ppm",K_GAUGE,0,1000}}},

  // ── Electricity / energy / battery ──
  {"ina219", 3, {{"Voltage","V",K_GAUGE,0,26},{"Current","A",K_METRIC,0,3.2f},{"Power","W",K_CHART,0,80}}},
  {"ina226", 3, {{"Voltage","V",K_GAUGE,0,36},{"Current","A",K_METRIC,-20,20},{"Power","W",K_CHART,0,700}}},
  {"acs712", 1, {{"Current","A",K_GAUGE,-30,30}}},
  {"ct-clamp",2,{{"Current","A",K_GAUGE,0,100},{"Power","W",K_CHART,0,23000}}},
  {"hlw8012",3, {{"Voltage","V",K_GAUGE,0,260},{"Current","A",K_METRIC,0,16},{"Power","W",K_CHART,0,3600}}},
  {"cse7766",3, {{"Voltage","V",K_GAUGE,0,260},{"Current","A",K_METRIC,0,16},{"Power","W",K_CHART,0,3600}}},
  {"pzem004t",4,{{"Voltage","V",K_GAUGE,0,260},{"Current","A",K_METRIC,0,100},{"Power","W",K_CHART,0,23000},{"Energy","kWh",K_STAT,0,9999}}},
  {"bms",    4, {{"State of charge","%",K_GAUGE,0,100},{"Pack voltage","V",K_METRIC,0,60},{"Current","A",K_CHART,-100,100},{"Temperature",DEG,K_METRIC,-20,80}}},

  // ── Light / UV ──
  {"ltr390", 2, {{"Illuminance","lx",K_METRIC,0,65535},{"UV index","",K_GAUGE,0,15}}},
  {"veml7700",1,{{"Illuminance","lx",K_METRIC,0,120000}}},
  {"veml6075",3,{{"UVA","",K_METRIC,0,2000},{"UVB","",K_METRIC,0,2000},{"UV index","",K_GAUGE,0,15}}},

  // ── Weight ──
  {"hx711",  1, {{"Weight","kg",K_GAUGE,0,200}}},
  {"nau7802",1, {{"Weight","g",K_GAUGE,0,5000}}},

  // ── Distance / presence ──
  {"hcsr04", 1, {{"Distance","cm",K_METRIC,0,400}}},
  {"vl53l0x",1, {{"Distance","mm",K_METRIC,0,2000}}},
  {"vl53l1x",1, {{"Distance","mm",K_METRIC,0,4000}}},
  {"ld2410", 3, {{"Presence","",K_METRIC,0,1},{"Distance","cm",K_GAUGE,0,600},{"Motion","%",K_CHART,0,100}}},
  {"ld2450", 2, {{"Targets","",K_METRIC,0,3},{"Distance","cm",K_GAUGE,0,600}}},

  // ── Pulse / flow / rotation ──
  {"pulse",  2, {{"Frequency","Hz",K_GAUGE,0,1000},{"Count","",K_STAT,0,1000000}}},
  {"flow",   2, {{"Flow rate","L/min",K_GAUGE,0,30},{"Total","L",K_STAT,0,100000}}},
  {"rpm",    1, {{"Speed","RPM",K_GAUGE,0,8000}}},

  // ── Motion / orientation / compass ──
  {"mpu6050",6, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2},{"Gyro X","\xC2\xB0/s",K_METRIC,-250,250},{"Gyro Y","\xC2\xB0/s",K_METRIC,-250,250},{"Gyro Z","\xC2\xB0/s",K_METRIC,-250,250}}},
  {"mpu9250",6, {{"Accel X","g",K_CHART,-2,2},{"Accel Y","g",K_CHART,-2,2},{"Accel Z","g",K_CHART,-2,2},{"Gyro X","\xC2\xB0/s",K_METRIC,-250,250},{"Gyro Y","\xC2\xB0/s",K_METRIC,-250,250},{"Gyro Z","\xC2\xB0/s",K_METRIC,-250,250}}},
  {"bno055", 3, {{"Pitch","\xC2\xB0",K_METRIC,-90,90},{"Roll","\xC2\xB0",K_METRIC,-180,180},{"Yaw","\xC2\xB0",K_GAUGE,0,360}}},
  {"qmc5883l",1,{{"Heading","\xC2\xB0",K_GAUGE,0,360}}},

  // ── Thermocouple / RTD (high temperature) ──
  {"max6675",1, {{"Temperature",DEG,K_GAUGE,0,1024}}},
  {"max31855",1,{{"Temperature",DEG,K_GAUGE,-200,1350}}},
  {"max31865",1,{{"Temperature",DEG,K_GAUGE,-200,600}}},

  // ── Weather station ──
  {"weather",4, {{"Wind speed","km/h",K_GAUGE,0,120},{"Gust","km/h",K_METRIC,0,150},{"Direction","\xC2\xB0",K_METRIC,0,360},{"Rain","mm",K_STAT,0,500}}},
  {"as3935", 2, {{"Lightning","km",K_GAUGE,0,40},{"Strikes","",K_STAT,0,1000}}},

  // ── Water quality / soil ──
  {"soil",   1, {{"Soil moisture","%",K_GAUGE,0,100}}},
  {"tds",    2, {{"TDS","ppm",K_GAUGE,0,2000},{"EC","\xC2\xB5S/cm",K_METRIC,0,4000}}},
  {"ph",     1, {{"pH","",K_GAUGE,0,14}}},
  {"turbidity",1,{{"Turbidity","NTU",K_GAUGE,0,3000}}},
  {"water-level",1,{{"Level","%",K_GAUGE,0,100}}},

  // ── Health / wearable ──
  {"max30102",2,{{"Heart rate","bpm",K_GAUGE,40,200},{"SpO2","%",K_METRIC,70,100}}},

  // ── Gas / safety ──
  {"mq2",    1, {{"Smoke / LPG","ppm",K_GAUGE,0,10000}}},
  {"mq7",    1, {{"CO","ppm",K_GAUGE,0,2000}}},
  {"mq9",    1, {{"Flammable gas","ppm",K_GAUGE,0,10000}}},

  // ── Navigation / audio ──
  {"neo-m10",3, {{"Speed","km/h",K_GAUGE,0,200},{"Altitude","m",K_METRIC,-100,9000},{"Satellites","",K_METRIC,0,24}}},
  {"neo-6m", 3, {{"Speed","km/h",K_GAUGE,0,200},{"Altitude","m",K_METRIC,-100,9000},{"Satellites","",K_METRIC,0,24}}},
  {"inmp441",1, {{"Sound level","dB",K_GAUGE,30,120}}},

  // ── System / diagnostics ──
  {"system", 3, {{"Uptime","h",K_STAT,0,100000},{"RSSI","dBm",K_METRIC,-100,0},{"Free heap","KB",K_METRIC,0,520}}},
};
const uint8_t RSENSOR_COUNT = sizeof(RSENSORS) / sizeof(RSENSORS[0]);
}  // namespace

SensorGroup RisalUI::sensor(const char* id, float* p0, float* p1, float* p2, float* p3,
                            float* p4, float* p5, float* p6, float* p7) {
  SensorGroup g(this);
  const RSensor* s = nullptr;
  for (uint8_t i = 0; i < RSENSOR_COUNT; i++)
    if (strcmp(id, RSENSORS[i].id) == 0) { s = &RSENSORS[i]; break; }
  if (!s) return g;  // unknown preset -> empty group (chaining is a no-op)
  float* p[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
  for (uint8_t i = 0; i < s->n && i < SensorGroup::CAP; i++) {
    if (!p[i]) continue;             // caller didn't bind this quantity -> skip it
    const RMeasure& m = s->m[i];
    Widget* w = nullptr;
    switch (m.kind) {
      case K_GAUGE: w = &gauge(m.title, p[i], m.lo, m.hi, m.unit); break;
      case K_CHART: w = &chart(m.title, p[i], m.unit); break;
      case K_STAT:  w = &stat(m.title, p[i], m.unit); break;
      default:      w = &metric(m.title, p[i], m.unit); break;
    }
    g._w[g._n] = w;
    g._p[g._n] = p[i];
    g._title[g._n] = m.title;
    g._unit[g._n] = m.unit;
    g._isChart[g._n] = (m.kind == K_CHART);
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
