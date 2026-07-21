// RisalDash I2C Scanner + DHT11 — a pocket bench tool. Scans the I2C bus, lists every device it finds
// with its address and a best-guess of the chip, and reads a DHT11 temperature/humidity sensor — all on
// a live web dashboard. Great for "what's on this board?" and quick sensor checks with no serial cable.
//
// Board: Wemos D1 mini (ESP8266). Wiring:
//   DHT11  DATA -> D6 (GPIO12)   VCC -> 3V3   GND -> GND
//   I2C    SDA  -> D2 (GPIO4)    SCL -> D1 (GPIO5)   (Wemos defaults) — plus VCC/GND
//
// Libraries: RisalDash, Adafruit DHT sensor library (+ Adafruit Unified Sensor), Wire (built-in).
#include <RisalDash.h>
#include <Wire.h>
#include <DHT.h>

#define DHTPIN  12       // D6 on a Wemos D1 mini
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

RisalUI dash("I2C Scanner");

float temp = 0, hum = 0;
float i2cCount = 0;                 // float mirror so a metric can show it
bool  rescan = false;              // set by the "Re-scan" button
LogWidget* devLog = nullptr;

// Best-guess of a chip from its 7-bit I2C address (common hobby modules).
const char* i2cGuess(uint8_t a) {
  switch (a) {
    case 0x76: case 0x77: return "BME280 / BMP280 / BME680";
    case 0x3C: case 0x3D: return "SSD1306 / SH1106 OLED";
    case 0x68:            return "MPU6050 / DS3231 / DS1307";
    case 0x69:            return "MPU6050 (AD0=1)";
    case 0x23: case 0x5C: return "BH1750 light";
    case 0x38:            return "AHT10 / AHT20";
    case 0x40:            return "INA219 / HTU21D / Si7021";
    case 0x44: case 0x45: return "SHT3x";
    case 0x48: case 0x49: return "ADS1115 / LM75 / PCF8591";
    case 0x27: case 0x3F: return "PCF8574 LCD backpack";
    case 0x57:            return "MAX30102 / AT24C EEPROM";
    case 0x29:            return "VL53L0X / TCS34725";
    case 0x0D: case 0x1E: return "QMC5883L / HMC5883L compass";
    case 0x5A:            return "MLX90614 / CCS811";
    case 0x62:            return "SCD30 CO2";
    case 0x53:            return "ADXL345";
    default:              return "unknown device";
  }
}

void scanI2C() {
  if (!devLog) return;
  devLog->print("Scanning I2C bus...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      found++;
      char line[52];
      snprintf(line, sizeof(line), "0x%02X  -  %s", a, i2cGuess(a));
      devLog->print(line);
    }
  }
  i2cCount = found;
  devLog->print(found ? (String(found) + " device(s) found") : "No I2C devices found");
}

void setup() {
  dash.timezone(300);
  dht.begin();
  Wire.begin();                    // SDA = D2 (GPIO4), SCL = D1 (GPIO5) on a Wemos

  dash.separator("DHT11 · D6");
  dash.metric("Temperature", &temp, "°C");
  dash.metric("Humidity", &hum, "%");
  dash.chart("Temperature · trend", &temp, "°C");

  dash.separator("I2C bus");
  dash.metric("Devices found", &i2cCount);
  dash.button("Scan", "Re-scan I2C", []() { rescan = true; });
  devLog = &dash.log("I2C devices", 8);

  dash.enableOTA();
  dash.apName("I2C-Scanner");
  dash.begin();                    // joins saved Wi-Fi, else raises the setup portal

  scanI2C();                       // one scan at boot
}

void loop() {
  dash.update();

  static uint32_t lastDht = 0;
  if (millis() - lastDht > 2000) {  // DHT11 tops out at ~1 read/second
    lastDht = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temp = t;
    if (!isnan(h)) hum = h;
  }

  if (rescan) { rescan = false; scanI2C(); }
}
