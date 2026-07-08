// ─────────────────────────────────────────────────────────────────────────────
//  TEMPLATE · Greenhouse controller
//  A ready-to-ship dashboard: climate + soil sensors, a light schedule, and pump
//  control. Copy it, bind your real pins/sensors where marked, and you're done.
//  Served over an access point — connect to "Greenhouse" and open http://192.168.4.1/
// ─────────────────────────────────────────────────────────────────────────────
#include <RisalUI.h>

RisalUI dash("Greenhouse");

// ── Bind these to real drivers (BME280, capacitive soil probe, relays). ──
float airT = 24.5, airH = 62, soil = 48, lux = 8200;
bool  pump = false, fan = false, lamp = true;
int   target = 24;
String note = "Soil a little dry — watering at 18:00.";

void setup() {
  dash.timezone(180);
  dash.accent(1);  // Blue — calm, "utility" feel; users can change it live in Settings

  // ── Climate ──
  dash.layout("Climate", RICON_THERMOMETER);
  dash.gauge("Air temp", &airT, 0, 50, "C");                 // ring
  dash.gauge("Humidity", &airH, 0, 100, "%").variant("semi");
  dash.chart("Trend", &airT, "C");
  dash.number("Target", &target, 16, 32, 1, [](int v) { (void)v; /* setpoint */ });
  dash.toggle("Fan", &fan, [](bool on) { (void)on; /* digitalWrite(FAN_PIN, on) */ });

  // ── Soil & light ──
  dash.layout("Soil", RICON_LEAF);
  dash.gauge("Moisture", &soil, 0, 100, "%").variant("bar");
  dash.stat("Light", &lux, "lux");
  dash.toggle("Grow lamp", &lamp, [](bool on) { (void)on; });
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP_PIN, on) */ });
  dash.ai("Advice", &note);  // surface a hint (or wire an AI agent to write here)

  dash.beginAP("Greenhouse", "12345678");
}

void loop() {
  // Replace with real reads: airT = bme.readTemperature(); soil = readSoil(); …
  dash.update();
}
