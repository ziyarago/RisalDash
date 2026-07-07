// Optional MQTT mirror + Home Assistant discovery (-D RISAL_ENABLE_MQTT).
#include "RisalUI.h"
#include "RisalInternal.h"

#ifdef RISAL_ENABLE_MQTT
RisalUI* RisalUI::_self = nullptr;

RisalUI& RisalUI::mqtt(const char* host, uint16_t port, const char* baseTopic) {
  _mqttHost = host;
  _mqttPort = port;
  _mqttBase = baseTopic;
  _self = this;
  return *this;
}

// Inbound: <base>/<key>/set -> applyCommand(payload).
void RisalUI::_mqttCb(char* topic, uint8_t* payload, unsigned int len) {
  if (!_self) return;
  String t(topic);
  String base = String(_self->_mqttBase) + "/";
  if (!t.startsWith(base) || !t.endsWith("/set")) return;
  String key = t.substring(base.length(), t.length() - 4);  // strip base.../ and /set
  String val;
  for (unsigned int i = 0; i < len; i++) val += (char)payload[i];
  for (uint8_t i = 0; i < _self->_count; i++)
    if (key == _self->_widgets[i]->key()) { _self->_widgets[i]->applyCommand(val); break; }
}

// Outbound: publish a widget's current value (retained) to <base>/<key>.
void RisalUI::_mqttPublish(Widget* w) {
  if (!w->hasState()) return;
  String kv;
  w->writeKV(kv);  // "key":value or "key":"value"
  int c = kv.indexOf(':');
  if (c < 0) return;
  String val = kv.substring(c + 1);
  if (val.length() && val[0] == '"') val = val.substring(1, val.length() - 1);  // unquote strings
  String topic = String(_mqttBase) + "/" + w->key();
  _mqtt.publish(topic.c_str(), val.c_str(), true);
}

void RisalUI::_mqttLoop() {
  if (!_mqttHost) return;
  if (_mqtt.connected()) { _mqtt.loop(); return; }
  if (millis() < _mqttRetry) return;
  _mqttRetry = millis() + 3000;  // throttle reconnects
  _mqtt.setServer(_mqttHost, _mqttPort);
  _mqtt.setCallback(_mqttCb);
  if (_ha) _mqtt.setBufferSize(640);  // HA discovery configs exceed the 256-byte default
  String cid = "risaldash-" + WiFi.macAddress();
  if (!_mqtt.connect(cid.c_str())) return;
  for (uint8_t i = 0; i < _count; i++) {
    if (_widgets[i]->hasState()) {
      String sub = String(_mqttBase) + "/" + _widgets[i]->key() + "/set";
      _mqtt.subscribe(sub.c_str());
      _mqttPublish(_widgets[i]);  // seed retained state
    }
    if (_ha) _haDiscover(_widgets[i]);  // HA auto-discovery (also subscribes stateless controls)
  }
}

// Home Assistant MQTT discovery for one widget: maps the widget type to an HA component and
// publishes a retained config to homeassistant/<component>/<node>/<slug>/config. State/command
// topics reuse the existing <base>/<key> mirror, so HA shares the same topics as the dashboard.
void RisalUI::_haDiscover(Widget* w) {
  const char* ty = w->typeId();
  const char* comp;
  bool cmd = false, isSwitch = false, isBinary = false, isButton = false;
  if (!strcmp(ty, "metric") || !strcmp(ty, "gauge") || !strcmp(ty, "stat") ||
      !strcmp(ty, "chart") || !strcmp(ty, "progress") || !strcmp(ty, "badge")) comp = "sensor";
  else if (!strcmp(ty, "toggle")) { comp = "switch"; cmd = true; isSwitch = true; }
  else if (!strcmp(ty, "led")) { comp = "binary_sensor"; isBinary = true; }
  else if (!strcmp(ty, "slider") || !strcmp(ty, "number")) { comp = "number"; cmd = true; }
  else if (!strcmp(ty, "button")) { comp = "button"; cmd = true; isButton = true; }
  else return;  // type without a clean HA mapping → skip

  String key = w->key();
  String slug;  // HA-safe id: lowercase alnum, others → '_'
  for (size_t i = 0; i < key.length(); i++) {
    char c = key[i];
    if (c >= 'A' && c <= 'Z') slug += (char)(c + 32);
    else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) slug += c;
    else slug += '_';
  }
  String base = String(_mqttBase) + "/" + key;  // raw key — matches _mqttPublish/_mqttCb

  String cfg = "{\"name\":\"";
  cfg += key;
  cfg += "\",\"uniq_id\":\"";
  cfg += _haNode; cfg += "_"; cfg += slug;
  cfg += "\",";
  if (isButton) {
    cfg += "\"cmd_t\":\""; cfg += base; cfg += "/set\",\"pl_prs\":\"true\",";
  } else {
    cfg += "\"stat_t\":\""; cfg += base; cfg += "\",";
    if (cmd) { cfg += "\"cmd_t\":\""; cfg += base; cfg += "/set\","; }
    if (isSwitch) cfg += "\"pl_on\":\"true\",\"pl_off\":\"false\",\"stat_on\":\"true\",\"stat_off\":\"false\",";
    if (isBinary) cfg += "\"pl_on\":\"true\",\"pl_off\":\"false\",";
  }
  cfg += "\"dev\":{\"ids\":[\""; cfg += _haNode;
  cfg += "\"],\"name\":\""; cfg += _title;
  cfg += "\",\"mf\":\"RisalDash\"}}";

  if (cmd) { String sub = base + "/set"; _mqtt.subscribe(sub.c_str()); }  // covers stateless button

  String topic = "homeassistant/";
  topic += comp; topic += "/"; topic += _haNode; topic += "/"; topic += slug; topic += "/config";
  _mqtt.publish(topic.c_str(), cfg.c_str(), true);  // retained
}
#endif
