// RisalHub Discovery Center (PoC) — a single ESP32-C6 that discovers devices across MULTIPLE transports
// and turns them into beautiful tiles, no per-device code:
//   • MQTT  — its own broker (PicoMQTT); parses Home Assistant discovery configs → runtime tiles.
//   • BLE   — scans continuously (the C6's own radio); lists nearby devices with live RSSI.
//   • Zigbee— shown as a slot for schema 3 (native coordinator + WiFi coexistence — separate sketch).
//
// Three pages: Discovery (transport status + MQTT feed), Bluetooth (nearby BLE list), Devices (the
// runtime-discovered tiles). Everything feeds the same runtime-widget engine + "_struct" live reload.
//
// Build:  -D RISAL_MAX_WIDGETS=80   (headroom for discovered tiles)
// Libs:   RisalDash, mlesniew/PicoMQTT, bblanchon/ArduinoJson  (BLE is core: <BLEDevice.h>)
#include <RisalUI.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;

// ── Transport status (Discovery page) ──
int msgCount = 0, mqttDevices = 0, bleDevices = 0;
bool bleOn = true;
String zbStatus = "off - schema 3";

LogWidget* feed = nullptr;
LogWidget* bleList = nullptr;

// ── MQTT auto-discovery registry ──
struct Ent { String cfgTopic, name, stateTopic, unit; float fval; bool bval; char kind; };
Ent ents[24];
int entN = 0;
bool known(const String& c) { for (int i = 0; i < entN; i++) if (ents[i].cfgTopic == c) return true; return false; }

void addDiscovered(const String& cfgTopic, const String& comp, JsonDocument& d) {
  if (entN >= 24 || known(cfgTopic)) return;
  Ent& e = ents[entN];
  e.cfgTopic = cfgTopic;
  e.name = (const char*)(d["name"] | "Device");
  e.stateTopic = (const char*)(d["state_topic"] | "");
  e.unit = (const char*)(d["unit_of_measurement"] | "");
  e.fval = 0; e.bval = false;
  if (comp == "switch" || comp == "light") {
    e.kind = 't';
    String cmd = (const char*)(d["command_topic"] | "");
    dash.toggle(e.name.c_str(), &e.bval, [cmd](bool on) { if (cmd.length()) mqtt.publish(cmd.c_str(), on ? "ON" : "OFF"); });
  } else if (comp == "binary_sensor") {
    e.kind = 'l'; dash.led(e.name.c_str(), &e.bval);
  } else {
    e.kind = 'm'; dash.metric(e.name.c_str(), &e.fval, e.unit.c_str());
  }
  entN++; mqttDevices = entN;
}

// ── BLE scan: a small live table, filled from the BLE task under a spinlock ──
struct BleDev { char addr[18]; char name[22]; int rssi; uint32_t seen; };
static const int BMAX = 16;
static BleDev btab[BMAX];
static int bcnt = 0;
static portMUX_TYPE bmux = portMUX_INITIALIZER_UNLOCKED;

class ScanCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    char addr[18]; strncpy(addr, d.getAddress().toString().c_str(), 17); addr[17] = 0;
    char name[22];
    if (d.haveName()) { strncpy(name, d.getName().c_str(), 21); name[21] = 0; } else strcpy(name, "(no name)");
    int rssi = d.getRSSI();
    uint32_t now = millis();
    portENTER_CRITICAL(&bmux);
    int idx = -1;
    for (int i = 0; i < bcnt; i++) if (strcmp(btab[i].addr, addr) == 0) { idx = i; break; }
    if (idx < 0 && bcnt < BMAX) idx = bcnt++;
    if (idx >= 0) { strcpy(btab[idx].addr, addr); strcpy(btab[idx].name, name); btab[idx].rssi = rssi; btab[idx].seen = now; }
    portEXIT_CRITICAL(&bmux);
  }
};
static ScanCB scanCb;
static BLEScan* bleScan = nullptr;

void bleBegin() {
  BLEDevice::init("RisalHub");
  bleScan = BLEDevice::getScan();
  bleScan->setAdvertisedDeviceCallbacks(&scanCb, true);
  bleScan->setActiveScan(true);
  bleScan->setInterval(160);
  bleScan->setWindow(80);
  bleScan->start(0, nullptr, false);   // scan forever, non-blocking
}

// Rebuild the "Nearby BLE" list from the live table (throttled by the caller).
void bleRefresh() {
  if (!bleList) return;
  BleDev snap[BMAX]; int n;
  portENTER_CRITICAL(&bmux);
  n = bcnt; for (int i = 0; i < n; i++) snap[i] = btab[i];
  portEXIT_CRITICAL(&bmux);
  bleDevices = n;
  // Show up to 5 strongest, most-recent devices as "name  rssidBm".
  // (The log holds the latest lines; print in RSSI order for a stable-ish view.)
  for (int pass = 0; pass < 5 && pass < n; pass++) {
    int best = -1;
    for (int i = 0; i < n; i++) if (snap[i].rssi > -127 && (best < 0 || snap[i].rssi > snap[best].rssi)) best = i;
    if (best < 0) break;
    String line = String(snap[best].name) + "  " + snap[best].rssi + "dBm";
    bleList->print(line);
    snap[best].rssi = -127;  // mark consumed
  }
}

void setup() {
  dash.timezone(300);

  // Page 1 — Discovery: transport status + MQTT feed.
  dash.layout("Discovery", RICON_SIGNAL);
  dash.badge("MQTT devices", &mqttDevices);
  dash.badge("BLE devices", &bleDevices);
  dash.badge("Messages", &msgCount);
  dash.toggle("BLE scan", &bleOn, [](bool on) { if (bleScan) { if (on) bleScan->start(0, nullptr, false); else bleScan->stop(); } });
  dash.label("Zigbee", &zbStatus);
  feed = &dash.log("MQTT feed", 5);

  // Page 2 — Bluetooth: nearby devices.
  dash.layout("Bluetooth", RICON_WIFI);
  bleList = &dash.log("Nearby BLE", 5);

  // Page 3 — Devices: runtime-discovered MQTT tiles land here.
  dash.layout("Devices", RICON_HOME);

  dash.begin();

  mqtt.subscribe("#", [](const char* topic, const char* payload) {
    msgCount++;
    String t = topic;
    String line = t + " = " + payload;
    if (line.length() > 60) line = line.substring(0, 59) + "…";
    if (feed) feed->print(line);
    if (t.startsWith("homeassistant/") && t.endsWith("/config")) {
      int s1 = t.indexOf('/'), s2 = t.indexOf('/', s1 + 1);
      JsonDocument d;
      if (!deserializeJson(d, payload)) addDiscovered(t, t.substring(s1 + 1, s2), d);
      return;
    }
    for (int i = 0; i < entN; i++)
      if (ents[i].stateTopic.length() && ents[i].stateTopic == t) {
        if (ents[i].kind == 'm') ents[i].fval = String(payload).toFloat();
        else { String p = payload; ents[i].bval = (p == "ON" || p == "on" || p == "1" || p == "true"); }
      }
  });
  mqtt.begin();

  bleBegin();
}

uint32_t lastBle = 0;
void loop() {
  dash.update();
  mqtt.loop();
  if (millis() - lastBle > 3000) { lastBle = millis(); bleRefresh(); }  // refresh the BLE list
}
