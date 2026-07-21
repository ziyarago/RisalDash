// RisalDash Sensor Compare — four temperature/humidity sensors on one I2C bus, side by side, each with
// its live value AND a trend chart, so you can watch them agree (or drift) in real time. Every sensor
// is auto-detected: plug it in and it comes online; leave it out and its card reads "offline".
//
// Sensors: BME280 (0x76, temp+humidity+pressure), BMP280 (0x77, temp+pressure), AHT20 (0x38,
// temp+humidity) and HDC1080 (0x40, temp+humidity). Compare the numbers and the curves — in a real run
// the HDC1080 read several % higher humidity than the BME280 next to it.
//
// Universal: same sketch on ESP8266 and ESP32. I2C uses each board's default pins:
//   ESP8266 (Wemos): SDA = D2 (GPIO4), SCL = D1 (GPIO5)   ·   ESP32: SDA = GPIO21, SCL = GPIO22
// All parts share SDA/SCL + 3V3/GND.
//
// Libraries: RisalDash, Adafruit BME280, Adafruit BMP280, Adafruit AHTX0, ClosedCube HDC1080,
// Adafruit Unified Sensor, Adafruit BusIO, Wire (built-in).
#include <RisalDash.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <ClosedCube_HDC1080.h>

RisalUI dash("Sensor Compare");

Adafruit_BME280    bme;     // 0x76
Adafruit_BMP280    bmp;     // 0x77
Adafruit_AHTX0     aht;     // 0x38
ClosedCube_HDC1080 hdc;     // 0x40
bool bmeOK = false, bmpOK = false, ahtOK = false, hdcOK = false;

float bmeT = 0, bmeH = 0, bmeP = 0;   // BME280
float bmpT = 0, bmpP = 0;             // BMP280 (no humidity)
float ahtT = 0, ahtH = 0;             // AHT20
float hdcT = 0, hdcH = 0;             // HDC1080

bool i2cPresent(uint8_t a) { Wire.beginTransmission(a); return Wire.endTransmission() == 0; }

// Release a slave that's holding SDA low (a transaction cut short by a reset locks the whole bus, so
// every begin() then fails). Pulse SCL up to 9 times until SDA goes high, before Wire.begin().
void i2cRecover(uint8_t sda, uint8_t scl) {
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, INPUT_PULLUP);
  delay(2);
  for (int i = 0; i < 9 && digitalRead(sda) == LOW; i++) {
    pinMode(scl, OUTPUT); digitalWrite(scl, LOW); delayMicroseconds(6);
    pinMode(scl, INPUT_PULLUP);                    delayMicroseconds(6);
  }
}

void setup() {
  dash.timezone(300);
#if defined(ESP8266)
  i2cRecover(4, 5);     // SDA=D2/GPIO4, SCL=D1/GPIO5
#else
  i2cRecover(21, 22);   // ESP32 default I2C
#endif
  Wire.begin();

  bmeOK = bme.begin(0x76);
  bmpOK = bmp.begin(0x77);
  ahtOK = aht.begin();                // 0x38
  hdcOK = i2cPresent(0x40);
  if (hdcOK) hdc.begin(0x40);

  dash.separator("BME280 · 0x76");
  dash.led("BME280 online", &bmeOK);
  dash.metric("BME280 temp", &bmeT, "°C").decimals(1);
  dash.chart("BME280 temp trend", &bmeT, "°C");
  dash.metric("BME280 humidity", &bmeH, "%").decimals(1);
  dash.metric("BME280 pressure", &bmeP, "hPa").decimals(0);

  dash.separator("BMP280 · 0x77");
  dash.led("BMP280 online", &bmpOK);
  dash.metric("BMP280 temp", &bmpT, "°C").decimals(1);
  dash.chart("BMP280 temp trend", &bmpT, "°C");
  dash.metric("BMP280 pressure", &bmpP, "hPa").decimals(0);

  dash.separator("AHT20 · 0x38");
  dash.led("AHT20 online", &ahtOK);
  dash.metric("AHT20 temp", &ahtT, "°C").decimals(1);
  dash.chart("AHT20 temp trend", &ahtT, "°C");
  dash.metric("AHT20 humidity", &ahtH, "%").decimals(1);

  dash.separator("HDC1080 · 0x40");
  dash.led("HDC1080 online", &hdcOK);
  dash.metric("HDC1080 temp", &hdcT, "°C").decimals(1);
  dash.chart("HDC1080 temp trend", &hdcT, "°C");
  dash.metric("HDC1080 humidity", &hdcH, "%").decimals(1);

  dash.enableOTA();
  dash.apName("SensorCompare");
  dash.begin();
}

void loop() {
  dash.update();

  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();

    if (bmeOK) { float t = bme.readTemperature(), h = bme.readHumidity(), p = bme.readPressure() / 100.0f;
                 if (!isnan(t)) bmeT = t; if (!isnan(h)) bmeH = h; if (!isnan(p)) bmeP = p; }
    if (bmpOK) { float t = bmp.readTemperature(), p = bmp.readPressure() / 100.0f;
                 if (!isnan(t)) bmpT = t; if (!isnan(p)) bmpP = p; }
    if (ahtOK) { sensors_event_t he, te; aht.getEvent(&he, &te);
                 if (!isnan(te.temperature)) ahtT = te.temperature; if (!isnan(he.relative_humidity)) ahtH = he.relative_humidity; }
    if (hdcOK) { float t = hdc.readTemperature(), h = hdc.readHumidity();
                 if (!isnan(t) && t > -40 && t < 125) hdcT = t; if (!isnan(h) && h >= 0 && h <= 100) hdcH = h; }
  }
}
