// Control a RASMA / IAA360 "ChengHao" BLE aroma diffuser from a RisalDash dashboard on the ESP32-C6.
// The protocol was recovered from the vendor APK (com.bluetooth.rasma): commands are written RAW to the
// device's write characteristic (no framing/CRC). A login frame must be sent first:
//
//   login    : 0x8F + <password ASCII>        (device PIN, 4 digits, e.g. "9999" — factory default "0000")
//   power on : 0x2D 0x09                        (total-control byte: lamp<4> 1<3> demo<2> fan<1> onOff<0>)
//   power off: 0x2D 0x08
//   aroma    : 0x2A <index> 0x02 <fan/fog bits> 0x00   (5 bytes)
//
// Write characteristic: the vendor's fff0/fff6 model differs per hardware. This unit (YGZK-XXJ-TJ)
// exposes an HM-10 style UART (ffe0/ffe2) plus a vendor service (ae30). We write to ffe2 (with an
// ae01/ae03 fallback). VERIFIED on hardware: login 0x8F+PIN then 0x2D 0x09 / 0x2D 0x08 turns the
// diffuser on/off (confirmed by stopping it mid-timer).
#include <RisalUI.h>
#include <BLEDevice.h>

#define TARGET   "YGZK"       // freshener advertised-name prefix
#define PASSWORD "9999"       // your device PIN (4 digits; factory default is usually "0000")

RisalUI dash("Freshener");
bool power = false, linked = false;

static BLEClient* client = nullptr;
static BLERemoteCharacteristic* writeChar = nullptr;
static BLEAddress* target = nullptr;

class ScanCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    if (!target && d.haveName() && String(d.getName().c_str()).startsWith(TARGET))
      target = new BLEAddress(d.getAddress());
  }
};

static bool writeCmd(const uint8_t* data, size_t len) {
  if (!writeChar) return false;
  writeChar->writeValue((uint8_t*)data, len, false);   // write-no-response, raw payload
  return true;
}

static BLERemoteCharacteristic* pick(BLERemoteService* s, const char* uuid) {
  if (!s) return nullptr;
  return s->getCharacteristic(BLEUUID(uuid));
}

void linkFreshener() {
  BLEDevice::init("RisalFreshener");
  BLEScan* scan = BLEDevice::getScan();
  static ScanCB cb;
  scan->setAdvertisedDeviceCallbacks(&cb);
  scan->setActiveScan(true);
  for (int i = 0; i < 8 && !target; i++) scan->start(3, false);
  scan->stop();
  if (!target) return;

  client = BLEDevice::createClient();
  if (!client->connect(*target)) return;

  // HM-10 UART first, then the vendor ae30 write characteristics.
  writeChar = pick(client->getService(BLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb")),
                   "0000ffe2-0000-1000-8000-00805f9b34fb");
  if (!writeChar) {
    BLERemoteService* ae = client->getService(BLEUUID("0000ae30-0000-1000-8000-00805f9b34fb"));
    writeChar = pick(ae, "0000ae01-0000-1000-8000-00805f9b34fb");
    if (!writeChar) writeChar = pick(ae, "0000ae03-0000-1000-8000-00805f9b34fb");
  }
  if (!writeChar) return;

  // Login: 0x8F + password bytes.
  uint8_t login[1 + 16];
  login[0] = 0x8F;
  size_t pl = strlen(PASSWORD);
  memcpy(login + 1, PASSWORD, pl);
  writeCmd(login, 1 + pl);
  linked = true;
}

void setup() {
  dash.timezone(300);
  dash.layout("Aroma", RICON_HOME);
  dash.led("Linked", &linked);
  dash.toggle("Power", &power, [](bool on) {
    uint8_t cmd[2] = {0x2D, (uint8_t)(on ? 0x09 : 0x08)};
    writeCmd(cmd, 2);
  });
  dash.begin();
  linkFreshener();   // scan + connect + login once WiFi is up
}

void loop() {
  dash.update();
}
