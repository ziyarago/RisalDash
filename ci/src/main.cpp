// Build smoke test — mirrors examples/AllWidgets (every widget type + a sensor preset).
#include <Arduino.h>
#include <RisalUI.h>

RisalUI dash("Showcase");

float cpu = 42, volts = 12.1, temp = 24.3, rssi = -52;
float hum = 55, pres = 1013;
int   ram = 60, bright = 128;
bool  pump = false, linkUp = true;
int   pumpState = 0;

void setup() {
  dash.metric("CPU", &cpu, "%").decimals(0).zone(70, 90);
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.progress("RAM", &ram, "%");
  dash.badge("Pump state", &pumpState).labels("running", "idle", "fault");
  dash.led("Link", &linkUp);
  dash.toggle("Pump", &pump, [](bool on) { pumpState = on ? 0 : 1; });
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; });
  dash.button("Reboot", "Restart", []() { ESP.restart(); });
  dash.sensor("bme280", &temp, &hum, &pres);
  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  cpu = 30 + (millis() / 200 % 55);
  dash.update();
}
