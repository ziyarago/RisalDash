// RisalHub Zigbee (schema 3) — the C6 as a NATIVE Zigbee coordinator + WiFi dashboard + LCD, all on one
// chip. It pairs a Zigbee sensor (Sonoff SNZB-02: temperature + humidity) directly over its 802.15.4
// radio — no bridge, no Pi, no cloud — and shows the readings on both the web dashboard and the panel
// (a sliding carousel). This is the WiFi + Zigbee coexistence build.
//
// Build (see platformio.ini): -D ZIGBEE_MODE_ZCZR  +  board_build.partitions = zigbee_single.csv
// Libs: RisalDash, GFX Library for Arduino. Zigbee is a core library (Zigbee.h).
#include <RisalUI.h>
#include <WiFi.h>
#include <Arduino_GFX_Library.h>
#include "Zigbee.h"

RisalUI dash("Risal Zigbee");

// ── LCD (Waveshare C6-LCD-1.47, ST7789 172x320) ──
enum { PIN_DC = 15, PIN_CS = 14, PIN_SCK = 7, PIN_MOSI = 6, PIN_RST = 21, PIN_BL = 22 };
static const uint16_t C_BG = RGB565(9, 13, 21), C_TEAL = RGB565(45, 222, 190),
                      C_GREEN = RGB565(52, 211, 153), C_INK = RGB565(234, 240, 251),
                      C_INK3 = RGB565(96, 107, 130), C_OFF = RGB565(86, 95, 115);
static Arduino_DataBus* _bus = nullptr;
static Arduino_GFX* gfx = nullptr;

// ── Zigbee coordinator + the SNZB-02's readings ──
#define ZB_EP 5
ZigbeeThermostat zbT(ZB_EP);
float zbTemp = 0, zbHum = 0;
bool  zbBound = false;
void onTemp(float t) { zbTemp = t; }
void onHum(float h) { zbHum = h; }

// ── LCD carousel cards ──
struct Card { const char* label; float* val; const char* unit; uint16_t col; };
Card cards[] = {
  {"TEMPERATURE", &zbTemp, "C", C_GREEN},
  {"HUMIDITY",    &zbHum,  "%", C_TEAL},
};
const int NC = sizeof(cards) / sizeof(cards[0]);
int curCard = 0;

void drawScreen(int idx, int xoff) {
  if (!gfx) return;
  gfx->fillScreen(C_BG);
  gfx->setTextSize(2); gfx->setCursor(12, 12);
  gfx->setTextColor(C_INK);  gfx->print("Risal");
  gfx->setTextColor(C_TEAL); gfx->print("Hub");
  gfx->setTextColor(zbBound ? C_GREEN : C_INK3); gfx->setTextSize(1); gfx->setCursor(12, 34);
  gfx->print(zbBound ? "Zigbee - SNZB-02 linked" : "Zigbee - pairing...");
  Card& c = cards[idx];
  gfx->setTextColor(C_INK3); gfx->setTextSize(2); gfx->setCursor(12 + xoff, 92); gfx->print(c.label);
  gfx->setTextColor(c.col); gfx->setTextSize(7); gfx->setCursor(12 + xoff, 128);
  gfx->print((int)*c.val);
  gfx->setTextSize(3); gfx->setTextColor(C_INK3); gfx->print(c.unit);
  for (int i = 0; i < NC; i++) gfx->fillCircle(66 + i * 22, 296, 5, i == idx ? C_TEAL : C_OFF);
}

void slideTo(int next) {                 // new card slides in from the right
  for (int x = 172; x > 0; x -= 20) drawScreen(next, x);
  drawScreen(next, 0);
}

void setup() {
  setCpuFrequencyMhz(80);   // heat-safe: the C6 runs hot at 160MHz, and two radios add to it
  Serial.begin(115200);
  dash.timezone(300);       // language defaults to English
  dash.brand("Risal<b>Hub</b>");

  _bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
  gfx = new Arduino_ST7789(_bus, PIN_RST, 0, true, 172, 320, 34, 0, 34, 0);
  gfx->begin();
  analogWrite(PIN_BL, 43 * 255 / 100);   // heat-safe backlight
  drawScreen(0, 0);

  // Web page — the sensor's readings.
  dash.layout("Sensor", RICON_THERMOMETER);
  dash.led("Zigbee", &zbBound);
  dash.separator("SNZB-02 · Zigbee");
  dash.metric("Temperature", &zbTemp, "°C");
  dash.metric("Humidity", &zbHum, "%");
  dash.begin();   // WiFi first (saved creds), then Zigbee below
  Serial.print("[WiFi] status=");
  Serial.print(WiFi.status() == WL_CONNECTED ? "connected  IP=" : "portal/AP  softAP=");
  Serial.println(WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP());
  WiFi.setSleep(true);   // modem sleep — cuts heat and quiets the shared 2.4GHz RF for 802.15.4

  // Zigbee coordinator after WiFi is up (WiFi + 802.15.4 coexistence).
  zbT.onTempReceive(onTemp);
  zbT.onHumidityReceive(onHum);
  zbT.setManufacturerAndModel("Risal", "Hub");
  Zigbee.addEndpoint(&zbT);
  Zigbee.setRebootOpenNetwork(180);
  Serial.println("[ZB] starting coordinator...");
  if (!Zigbee.begin(ZIGBEE_COORDINATOR)) {
    Serial.println("[ZB] Zigbee.begin FAILED — restarting");
    delay(1000);
    ESP.restart();
  }
  Serial.println("[ZB] coordinator up; opening network for 180s");
  Zigbee.openNetwork(180);
}

uint32_t lastSlide = 0, lastDraw = 0;
void loop() {
  dash.update();
  if (zbT.bound() && !zbBound) {         // SNZB-02 joined + bound
    zbBound = true;
    zbT.setTemperatureReporting(0, 30, 1);
  }
  uint32_t now = millis();
  if (now - lastSlide > 3500) { lastSlide = now; curCard = (curCard + 1) % NC; slideTo(curCard); lastDraw = now; }
  else if (now - lastDraw > 1000) { lastDraw = now; drawScreen(curCard, 0); }   // refresh live value
}
