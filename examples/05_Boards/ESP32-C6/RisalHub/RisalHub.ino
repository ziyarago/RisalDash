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
static const uint16_t C_BG = RGB565(9, 13, 21), C_TEAL = RGB565(45, 222, 190),
                      C_GREEN = RGB565(52, 211, 153), C_INK = RGB565(234, 240, 251),
                      C_INK3 = RGB565(96, 107, 130), C_LINE = RGB565(40, 48, 66),
                      C_AMBER = RGB565(245, 185, 74), C_OFF = RGB565(86, 95, 115);
static Arduino_DataBus* _bus = nullptr;
static Arduino_GFX* gfx = nullptr;

// ── Driver model ──────────────────────────────────────────────────────────────────────────────────
enum Transport { T_MQTT, T_BLE };

struct Device {
  const char* name;         // shown on the control card (web)
  const char* emoji;        // card icon
  const char* lcd;          // Latin name for the Latin-only LCD font
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
  int rssi;                 // last advertised RSSI (BLE), for the map's link quality
};

// ── THE DRIVER TABLE — add a device = add a row ──
Device devices[] = {
  { "Освежитель", "\xF0\x9F\xAB\xA7", "Freshener", T_BLE,  false, false,   // 🫧
    nullptr, nullptr, nullptr,                 // (no MQTT)
    "YGZK", "9999",                            // BLE name prefix + PIN
    {0x2D, 0x09}, 2, {0x2D, 0x08}, 2,          // on / off command bytes
    nullptr, nullptr, nullptr, -127 },
  { "Розетка",   "\xF0\x9F\x94\x8C", "Plug", T_MQTT, false, true,    // 🔌
    "cmnd/plug/POWER", "ON", "OFF",            // MQTT command topic + payloads
    nullptr, nullptr, {0}, 0, {0}, 0,          // (no BLE)
    nullptr, nullptr, nullptr, -127 },
};
const int NDEV = sizeof(devices) / sizeof(devices[0]);

// Persisted settings (NVS) — editable live from the Settings page.
Preferences prefs;
String freshenerPin = "9999";      // BLE login PIN (0x8F + PIN)
bool   bleOn = true;               // enable BLE device linking
int    backlight = 43;             // LCD brightness % (capped ≤50 — Waveshare panels overheat higher)
String brokerStatus = "running :1883";

void applyBacklight() { analogWrite(PIN_BL, constrain(backlight, 5, 50) * 255 / 100); }

// Home summary (the Overview hero).
String homeHeadline = "Starting…", homeDetail = "";
int    homeMood = 0;               // 0 good · 1 warn · 2 alarm
String netData;                    // network-map records: "name~emoji~online~link~addr;"
String tableData;                  // device-table records: "emoji~name~transport~addr~link~lastseen;"

// Discovery feed (MQTT visibility).
LogWidget* feed = nullptr;
int msgCount = 0;

// ── BLE sensor (passive beacon): a Xiaomi LYWSD03MMC on ATC/pvvx firmware broadcasting BTHome v2
// (service 0xFCD2) — temperature / humidity / battery in plaintext. Read from the scan, no connection.
float senseT = 0, senseH = 0, senseBat = 0;
bool  senseSeen = false;

void decodeBTHome(const uint8_t* p, int len) {
  if (len < 1 || (p[0] & 0x01)) return;   // bit0 = encrypted -> skip
  int i = 1;
  while (i + 1 < len) {
    uint8_t obj = p[i++];
    if (obj == 0x00) { i += 1; }                                                   // packet id
    else if (obj == 0x01) { senseBat = p[i]; i += 1; }                             // battery %
    else if (obj == 0x02) { senseT = (int16_t)(p[i] | (p[i + 1] << 8)) * 0.01f; i += 2; }   // temp °C
    else if (obj == 0x03) { senseH = (uint16_t)(p[i] | (p[i + 1] << 8)) * 0.01f; i += 2; }  // humidity %
    else if (obj == 0x0C) { i += 2; }                                              // voltage (skip)
    else return;   // unknown object id -> stop (variable lengths)
  }
  senseSeen = true;
}

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
    // Passive sensor beacon: BTHome v2 on service 0xFCD2 (Xiaomi LYWSD03MMC on ATC/pvvx firmware).
    if (a.haveServiceData())
      for (int i = 0; i < a.getServiceDataCount(); i++)
        if (a.getServiceDataUUID(i).toString() == "0000fcd2-0000-1000-8000-00805f9b34fb") {
          String d = a.getServiceData(i);
          decodeBTHome((const uint8_t*)d.c_str(), d.length());
        }
    // Named BLE driver devices (e.g. the freshener): grab address + RSSI for connect + the map.
    if (!a.haveName()) return;
    String n = a.getName().c_str();
    for (int i = 0; i < NDEV; i++)
      if (devices[i].transport == T_BLE && n.startsWith(devices[i].bleMatch)) {
        if (!devices[i].addr) devices[i].addr = new BLEAddress(a.getAddress());
        devices[i].rssi = a.getRSSI();
      }
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
  applyBacklight();   // heat-safe brightness (capped ≤50%)
}

// Boot splash — the RisalHub wordmark, shown for a moment at power-on.
void lcdSplash() {
  if (!gfx) return;
  gfx->fillScreen(C_BG);
  gfx->setTextSize(4);
  gfx->setTextColor(C_INK);  gfx->setCursor(26, 116); gfx->print("Risal");
  gfx->setTextColor(C_TEAL); gfx->setCursor(50, 158); gfx->print("Hub");
  gfx->setTextSize(1); gfx->setTextColor(C_INK3); gfx->setCursor(44, 212); gfx->print("smart home hub");
}

// Redraw the panel: wordmark + a big "how many on" summary + a device list with colour-coded dots.
void lcdRender() {
  if (!gfx) return;
  gfx->fillScreen(C_BG);
  gfx->setTextSize(2); gfx->setCursor(12, 12);
  gfx->setTextColor(C_INK);  gfx->print("Risal");
  gfx->setTextColor(C_TEAL); gfx->print("Hub");
  gfx->setTextColor(C_INK3); gfx->setTextSize(1); gfx->setCursor(12, 34);
  gfx->print(WiFi.localIP().toString().c_str());

  int on = 0, issue = 0;
  for (int i = 0; i < NDEV; i++) {
    if (devices[i].power) on++;
    if (devices[i].transport == T_BLE && !devices[i].linked) issue++;
  }
  gfx->setTextColor(C_GREEN); gfx->setTextSize(5); gfx->setCursor(12, 52); gfx->print(on);
  gfx->setTextSize(1); gfx->setTextColor(C_INK3);
  gfx->setCursor(56, 58); gfx->print("ON");
  gfx->setCursor(56, 72); gfx->print("of "); gfx->print(NDEV);
  gfx->setCursor(12, 106);
  if (issue) { gfx->setTextColor(C_AMBER); gfx->print(issue); gfx->print(" offline"); }
  else { gfx->setTextColor(C_INK3); gfx->print("all linked"); }
  gfx->drawFastHLine(12, 124, 148, C_LINE);

  for (int i = 0; i < NDEV; i++) {
    int y = 140 + i * 26;
    uint16_t dc = devices[i].power ? C_GREEN
                : (devices[i].transport == T_BLE && !devices[i].linked) ? C_AMBER : C_OFF;
    gfx->fillCircle(17, y + 4, 4, dc);
    gfx->setTextColor(C_INK); gfx->setTextSize(1); gfx->setCursor(30, y); gfx->print(devices[i].lcd);
  }
}

// A tiny signature of what's on screen, so we only redraw on change (no flicker/CPU waste).
uint32_t lcdSig() {
  uint32_t s = 0;
  for (int i = 0; i < NDEV; i++) s = s * 7 + (devices[i].power ? 1 : 0) * 2 + (devices[i].linked ? 1 : 0);
  return s;
}

void setup() {
  dash.timezone(300);
  dash.lang("ru");       // Russian UI (On/Off, status bar)
  dash.bottomNav();      // app-like bottom tab bar instead of the top strip
  dash.brand("Risal<b>Hub</b>");   // own wordmark instead of "RisalDash · …"
  lcdBegin();     // light the panel immediately
  lcdSplash();    // RisalHub logo at power-on
  delay(1500);    // hold the splash a moment

  prefs.begin("hub", false);                         // restore saved settings
  freshenerPin = prefs.getString("pin", "9999");
  bleOn = prefs.getBool("ble", true);
  backlight = prefs.getInt("bl", 43);
  applyBacklight();
  for (int i = 0; i < NDEV; i++) devices[i].power = prefs.getBool((String("p") + i).c_str(), false);

  // Settings page — transport status + editable, persisted device config.
  dash.layout("Настройки", RICON_SIGNAL);
  dash.label("MQTT брокер", &brokerStatus);
  dash.toggle("Bluetooth", &bleOn, [](bool on) { prefs.putBool("ble", on); });   // applies on next boot
  dash.slider("Яркость", &backlight, 5, 50, [](int v) { applyBacklight(); prefs.putInt("bl", v); });  // ≤50% (heat)
  dash.password("PIN освежителя", &freshenerPin, [](const String& v) { prefs.putString("pin", v); });

  // Overview page — at-a-glance home summary, then one composite device card per driver.
  dash.layout("Обзор", RICON_HOME);
  dash.summary("Дом", &homeHeadline, &homeDetail, &homeMood);
  dash.separator("Устройства");
  for (int i = 0; i < NDEV; i++) {
    dash.deviceCard(devices[i].name, devices[i].emoji, &devices[i].power, &devices[i].linked,
        [i](bool on) {
          sendPower(devices[i], on);
          prefs.putBool((String("p") + i).c_str(), on);   // remember across reboots
        })
      .sub(devices[i].transport == T_BLE ? "BLE" : "MQTT");
  }
  // Passive BLE sensor (Xiaomi LYWSD03MMC) — its readings grouped under the device name.
  dash.separator("Спальня · Xiaomi");
  dash.metric("Температура", &senseT, "°C");
  dash.metric("Влажность", &senseH, "%");
  dash.metric("Батарея", &senseBat, "%");

  // Devices page — a dense technical table (à la a Zigbee gateway's device page).
  dash.layout("Устройства", RICON_HOME);
  dash.deviceTable("Devices", &tableData);

  // Map page — radial topology of the hub + its devices.
  dash.layout("Карта", RICON_SIGNAL);
  dash.network("Network", &netData);

  // Discovery page — MQTT visibility.
  dash.layout("Журнал", RICON_SIGNAL);
  dash.badge("MQTT msgs", &msgCount);
  feed = &dash.log("MQTT feed", 6);

  // First boot with no saved Wi-Fi -> RisalDash raises a captive-portal AP "RisalHub" for setup.
  // Once credentials are saved it joins the network and serves the dashboard.
  dash.apName("RisalHub");
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

  lcdRender();        // show the device list before the (blocking) BLE scan, not a frozen splash
  linkBleDevices();   // connect + login BLE drivers once WiFi is up
  // Keep scanning after connecting — passively read sensor beacons (BTHome) alongside the connection.
  if (bleOn) { BLEScan* sc = BLEDevice::getScan(); sc->setActiveScan(false); sc->start(0, nullptr, false); }
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

  if (millis() - lastLcd > 400) {   // recompute the home summary + redraw the panel on any change
    lastLcd = millis();
    uint32_t s = lcdSig();
    if (s != lastSig) {
      lastSig = s;
      int on = 0, issue = 0;
      for (int i = 0; i < NDEV; i++) {
        if (devices[i].power) on++;
        if (devices[i].transport == T_BLE && !devices[i].linked) issue++;
      }
      homeMood = issue ? 1 : 0;
      homeHeadline = issue ? (String(issue) + " офлайн") : (on ? "Работает" : "Всё спокойно");
      homeDetail = String(NDEV) + " устройств · " + String(on) + " вкл";
      netData = "";
      tableData = "";
      for (int i = 0; i < NDEV; i++) {
        Device& d = devices[i];
        int link = d.transport == T_MQTT ? (d.linked ? 96 : 0)                         // local net = strong
                 : (d.linked ? constrain(map(d.rssi, -95, -45, 5, 99), 5, 99) : 0);     // BLE from RSSI
        const char* tr = d.transport == T_BLE ? "BLE" : "MQTT";
        netData += String(d.name) + "~" + d.emoji + "~" + (d.linked ? "1" : "0") + "~" + String(link) + "~" + tr + ";";
        String addr = d.transport == T_BLE ? (d.addr ? String(d.addr->toString().c_str()) : String("scanning")) : String("local");
        tableData += String(d.emoji) + "~" + d.name + "~" + tr + "~" + addr + "~" + String(link) + "~" + (d.linked ? "live" : "lost") + ";";
      }
      lcdRender();
    }
  }
}
