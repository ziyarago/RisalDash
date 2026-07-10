// Real BLE scanner (NimBLE stack on the C6). A continuous, non-blocking scan fills a small
// deduped table (by address) from the BLE task; updateLog() rebuilds the "Nearby" list from it
// every few seconds, strongest signal first. Wi-Fi + BLE share the 2.4 GHz radio by
// time-slicing — this works while the dashboard is served.
#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <RisalUI.h>  // LogWidget

namespace bleScan {

struct Dev { char addr[18]; char name[22]; int rssi; uint32_t seen;
             bool sensor; float temp; int hum; int batt; };  // sensor=true if we decoded a beacon
static const int MAX = 12;
static Dev tab[MAX];
static volatile int cnt = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Decode a Xiaomi-family temperature/humidity beacon from its advertised service data. Handles the
// two common decodable formats: ATC/pvvx custom firmware (service UUID 0x181A) and the stock Xiaomi
// MiBeacon (0xFE95, unencrypted temp/hum objects). Returns true and fills t/h/b on success.
inline bool decodeBeacon(BLEAdvertisedDevice &d, float &t, int &h, int &b) {
  int n = d.getServiceDataCount();
  for (int i = 0; i < n; i++) {
    String uuid = d.getServiceDataUUID(i).toString().c_str();  // e.g. "0x181a" / "0xfe95"
    uuid.toLowerCase();
    String sd = d.getServiceData(i);           // Arduino String; its buffer keeps embedded nulls
    const uint8_t *p = (const uint8_t *)sd.c_str();
    int len = sd.length();
    if (uuid.indexOf("181a") >= 0) {           // ATC / pvvx (LYWSD03MMC custom fw)
      if (len >= 15) {                          // pvvx: temp i16 LE *0.01, hum u16 LE *0.01, batt% @12
        int16_t tt = p[6] | (p[7] << 8);
        uint16_t hh = p[8] | (p[9] << 8);
        t = tt / 100.0f; h = hh / 100; b = p[12]; return true;
      } else if (len >= 10) {                   // ATC: temp i16 BE *0.1, hum u8, batt% @9
        int16_t tt = (p[6] << 8) | p[7];
        t = tt / 10.0f; h = p[8]; b = p[9]; return true;
      }
    } else if (uuid.indexOf("fe95") >= 0 && len >= 5) {  // Xiaomi MiBeacon — scan the objects
      int off = (p[0] & 0x10) ? 11 : 5;         // skip header (+MAC if the "MAC included" bit is set)
      while (off + 3 <= len) {
        uint16_t type = p[off] | (p[off + 1] << 8);
        int dl = p[off + 2];
        const uint8_t *v = p + off + 3;
        if (off + 3 + dl > len) break;
        if (type == 0x1004 && dl >= 2) { t = (int16_t)(v[0] | (v[1] << 8)) / 10.0f; }
        else if (type == 0x1006 && dl >= 2) { h = (uint16_t)(v[0] | (v[1] << 8)) / 10; }
        else if (type == 0x100D && dl >= 4) { t = (int16_t)(v[0] | (v[1] << 8)) / 10.0f; h = (uint16_t)(v[2] | (v[3] << 8)) / 10; }
        else if (type == 0x100A && dl >= 1) { b = v[0]; }
        off += 3 + dl;
      }
      if (t > -100) return true;
    }
  }
  return false;
}

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
    float bt = -1000; int bh = 0, bb = 0;
    bool isSensor = decodeBeacon(d, bt, bh, bb);  // String/parse work outside the critical section
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
      if (isSensor) { tab[idx].sensor = true; tab[idx].temp = bt; tab[idx].hum = bh; tab[idx].batt = bb; }
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

// Fill `out` with up to `maxN` currently-nearby devices (heard in the last 10 s), strongest first.
// Returns the count. Safe to call from the UI/loop task — it snapshots under the spinlock.
inline int snapshot(Dev *out, int maxN) {
  uint32_t now = millis();
  portENTER_CRITICAL(&mux);
  int w = 0;
  for (int i = 0; i < cnt; i++)
    if (now - tab[i].seen <= 10000) tab[w++] = tab[i];  // compact out stale entries
  cnt = w;
  int m = w < maxN ? w : maxN;
  Dev tmp[MAX];
  for (int i = 0; i < w; i++) tmp[i] = tab[i];
  portEXIT_CRITICAL(&mux);
  for (int i = 0; i < w; i++)  // sort by RSSI desc
    for (int j = i + 1; j < w; j++)
      if (tmp[j].rssi > tmp[i].rssi) { Dev t = tmp[i]; tmp[i] = tmp[j]; tmp[j] = t; }
  for (int i = 0; i < m; i++) out[i] = tmp[i];
  return m;
}

}  // namespace bleScan
