// RisalHub (schema 1) — an all-in-one smart-home hub on a SINGLE ESP32-C6: it runs its OWN MQTT broker
// (PicoMQTT) AND a beautiful RisalDash web dashboard at the same time. No PC, no Raspberry Pi, no cloud.
// Point your Tasmota / Zigbee2MQTT devices' MQTT host at THIS board's IP; their readings show up.
//
// TWO pages:
//   • Discovery — a live feed of EVERY topic hitting the broker (subscribe "#") plus message/topic
//     counters. This is the "what's on my network, where from" view — you SEE devices before mapping.
//   • Home — a few example tiles mapped to specific topics (edit these to your own devices).
//
// Mapping is still by hand here (one subscribe → one variable). Auto-building tiles from Home Assistant
// discovery configs (homeassistant/.../config, which Tasmota & Zigbee2MQTT publish) is the next step and
// needs dynamic widgets in the library — see the notes at the bottom.
//
// Radios: WiFi only. Zigbee devices reach the hub through an external bridge that publishes MQTT.
// Libraries: RisalDash, mlesniew/PicoMQTT, bblanchon/ArduinoJson.
#include <RisalUI.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;   // the on-board broker, listening on :1883

// ── Discovery: a live registry of distinct topics + a rolling feed ──
LogWidget* feed = nullptr;
int msgCount = 0, topicCount = 0;
String seenTopics[32];
int seenN = 0;
bool firstSeen(const String& t) {
  for (int i = 0; i < seenN; i++) if (seenTopics[i] == t) return false;
  if (seenN < 32) seenTopics[seenN++] = t;
  topicCount = seenN;
  return true;
}

// ── Home: example tiles mapped to specific device topics (edit to your own) ──
float kitchenT = 0, kitchenH = 0;
bool  plug = false;

void setup() {
  dash.timezone(300);

  // Page 1 — see everything on the broker.
  dash.layout("Discovery", RICON_SIGNAL);
  dash.badge("Messages", &msgCount);
  dash.badge("Topics", &topicCount);
  feed = &dash.log("Live MQTT feed", 8);

  // Page 2 — mapped tiles.
  dash.layout("Home", RICON_HOME);
  dash.metric("Kitchen temp", &kitchenT, "C");
  dash.metric("Kitchen humidity", &kitchenH, "%");
  dash.toggle("Plug", &plug, [](bool on) {
    mqtt.publish("cmnd/plug/POWER", on ? "ON" : "OFF");
  });

  dash.begin();   // station mode (saved creds) — devices set their MQTT host to this board's IP

  // ONE handler for everything: PicoMQTT delivers a message to a single matching subscription, so a
  // broad "#" would shadow narrower ones. Subscribe once to "#", log it for Discovery, then route by
  // topic to the mapped tiles. Add your own `else if` per device — this is where mapping lives.
  mqtt.subscribe("#", [](const char* topic, const char* payload) {
    msgCount++;
    firstSeen(String(topic));
    String line = String(topic) + " = " + payload;
    if (line.length() > 64) line = line.substring(0, 63) + "…";
    if (feed) feed->print(line);

    String t = topic;
    if (t == "tele/kitchen/SENSOR") {
      JsonDocument doc;
      if (deserializeJson(doc, payload)) return;
      if (!doc["BME280"]["Temperature"].isNull()) kitchenT = doc["BME280"]["Temperature"].as<float>();
      if (!doc["BME280"]["Humidity"].isNull())    kitchenH = doc["BME280"]["Humidity"].as<float>();
    } else if (t == "stat/plug/POWER") {
      plug = (String(payload) == "ON");
    }
  });

  mqtt.begin();
}

void loop() {
  dash.update();
  mqtt.loop();
}
