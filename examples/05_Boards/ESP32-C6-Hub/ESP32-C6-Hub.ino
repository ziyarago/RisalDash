// RisalHub (schema 1) — an all-in-one smart-home hub on a SINGLE ESP32-C6: it runs its OWN MQTT broker
// (PicoMQTT) AND a beautiful RisalDash web dashboard at the same time. No PC, no Raspberry Pi, no cloud.
// Point your Tasmota / Zigbee2MQTT devices' MQTT host at THIS board's IP and their readings show up as
// live tiles; a toggle publishes a command straight back to a device.
//
// Radios used here: WiFi only. Zigbee devices reach the hub through an EXTERNAL bridge
// (Sonoff Bridge / Zigbee2MQTT) that publishes MQTT. Pairing Zigbee natively on the C6's own 802.15.4
// radio is schema 3 — a separate experimental sketch (ESP32-C6-Zigbee-Hub), because running WiFi and
// Zigbee together on one chip is still R&D on the Arduino side.
//
// Libraries: RisalDash, mlesniew/PicoMQTT, bblanchon/ArduinoJson.
#include <RisalUI.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>

#define WIFI_SSID "your-wifi"
#define WIFI_PASS "your-pass"

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;   // the on-board broker, listening on :1883

// Live values fed by incoming MQTT — bound straight to widgets.
float kitchenT = 0, kitchenH = 0;
bool  plug = false;

void setup() {
  dash.timezone(300);

  dash.layout("Home", RICON_HOME);
  dash.metric("Kitchen temp", &kitchenT, "C");
  dash.metric("Kitchen humidity", &kitchenH, "%");
  dash.toggle("Plug", &plug, [](bool on) {
    mqtt.publish("cmnd/plug/POWER", on ? "ON" : "OFF");   // command a Tasmota plug
  });

  dash.begin(WIFI_SSID, WIFI_PASS);   // station mode — devices set their MQTT host to this board's IP

  // The broker can subscribe to its own traffic: decode what devices publish, update the tiles.
  mqtt.subscribe("tele/kitchen/SENSOR", [](const char* topic, const char* payload) {
    JsonDocument doc;                                   // Tasmota BME280 telemetry
    if (deserializeJson(doc, payload)) return;          // {"BME280":{"Temperature":23.5,"Humidity":48}}
    if (!doc["BME280"]["Temperature"].isNull()) kitchenT = doc["BME280"]["Temperature"].as<float>();
    if (!doc["BME280"]["Humidity"].isNull())    kitchenH = doc["BME280"]["Humidity"].as<float>();
  });
  mqtt.subscribe("stat/plug/POWER", [](const char* topic, const char* payload) {
    plug = (String(payload) == "ON");                   // reflect the real relay state
  });

  mqtt.begin();
}

void loop() {
  dash.update();
  mqtt.loop();   // service the broker (accept clients, route messages)
}
