// RisalHub (schema 3, EXPERIMENTAL) — the "everything on one chip" hub: a single ESP32-C6 acts as a
// native Zigbee COORDINATOR on its 802.15.4 radio AND serves the RisalDash web dashboard over WiFi at
// the same time. If this coexistence is stable, no Sonoff bridge, no PC, no Pi, no cloud — the C6 pairs
// Zigbee sensors directly and renders them beautifully. That is exactly what this sketch is here to
// PROVE: does WiFi + Zigbee actually run together on one C6 under Arduino?
//
// ⚠ STATUS: R&D / unverified. Espressif supports WiFi + 802.15.4 coexistence in ESP-IDF (the Zigbee
// Gateway example), but on the Arduino side running both together is not a well-trodden path. Treat
// this as a probe, not a finished feature. Schema 1 (ESP32-C6-Hub, external bridge + MQTT) is the
// stable option meanwhile.
//
// BUILD REQUIREMENTS (this will NOT compile as a plain sketch):
//   • ESP32 core 3.x with the Zigbee library, C6/H2 only (needs the 802.15.4 radio).
//   • Arduino IDE:  Tools → Zigbee mode → "Zigbee ZCZR (coordinator/router)"  + a Zigbee partition scheme.
//   • PlatformIO:   a Zigbee-enabled partition table + the Zigbee build defines; WiFi/802.15.4 coexistence
//                   must be enabled in sdkconfig (CONFIG_ESP_COEX_SW_COEXIST_ENABLE). This is the hard part.
//   • Libraries:    RisalDash, and the core-bundled "Zigbee.h".
#include <RisalUI.h>
#include "Zigbee.h"

#define WIFI_SSID "your-wifi"
#define WIFI_PASS "your-pass"

RisalUI dash("Risal Zigbee Hub");

// A coordinator that binds to remote Zigbee temperature sensors and receives their reports.
// (ZigbeeThermostat is the core's coordinator-role endpoint that consumes temperature clusters.)
ZigbeeThermostat zbCoord(10);   // endpoint id

float zbTemp = 0;      // last temperature received over Zigbee
int   zbDevices = 0;   // devices bound to the coordinator
bool  zbReady = false; // did the Zigbee stack come up alongside WiFi?

// Called by the Zigbee stack when a bound sensor reports a new temperature.
void onZbTemperature(float t) {
  zbTemp = t;
}

void setup() {
  dash.timezone(300);

  // 1) WiFi + dashboard first (the web UI must be reachable regardless of Zigbee).
  dash.layout("Zigbee", RICON_HOME);
  dash.led("Zigbee up", &zbReady);
  dash.badge("Paired devices", &zbDevices);
  dash.metric("Zigbee temp", &zbTemp, "C");
  dash.begin(WIFI_SSID, WIFI_PASS);

  // 2) Zigbee coordinator on the SAME chip — the coexistence moment of truth.
  zbCoord.onTempReceive(onZbTemperature);
  Zigbee.addEndpoint(&zbCoord);
  zbReady = Zigbee.begin(ZIGBEE_COORDINATOR);   // returns false if the stack fails to start
  if (zbReady) {
    Zigbee.openNetwork(180);                     // permit join for 3 min so sensors can pair
  }
}

void loop() {
  dash.update();
  zbDevices = zbCoord.getBoundDevices().size();  // reflect how many sensors have paired
  delay(10);
}
