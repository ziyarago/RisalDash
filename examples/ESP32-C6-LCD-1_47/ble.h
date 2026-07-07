// Real BLE scanner (NimBLE stack on the C6). A continuous, non-blocking scan fills a small
// deduped table (by address) from the BLE task; updateLog() rebuilds the "Nearby" list from it
// every few seconds, strongest signal first. Wi-Fi + BLE share the 2.4 GHz radio by
// time-slicing — this works while the dashboard is served.
#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <RisalUI.h>  // LogWidget

namespace bleScan {

struct Dev { char addr[18]; char name[22]; int rssi; uint32_t seen; };
static const int MAX = 12;
static Dev tab[MAX];
static volatile int cnt = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Called from the BLE task for every advertisement. Keep the String work outside the critical
// section; only the tiny table upsert runs with the spinlock held.
class ScanCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    char addr[18];
    strncpy(addr, d.getAddress().toString().c_str(), 17);
    addr[17] = 0;
    char name[22];
    if (d.haveName()) { strncpy(name, d.getName().c_str(), 21); name[21] = 0; }
    else strcpy(name, "(no name)");
    int rssi = d.getRSSI();
    uint32_t now = millis();
    portENTER_CRITICAL(&mux);
    int idx = -1;
    for (int i = 0; i < cnt; i++)
      if (strcmp(tab[i].addr, addr) == 0) { idx = i; break; }
    if (idx < 0 && cnt < MAX) idx = cnt++;
    if (idx >= 0) {
      strcpy(tab[idx].addr, addr);
      strcpy(tab[idx].name, name);
      tab[idx].rssi = rssi;
      tab[idx].seen = now;
    }
    portEXIT_CRITICAL(&mux);
  }
};
static ScanCB cb;

inline void begin() {  // start the continuous async scan (called after Wi-Fi is up)
  BLEDevice::init("RisalDash");
  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(&cb, true);  // report duplicates so RSSI stays fresh
  scan->setActiveScan(true);                       // ask for names (a bit more power)
  scan->setInterval(160);
  scan->setWindow(80);
  scan->start(0, nullptr, false);  // duration 0 = scan forever, non-blocking
}

// Rebuild the "Nearby" list from the live table (throttled inside; call from loop()).
inline void updateLog(LogWidget *log) {
  static uint32_t last = 0;
  if (!log || millis() - last < 2500) return;
  last = millis();
  Dev snap[MAX];
  uint32_t now = millis();
  portENTER_CRITICAL(&mux);
  int w = 0;  // compact out devices not heard from in the last 10 s
  for (int i = 0; i < cnt; i++)
    if (now - tab[i].seen <= 10000) tab[w++] = tab[i];
  cnt = w;
  for (int i = 0; i < w; i++) snap[i] = tab[i];
  portEXIT_CRITICAL(&mux);
  for (int i = 0; i < w; i++)  // strongest signal first
    for (int j = i + 1; j < w; j++)
      if (snap[j].rssi > snap[i].rssi) { Dev t = snap[i]; snap[i] = snap[j]; snap[j] = t; }
  if (w == 0) {
    log->print("scanning...");
  } else {
    int show = w < 5 ? w : 5;
    for (int i = 0; i < show; i++)
      log->print(String(snap[i].name) + "  " + snap[i].rssi + "dBm");
  }
}

}  // namespace bleScan
