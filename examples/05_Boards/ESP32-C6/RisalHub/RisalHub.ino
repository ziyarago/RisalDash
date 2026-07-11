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

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;

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
};

// ── THE DRIVER TABLE — add a device = add a row ──
Device devices[] = {
  { "Air Freshener", T_BLE,  false, false,
    nullptr, nullptr, nullptr,                 // (no MQTT)
    "YGZK", "9999",                            // BLE name prefix + PIN
    {0x2D, 0x09}, 2, {0x2D, 0x08}, 2,          // on / off command bytes
    nullptr, nullptr },
  { "Sonoff Plug",   T_MQTT, false, true,
    "cmnd/plug/POWER", "ON", "OFF",            // MQTT command topic + payloads
    nullptr, nullptr, {0}, 0, {0}, 0,          // (no BLE)
    nullptr, nullptr },
};
const int NDEV = sizeof(devices) / sizeof(devices[0]);

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

void linkBleDevices() {
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

  for (int i = 0; i < NDEV; i++) {
    Device& d = devices[i];
    if (d.transport != T_BLE || !d.addr) continue;
    BLEClient* c = BLEDevice::createClient();
    if (!c->connect(*d.addr)) continue;
    // HM-10 UART (ffe2) first, then the vendor ae30 write chars.
    d.ch = pick(c->getService(BLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb")), "0000ffe2-0000-1000-8000-00805f9b34fb");
    if (!d.ch) {
      BLERemoteService* ae = c->getService(BLEUUID("0000ae30-0000-1000-8000-00805f9b34fb"));
      d.ch = pick(ae, "0000ae01-0000-1000-8000-00805f9b34fb");
      if (!d.ch) d.ch = pick(ae, "0000ae03-0000-1000-8000-00805f9b34fb");
    }
    if (!d.ch) continue;
    if (d.pin) {                                     // login: 0x8F + PIN
      uint8_t login[1 + 8]; login[0] = 0x8F;
      size_t pl = strlen(d.pin); memcpy(login + 1, d.pin, pl);
      d.ch->writeValue(login, 1 + pl, false);
    }
    d.linked = true;
  }
}

void setup() {
  dash.timezone(300);

  // Devices page — a control card per driver.
  dash.layout("Devices", RICON_HOME);
  for (int i = 0; i < NDEV; i++) {
    dash.toggle(devices[i].name, &devices[i].power, [i](bool on) { sendPower(devices[i], on); });
    dash.led(devices[i].transport == T_BLE ? "  linked" : "  online", &devices[i].linked);
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

void loop() {
  dash.update();
  mqtt.loop();
}
