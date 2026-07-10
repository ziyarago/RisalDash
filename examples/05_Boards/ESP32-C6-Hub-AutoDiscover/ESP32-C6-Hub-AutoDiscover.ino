// RisalHub Auto-Discovery (PoC) — the hub BUILDS ITS OWN TILES AT RUNTIME from Home Assistant MQTT
// discovery configs. Tasmota and Zigbee2MQTT publish `homeassistant/<component>/.../config` describing
// each entity (name, state topic, unit); the hub parses those and creates the matching widget on the
// fly — zero per-device code. New tiles appear live (the "_struct" revision tells open clients to
// reload). This is the payoff of dynamic widgets: plug a device in, it shows up.
//
// Needs the RisalDash runtime-widget support (_add works after begin(), "_struct" reload signal).
// Build with headroom for discovered tiles:  -D RISAL_MAX_WIDGETS=80
// Libraries: RisalDash, mlesniew/PicoMQTT, bblanchon/ArduinoJson.
#include <RisalUI.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>

RisalUI dash("Risal Hub");
PicoMQTT::Server mqtt;

LogWidget* feed = nullptr;
int msgCount = 0, deviceCount = 0;

// Registry of discovered entities. Fixed array = stable addresses, so a widget can bind to &e.fval and
// use e.name.c_str() for its whole life.
struct Ent { String cfgTopic, name, stateTopic, unit; float fval; bool bval; char kind; };
Ent ents[24];
int entN = 0;

bool known(const String& cfg) { for (int i = 0; i < entN; i++) if (ents[i].cfgTopic == cfg) return true; return false; }

// Turn one HA discovery config into a live tile.
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
    e.kind = 'l';
    dash.led(e.name.c_str(), &e.bval);
  } else {  // sensor (and anything else) -> a metric readout
    e.kind = 'm';
    dash.metric(e.name.c_str(), &e.fval, e.unit.c_str());
  }
  entN++;
  deviceCount = entN;   // the new widget bumped the structure revision -> open clients reload
}

void setup() {
  dash.timezone(300);

  dash.layout("Discovery", RICON_SIGNAL);
  dash.badge("Messages", &msgCount);
  dash.badge("Devices", &deviceCount);
  feed = &dash.log("Live MQTT feed", 6);

  dash.layout("Devices", RICON_HOME);   // runtime-discovered tiles get appended onto this page

  dash.begin();

  mqtt.subscribe("#", [](const char* topic, const char* payload) {
    msgCount++;
    String t = topic;
    String line = t + " = " + payload;
    if (line.length() > 64) line = line.substring(0, 63) + "…";
    if (feed) feed->print(line);

    // Home Assistant discovery config -> build a tile.
    if (t.startsWith("homeassistant/") && t.endsWith("/config")) {
      int s1 = t.indexOf('/'), s2 = t.indexOf('/', s1 + 1);
      String comp = t.substring(s1 + 1, s2);   // sensor / switch / binary_sensor / ...
      JsonDocument d;
      if (!deserializeJson(d, payload)) addDiscovered(t, comp, d);
      return;
    }

    // State update for a discovered entity? (PoC: plain value on the state topic, no value_template.)
    for (int i = 0; i < entN; i++) {
      if (ents[i].stateTopic.length() && ents[i].stateTopic == t) {
        if (ents[i].kind == 'm') ents[i].fval = String(payload).toFloat();
        else { String p = payload; ents[i].bval = (p == "ON" || p == "on" || p == "1" || p == "true"); }
      }
    }
  });

  mqtt.begin();
}

void loop() {
  dash.update();
  mqtt.loop();
}
