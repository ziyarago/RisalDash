// RisalHub (step 1) — a fresh, layer-by-layer rebuild. This layer is Wi-Fi + web dashboard + the on-screen
// onboarding flow, with NO Zigbee yet (that comes in step 2).
//
// The screen tells you the state at a glance:
//   • Not configured (AP mode): the RisalHub logo, then a QR code — scan it and your phone joins the
//     "RisalHub-Setup" access point, where the captive portal opens to pick your Wi-Fi.
//   • Configured (STA mode): the logo again after the reboot, then the dashboard info screen with the
//     device IP and a QR that opens the web dashboard.
//
// Board: Waveshare ESP32-C6-LCD-1.47 (ST7789 172x320). Libs: RisalDash, GFX Library for Arduino,
// ricmoo/QRCode. Build: board_build.partitions = zigbee_single.csv (kept for step 2).
#include <RisalUI.h>
#include <WiFi.h>
#include <Arduino_GFX_Library.h>
#include "qrcode.h"

RisalUI dash("RisalHub");

// ── LCD (Waveshare C6-LCD-1.47) ──
enum { PIN_DC = 15, PIN_CS = 14, PIN_SCK = 7, PIN_MOSI = 6, PIN_RST = 21, PIN_BL = 22 };
enum { SCR_W = 172, SCR_H = 320 };
static const uint16_t C_BG   = RGB565(9, 13, 21),   C_TEAL = RGB565(45, 222, 190),
                      C_GREEN = RGB565(52, 211, 153), C_INK = RGB565(234, 240, 251),
                      C_INK3 = RGB565(96, 107, 130),  C_WHITE = 0xFFFF, C_BLACK = 0x0000;
static Arduino_DataBus* _bus = nullptr;
static Arduino_GFX*     gfx  = nullptr;

const char* AP_SSID = "RisalHub-Setup";

// ── Web state (step 1: just health, so the dashboard has something live) ──
float chipTemp = 0;
float uptimeMin = 0;

// ── Screen drawing ───────────────────────────────────────────────────────
void centerText(const char* s, int y, uint8_t size, uint16_t col) {
  gfx->setTextSize(size);
  gfx->setTextColor(col);
  int w = strlen(s) * 6 * size;          // 6px glyph cell at size 1
  gfx->setCursor((SCR_W - w) / 2, y);
  gfx->print(s);
}

void drawLogo() {
  gfx->fillScreen(C_BG);
  centerText("Risal", 116, 4, C_INK);    // stacked wordmark — "RisalHub" is too wide for 172px on one line
  centerText("Hub",   158, 4, C_TEAL);
  centerText("smart hub", 208, 1, C_INK3);
}

// Draw a QR of `text` centered, with a light panel so scanners read it on the dark UI.
void drawQR(const char* text, int top) {
  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(3)];
  qrcode_initText(&qr, buf, 3, ECC_LOW, text);   // version 3 = 29x29, holds our short strings
  int scale = 5, quiet = 8;
  int dim = qr.size * scale;
  int panel = dim + quiet * 2;
  int px = (SCR_W - panel) / 2, py = top;
  gfx->fillRoundRect(px, py, panel, panel, 6, C_WHITE);
  for (uint8_t y = 0; y < qr.size; y++)
    for (uint8_t x = 0; x < qr.size; x++)
      if (qrcode_getModule(&qr, x, y))
        gfx->fillRect(px + quiet + x * scale, py + quiet + y * scale, scale, scale, C_BLACK);
}

// AP mode: logo wordmark on top, "scan to set up" QR (joins the AP), SSID caption.
void drawApScreen() {
  gfx->fillScreen(C_BG);
  centerText("RisalHub", 16, 2, C_INK);
  centerText("SETUP MODE", 40, 1, C_TEAL);
  char wifi[64];
  snprintf(wifi, sizeof(wifi), "WIFI:S:%s;T:nopass;;", AP_SSID);
  drawQR(wifi, 66);
  centerText("Scan to connect", 250, 1, C_INK);
  centerText(AP_SSID, 268, 1, C_INK3);
  centerText("then pick your Wi-Fi", 286, 1, C_INK3);
}

// STA mode: online status, device IP, and a QR that opens the dashboard in a browser.
void drawStaScreen() {
  gfx->fillScreen(C_BG);
  centerText("RisalHub", 16, 2, C_INK);
  centerText("online", 40, 1, C_GREEN);
  String ip = WiFi.localIP().toString();
  char url[40];
  snprintf(url, sizeof(url), "http://%s/", ip.c_str());
  drawQR(url, 66);
  centerText("Scan to open", 250, 1, C_INK);
  centerText(ip.c_str(), 268, 1, C_TEAL);
}

// ── Boot / loop screen state machine (non-blocking, so web stays responsive) ──
enum { SCR_LOGO, SCR_MAIN } screen = SCR_LOGO;
bool     sta = false;
uint32_t logoUntil = 0;

void setup() {
  setCpuFrequencyMhz(80);   // heat-safe on the C6
  Serial.begin(115200);

  // Diagnostic: trace a client's join to the softAP — association vs. DHCP lease.
  WiFi.onEvent([](WiFiEvent_t e) {
    if (e == ARDUINO_EVENT_WIFI_AP_STACONNECTED)    Serial.println("[AP] client associated");
    if (e == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED)   Serial.println("[AP] client got IP (DHCP ok)");
    if (e == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) Serial.println("[AP] client left");
  });

  _bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
  gfx  = new Arduino_ST7789(_bus, PIN_RST, 0, true, SCR_W, SCR_H, 34, 0, 34, 0);
  gfx->begin();
  analogWrite(PIN_BL, 20 * 255 / 100);   // heat-safe backlight
  drawLogo();                            // logo shows during the (possibly slow) Wi-Fi join

  dash.brand("Risal<b>Hub</b>");
  dash.apName(AP_SSID);
  dash.layout("Hub", RICON_HOME);
  dash.separator("System");
  dash.metric("Chip temp", &chipTemp, "°C");
  dash.metric("Uptime", &uptimeMin, "min");
  dash.begin();

  sta = (WiFi.status() == WL_CONNECTED);
  Serial.printf("[WiFi] %s  %s\n", sta ? "STA connected IP=" : "portal/AP softAP=",
                (sta ? WiFi.localIP() : WiFi.softAPIP()).toString().c_str());
  if (sta) WiFi.setSleep(true);   // modem sleep ONLY in STA — it silences a softAP's beacon (AP vanishes)

  drawLogo();                            // fresh logo after the join decision
  logoUntil = millis() + 3000;           // hold ~3s, then reveal the QR/info screen
}

void loop() {
  dash.update();
  uint32_t now = millis();

  if (screen == SCR_LOGO && now > logoUntil) {
    screen = SCR_MAIN;
    sta ? drawStaScreen() : drawApScreen();
  }

  static uint32_t lastTick = 0;
  if (now - lastTick > 5000) {           // health telemetry
    lastTick = now;
    chipTemp = temperatureRead();
    uptimeMin = now / 60000;
    Serial.printf("[SYS] chip=%.1fC heap=%u wifi=%s\n", chipTemp, ESP.getFreeHeap(),
                  sta ? "STA" : "AP");
  }
}
