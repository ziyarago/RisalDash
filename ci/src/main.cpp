// Hardware verification probe for the Hosyond 2.8" ESP32-S3 touch module (lcdwiki pinout).
// Exercises every peripheral and reports to serial: ILI9341 LCD (colour bars + text), FT6336
// capacitive touch (draw dots where you tap), WS2812 RGB blink, SDIO card mount, battery ADC.
// Temporary CI payload — not an example.
#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <SD_MMC.h>

// lcdwiki: LCD CS=10 DC=46 SCK=12 MOSI=11 MISO=13 BL=45 · touch SDA=16 SCL=15 INT=17 RST=18
// RGB=42 · SD CLK=38 CMD=40 D0-3=39/41/48/47 · battery ADC=9
enum { P_CS = 10, P_DC = 46, P_SCK = 12, P_MOSI = 11, P_MISO = 13, P_BL = 45 };
enum { T_SDA = 16, T_SCL = 15, T_RST = 18, T_INT = 17 };

Arduino_DataBus *bus = new Arduino_ESP32SPI(P_DC, P_CS, P_SCK, P_MOSI, P_MISO);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, GFX_NOT_DEFINED, 0 /* rotation */, false);

bool touchOk = false;

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("=== Hosyond 2.8 S3 probe ===");

  // ── LCD ──
  bool lcd = gfx->begin();
  pinMode(P_BL, OUTPUT);
  digitalWrite(P_BL, HIGH);  // backlight on (if the panel stays dark, polarity is inverted)
  Serial.printf("[lcd] begin=%d\n", lcd);
  const uint16_t C[] = {RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW, RGB565_CYAN, RGB565_MAGENTA};
  for (int i = 0; i < 6; i++) gfx->fillRect(0, i * 53, 240, 53, C[i]);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 10);
  gfx->print("RisalDash probe");
  gfx->setCursor(10, 40);
  gfx->print("tap the screen!");

  // ── Touch: FT6336 @0x38 on SDA=16 SCL=15 (hardware reset first) ──
  pinMode(T_RST, OUTPUT);
  digitalWrite(T_RST, LOW);
  delay(10);
  digitalWrite(T_RST, HIGH);
  delay(300);
  Wire.begin(T_SDA, T_SCL, 400000);
  for (uint8_t a = 0x08; a <= 0x77; a++) {  // full bus scan so we see everything on it
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("[i2c] found 0x%02X\n", a);
  }
  Wire.beginTransmission(0x38);
  Wire.write(0xA6);  // FT63xx firmware version reg
  if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x38, 1) == 1) {
    Serial.printf("[touch] FT6336 fw=0x%02X\n", Wire.read());
    touchOk = true;
  } else {
    Serial.println("[touch] FT6336 NOT responding at 0x38");
  }

  // ── Fingerprint the new device at 0x77: Bosch chip-id reg 0xD0 / DPS310 reg 0x0D ──
  for (uint8_t reg : {0xD0, 0x0D}) {
    Wire.beginTransmission(0x77);
    Wire.write(reg);
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x77, 1) == 1)
      Serial.printf("[0x77] reg 0x%02X = 0x%02X\n", reg, Wire.read());
  }
  // Bosch id map: BMP180=0x55 · BMP280=0x58 · BME280=0x60 · BME680=0x61 · BMP388=0x50 · DPS310(0x0D)=0x10

  // ── RGB (WS2812 on 42): blink red -> green -> blue ──
  neopixelWrite(42, 40, 0, 0); delay(250);
  neopixelWrite(42, 0, 40, 0); delay(250);
  neopixelWrite(42, 0, 0, 40); delay(250);
  neopixelWrite(42, 0, 0, 0);
  Serial.println("[rgb] blinked on GPIO42");

  // ── SD over SDIO 4-bit ──
  SD_MMC.setPins(38, 40, 39, 41, 48, 47);
  if (SD_MMC.begin("/sdcard", false)) {
    Serial.printf("[sd] mounted, size=%llu MB, type=%d\n", SD_MMC.cardSize() / 1048576ULL, SD_MMC.cardType());
  } else {
    Serial.println("[sd] mount FAILED (no card inserted?)");
  }

  // ── Battery ADC ──
  Serial.printf("[bat] GPIO9 = %lu mV (raw, before divider math)\n", (unsigned long)analogReadMilliVolts(9));
  Serial.println("=== probe ready — tap the screen ===");
}

uint32_t lastHb = 0;

void loop() {
  if (touchOk) {
    Wire.beginTransmission(0x38);
    Wire.write(0x02);  // TD_STATUS: number of touch points, then P1 XY
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x38, 5) == 5) {
      uint8_t n = Wire.read() & 0x0F;
      uint8_t xh = Wire.read(), xl = Wire.read(), yh = Wire.read(), yl = Wire.read();
      if (n > 0 && n < 3) {
        int x = ((xh & 0x0F) << 8) | xl;
        int y = ((yh & 0x0F) << 8) | yl;
        Serial.printf("[touch] n=%d x=%d y=%d\n", n, x, y);
        gfx->fillCircle(x, y, 6, RGB565_WHITE);
      }
    }
  }
  if (millis() - lastHb > 3000) {
    lastHb = millis();
    Serial.printf("[hb] heap=%u psram=%u\n", ESP.getFreeHeap(), ESP.getFreePsram());
  }
  delay(30);
}
