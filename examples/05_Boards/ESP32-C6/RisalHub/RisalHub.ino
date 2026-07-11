// RisalHub — an all-in-one smart-home hub on a single ESP32-C6. It runs its OWN MQTT broker AND a
// beautiful RisalDash dashboard, discovers devices across transports, and CONTROLS them with commands —
// no PC, Pi or cloud.
//
// The heart of the product is the DRIVER TABLE below: a device = { transport, how-to-reach it, its
// commands }. The hub renders a control card per device and dispatches commands by transport. Adding
// support for a new gadget is one row in `devices[]`, not new plumbing.
//   • BLE driver  — connect + optional login, then write command bytes (e.g. the RASMA aroma diffuser).
//   • MQTT driver — publish a command topic to the built-in broker (e.g. a Tasmota/Sonoff plug).
//
// Build:  -D RISAL_MAX_WIDGETS=80
// Libs:   RisalDash, mlesniew/PicoMQTT, bblanchon/ArduinoJson   (BLE is core: <BLEDevice.h>)
#include <RisalUI.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <Preferences.h>           // persist hub settings (PIN, transports) to NVS
#include <Arduino_GFX_Library.h>   // Waveshare C6-LCD-1.47 (ST7789 172x320)

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;

// ── On-board LCD: a live device list on the panel (so the hub looks finished, not dark) ──
// ⚠ HEAT: never drive the backlight above ~50% on these Waveshare boards.
enum { PIN_DC = 15, PIN_CS = 14, PIN_SCK = 7, PIN_MOSI = 6, PIN_RST = 21, PIN_BL = 22 };
static const uint16_t C_BG = RGB565(20, 26, 40), C_TEAL = RGB565(45, 222, 190),
                      C_GREEN = RGB565(120, 230, 150), C_INK = RGB565(238, 242, 250),
                      C_INK3 = RGB565(120, 132, 150), C_LINE = RGB565(50, 58, 76);
static Arduino_DataBus* _bus = nullptr;
static Arduino_GFX* gfx = nullptr;

// ── Driver model ──────────────────────────────────────────────────────────────────────────────────
enum Transport { T_MQTT, T_BLE };

struct Device {
  const char* name;         // shown on the control card
  Transport   transport;
  bool        power;        // bound to the card's Power toggle
  bool        linked;       // BLE: connected+logged in / MQTT: always true
  // MQTT
  const char* cmdTopic;     // e.g. "cmnd/plug/POWER"
  const char* onMsg;        // e.g. "ON"
  const char* offMsg;       // e.g. "OFF"
  // BLE
  const char* bleMatch;     // advertised-name prefix, e.g. "YGZK"
  const char* pin;          // login PIN (0x8F + PIN), or nullptr for none
  uint8_t     onCmd[4];  uint8_t onLen;   // power-on bytes
  uint8_t     offCmd[4]; uint8_t offLen;  // power-off bytes
  BLEAddress* addr;         // filled by the scan
  BLERemoteCharacteristic* ch;
  BLEClient* client;        // kept to check the link + re-assert state
};

// ── THE DRIVER TABLE — add a device = add a row ──
Device devices[] = {
  { "Air Freshener", T_BLE,  false, false,
    nullptr, nullptr, nullptr,                 // (no MQTT)
    "YGZK", "9999",                            // BLE name prefix + PIN
    {0x2D, 0x09}, 2, {0x2D, 0x08}, 2,          // on / off command bytes
    nullptr, nullptr, nullptr },
  { "Sonoff Plug",   T_MQTT, false, true,
    "cmnd/plug/POWER", "ON", "OFF",            // MQTT command topic + payloads
    nullptr, nullptr, {0}, 0, {0}, 0,          // (no BLE)
    nullptr, nullptr, nullptr },
};
const int NDEV = sizeof(devices) / sizeof(devices[0]);

// Persisted settings (NVS) — editable live from the Settings page.
Preferences prefs;
String freshenerPin = "9999";      // BLE login PIN (0x8F + PIN)
bool   bleOn = true;               // enable BLE device linking
String brokerStatus = "running :1883";

// Per-device status-widget names (stable storage so each card has a unique, readable key).
String ledName[NDEV];

// Discovery feed (MQTT visibility).
LogWidget* feed = nullptr;
int msgCount = 0;

// ── Command dispatch: one entry point, routed by transport ──
void sendPower(Device& d, bool on) {
  if (d.transport == T_MQTT) {
    mqtt.publish(d.cmdTopic, on ? d.onMsg : d.offMsg);
  } else if (d.ch) {
    uint8_t* c = on ? d.onCmd : d.offCmd;
    d.ch->writeValue(c, on ? d.onLen : d.offLen, false);  // write-no-response, raw bytes
  }
}

// ── BLE: find each BLE driver by name during one scan pass ──
class ScanCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice a) override {
    if (!a.haveName()) return;
    String n = a.getName().c_str();
    for (int i = 0; i < NDEV; i++)
      if (devices[i].transport == T_BLE && !devices[i].addr && n.startsWith(devices[i].bleMatch))
        devices[i].addr = new BLEAddress(a.getAddress());
  }
};

static BLERemoteCharacteristic* pick(BLERemoteService* s, const char* uuid) {
  return s ? s->getCharacteristic(BLEUUID(uuid)) : nullptr;
}

// Connect + find the write characteristic + login, for a device whose address we already know.
// Used both at boot and by the reconnect loop — no scan, so it's quick enough to call from loop().
bool linkOne(Device& d) {
  if (!d.addr) return false;
  if (!d.client) d.client = BLEDevice::createClient();
  if (!d.client->isConnected() && !d.client->connect(*d.addr)) return false;
  d.ch = pick(d.client->getService(BLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb")), "0000ffe2-0000-1000-8000-00805f9b34fb");
  if (!d.ch) {
    BLERemoteService* ae = d.client->getService(BLEUUID("0000ae30-0000-1000-8000-00805f9b34fb"));
    d.ch = pick(ae, "0000ae01-0000-1000-8000-00805f9b34fb");
    if (!d.ch) d.ch = pick(ae, "0000ae03-0000-1000-8000-00805f9b34fb");
  }
  if (!d.ch) return false;
  if (d.pin) {                                     // login: 0x8F + configurable PIN (from NVS)
    uint8_t login[1 + 8]; login[0] = 0x8F;
    size_t pl = freshenerPin.length(); if (pl > 8) pl = 8;
    memcpy(login + 1, freshenerPin.c_str(), pl);
    d.ch->writeValue(login, 1 + pl, false);
  }
  d.linked = true;
  sendPower(d, d.power);   // restore the commanded state on (re)connect
  return true;
}

void linkBleDevices() {
  if (!bleOn) return;   // Bluetooth disabled in Settings
  bool anyBle = false;
  for (int i = 0; i < NDEV; i++) if (devices[i].transport == T_BLE) anyBle = true;
  if (!anyBle) return;

  BLEDevice::init("RisalHub");
  BLEScan* scan = BLEDevice::getScan();
  static ScanCB cb;
  scan->setAdvertisedDeviceCallbacks(&cb);
  scan->setActiveScan(true);
  for (int t = 0; t < 8; t++) scan->start(3, false);
  scan->stop();

  for (int i = 0; i < NDEV; i++)
    if (devices[i].transport == T_BLE) linkOne(devices[i]);
}

void lcdBegin() {
  _bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
  gfx = new Arduino_ST7789(_bus, PIN_RST, 0, true, 172, 320, 34, 0, 34, 0);
  gfx->begin();
  gfx->fillScreen(C_BG);
  analogWrite(PIN_BL, 43 * 255 / 100);   // 43% backlight (heat-safe)
}

// Redraw the device list. Called from loop() only when something changed.
void lcdRender() {
  if (!gfx) return;
  gfx->fillScreen(C_BG);
  gfx->setTextColor(C_TEAL); gfx->setTextSize(2); gfx->setCursor(12, 12); gfx->print("RISAL HUB");
  gfx->setTextColor(C_INK3); gfx->setTextSize(1); gfx->setCursor(12, 34);
  gfx->print(WiFi.localIP().toString().c_str());
  gfx->drawFastHLine(12, 52, 148, C_LINE);

  for (int i = 0; i < NDEV; i++) {
    int y = 66 + i * 62;
    gfx->setTextColor(C_INK); gfx->setTextSize(2); gfx->setCursor(12, y);
    gfx->print(devices[i].name);
    bool on = devices[i].power;
    gfx->setTextColor(on ? C_GREEN : C_INK3); gfx->setTextSize(2); gfx->setCursor(12, y + 22);
    gfx->print(on ? "\xF7 ON" : "\xF7 OFF");   // 0xF7 ~ round dot in the classic font
    gfx->setTextColor(devices[i].linked ? C_TEAL : C_INK3); gfx->setTextSize(1); gfx->setCursor(96, y + 26);
    gfx->print(devices[i].transport == T_BLE ? (devices[i].linked ? "BLE linked" : "BLE  ...") : "MQTT");
  }
  gfx->setTextColor(C_INK3); gfx->setTextSize(1); gfx->setCursor(12, 300);
  gfx->print(NDEV); gfx->print(" devices");
}

// A tiny signature of what's on screen, so we only redraw on change (no flicker/CPU waste).
uint32_t lcdSig() {
  uint32_t s = 0;
  for (int i = 0; i < NDEV; i++) s = s * 7 + (devices[i].power ? 1 : 0) * 2 + (devices[i].linked ? 1 : 0);
  return s;
}

void setup() {
  dash.timezone(300);
  lcdBegin();   // light the panel immediately

  prefs.begin("hub", false);                         // restore saved settings
  freshenerPin = prefs.getString("pin", "9999");
  bleOn = prefs.getBool("ble", true);
  for (int i = 0; i < NDEV; i++) devices[i].power = prefs.getBool((String("p") + i).c_str(), false);

  // Settings page — transport status + editable, persisted device config.
  dash.layout("Settings", RICON_SIGNAL);
  dash.label("MQTT broker", &brokerStatus);
  dash.toggle("Bluetooth", &bleOn, [](bool on) { prefs.putBool("ble", on); });   // applies on next boot
  dash.password("Freshener PIN", &freshenerPin, [](const String& v) { prefs.putString("pin", v); });

  // Devices page — a control card per driver.
  dash.layout("Devices", RICON_HOME);
  for (int i = 0; i < NDEV; i++) {
    dash.toggle(devices[i].name, &devices[i].power, [i](bool on) {
      sendPower(devices[i], on);
      prefs.putBool((String("p") + i).c_str(), on);   // remember across reboots
    });
    ledName[i] = String(devices[i].name) + (devices[i].transport == T_BLE ? " · link" : " · online");
    dash.led(ledName[i].c_str(), &devices[i].linked);
  }

  // Discovery page — MQTT visibility.
  dash.layout("Discovery", RICON_SIGNAL);
  dash.badge("MQTT msgs", &msgCount);
  feed = &dash.log("MQTT feed", 6);

  dash.begin();

  mqtt.subscribe("#", [](const char* topic, const char* payload) {
    msgCount++;
    String line = String(topic) + " = " + payload;
    if (line.length() > 60) line = line.substring(0, 59) + "…";
    if (feed) feed->print(line);
    // reflect a Tasmota plug's real state back onto its card
    for (int i = 0; i < NDEV; i++)
      if (devices[i].transport == T_MQTT && String(topic) == "stat/plug/POWER")
        devices[i].power = (String(payload) == "ON");
  });
  mqtt.begin();

  linkBleDevices();   // connect + login BLE drivers once WiFi is up
}

uint32_t lastSig = 0xFFFFFFFF, lastLcd = 0, lastEnforce = 0;
void loop() {
  dash.update();
  mqtt.loop();

  // Keepalive: re-assert each linked BLE device's commanded state so its own internal timer can't
  // override what the user set on the hub. Also detect a dropped link.
  if (millis() - lastEnforce > 20000) {
    lastEnforce = millis();
    for (int i = 0; i < NDEV; i++) {
      Device& d = devices[i];
      if (d.transport != T_BLE) continue;
      if (d.client && d.client->isConnected()) sendPower(d, d.power);   // re-assert (beats the device timer)
      else if (bleOn && d.addr) { d.linked = false; linkOne(d); }       // dropped -> reconnect by address
      else d.linked = false;
    }
  }

  if (millis() - lastLcd > 400) {   // redraw the panel only when a device state changed
    lastLcd = millis();
    uint32_t s = lcdSig();
    if (s != lastSig) { lastSig = s; lcdRender(); }
  }
}
