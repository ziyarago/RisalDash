// BLE GATT inspector — "crack open" any BLE device: scan for it by name, connect as a central, and dump
// its whole GATT (services + characteristics + properties + current values) to Serial. Use it to find
// the write characteristic that controls an unknown gadget (e.g. a BLE aroma diffuser) before writing a
// proper controller. Standalone (no WiFi) so the full dump is easy to read over USB serial.
//
// Set TARGET to a name prefix from the scan (or hard-set TARGET_MAC). Runs on ESP32 / ESP32-C6.
#include <BLEDevice.h>

#define TARGET "YGZK"   // name prefix to match (the aroma diffuser advertised YGZK-XXJ-TJ-...)

static BLEAddress* found = nullptr;
static String foundName;

class ScanCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    if (found) return;
    if (d.haveName() && String(d.getName().c_str()).startsWith(TARGET)) {
      found = new BLEAddress(d.getAddress());
      foundName = d.getName().c_str();
    }
  }
};

static void hexdump(const String& v) {
  Serial.print("      = ");
  for (size_t i = 0; i < v.length(); i++) Serial.printf("%02X ", (uint8_t)v[i]);
  Serial.println();
}

static void dumpGatt(BLEClient* c) {
  auto* services = c->getServices();   // discovery ran on connect(); this returns the map
  Serial.printf("services: %u\n", (unsigned)services->size());
  for (auto& sp : *services) {
    BLERemoteService* s = sp.second;
    Serial.printf("SERVICE %s\n", s->getUUID().toString().c_str());
    auto* chars = s->getCharacteristics();
    for (auto& cp : *chars) {
      BLERemoteCharacteristic* ch = cp.second;
      Serial.printf("  CHAR %s  [%s%s%s%s]\n",
                    ch->getUUID().toString().c_str(),
                    ch->canRead() ? "R" : "-",
                    ch->canWrite() ? "W" : "-",
                    ch->canWriteNoResponse() ? "w" : "-",
                    ch->canNotify() ? "N" : "-");
      if (ch->canRead()) hexdump(String(ch->readValue().c_str()));
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println();
  Serial.println("== BLE inspect: scanning for '" TARGET "' ==");
  BLEDevice::init("RisalInspect");
  BLEScan* scan = BLEDevice::getScan();
  static ScanCB cb;
  scan->setAdvertisedDeviceCallbacks(&cb);
  scan->setActiveScan(true);
  for (int tries = 0; tries < 10 && !found; tries++) scan->start(3, false);
  scan->stop();

  if (!found) { Serial.println("not found — is it advertising / in range?"); return; }
  Serial.printf("found %s @ %s — connecting...\n", foundName.c_str(), found->toString().c_str());

  BLEClient* c = BLEDevice::createClient();
  if (c->connect(*found)) {
    Serial.println("CONNECTED. dumping GATT (R=read W=write w=write-no-resp N=notify):");
    dumpGatt(c);
    Serial.println("== done ==");
    c->disconnect();
  } else {
    Serial.println("connect failed — device may require bonding/pairing.");
  }
}

void loop() { delay(1000); }
