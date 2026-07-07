// Integration surface: REST /api/set, Prometheus /metrics, the MCP manifest.
#include "RisalUI.h"
#include "RisalInternal.h"

// REST: apply one or more key=value params (query string or form body) to widgets.
void RisalUI::_handleSet(AsyncWebServerRequest* req) {
  size_t n = req->params();
  for (size_t i = 0; i < n; i++) {
    const AsyncWebParameter* p = req->getParam(i);
    for (uint8_t w = 0; w < _count; w++)
      if (p->name() == _widgets[w]->key()) { _widgets[w]->applyCommand(p->value()); break; }
  }
  req->send(200, "application/json", _stateJson());
}

// Prometheus exposition format — scrape the device straight into Grafana / Home Assistant.
// Derives metrics from the state JSON (numbers as-is, true/false -> 1/0).
void RisalUI::_handleMetrics(AsyncWebServerRequest* req) {
  String j = _stateJson();
  String out;
  int i = 1;
  while (i < (int)j.length()) {
    int k1 = j.indexOf('"', i);
    if (k1 < 0) break;
    int k2 = j.indexOf('"', k1 + 1);
    if (k2 < 0) break;
    String key = j.substring(k1 + 1, k2);
    int c = j.indexOf(':', k2);
    int e = j.indexOf(',', c);
    if (e < 0) e = j.indexOf('}', c);
    if (e < 0) e = j.length();
    String val = j.substring(c + 1, e);
    if (val == "true") val = "1";
    else if (val == "false") val = "0";
    String name = "risaldash_";
    for (uint16_t z = 0; z < key.length(); z++) {
      char ch = key[z];
      if (ch >= 'A' && ch <= 'Z') ch += 32;
      name += ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) ? ch : '_';
    }
    out += "# TYPE " + name + " gauge\n" + name + " " + val + "\n";
    i = e + 1;
  }
  req->send(200, "text/plain; version=0.0.4; charset=utf-8", out);
}

// MCP manifest: every widget becomes a get_* (read) and/or set_* (write) tool. The
// risal-mcp-server bridge reads this and proxies tool calls to /api/state and /api/set.
void RisalUI::_handleManifest(AsyncWebServerRequest* req) {
  if (!_mcpToken) { req->send(404); return; }
  String tok = req->hasParam("token") ? req->getParam("token")->value() : "";
  if (tok != _mcpToken) { req->send(403, "application/json", "{\"error\":\"unauthorized\"}"); return; }

  String s = "{\"device\":\"";
  s += rwJsonEsc(_title);
  s += "\",\"tools\":[";
  bool first = true;
  for (uint8_t i = 0; i < _count; i++) {
    const char* id = _widgets[i]->typeId();
    if (strcmp(id, "group") == 0) continue;
    const char* key = _widgets[i]->key();

    const char* type = "number";
    if (!strcmp(id, "toggle") || !strcmp(id, "led")) type = "bool";
    else if (!strcmp(id, "text") || !strcmp(id, "label") || !strcmp(id, "log")) type = "string";
    else if (!strcmp(id, "select")) type = "enum";
    else if (!strcmp(id, "button")) type = "action";

    bool readable = _widgets[i]->hasState();
    bool writable = !strcmp(id, "toggle") || !strcmp(id, "slider") || !strcmp(id, "button") ||
                    !strcmp(id, "number") || !strcmp(id, "select") || !strcmp(id, "text");

    String base;
    for (const char* c = key; *c; c++) {
      char ch = *c;
      if (ch >= 'A' && ch <= 'Z') ch += 32;
      base += ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) ? ch : '_';
    }
    String ek = rwJsonEsc(String(key));
    if (readable) {
      if (!first) s += ',';
      first = false;
      s += "{\"name\":\"get_" + base + "\",\"op\":\"read\",\"key\":\"" + ek + "\",\"type\":\"" + type + "\"}";
    }
    if (writable) {
      if (!first) s += ',';
      first = false;
      s += "{\"name\":\"set_" + base + "\",\"op\":\"write\",\"key\":\"" + ek + "\",\"type\":\"" + type + "\"}";
    }
  }
  s += "]}";
  req->send(200, "application/json", s);
}

