// Showcase of every RisalDash widget type, served over a plain access point.
// Connect to the "RisalDash-Demo" Wi-Fi and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Showcase");

float cpu = 42, volts = 12.1, temp = 24.3, rssi = -52;
float hum = 55, pres = 1013;
int   ram = 60, bright = 128;
bool  pump = false, linkUp = true;
int   pumpState = 0;  // 0 ok, 1 warn, 2 bad

void setup() {
  // Display
  dash.metric("CPU", &cpu, "%").decimals(0).zone(70, 90);
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.progress("RAM", &ram, "%");
  dash.badge("Pump state", &pumpState).labels("running", "idle", "fault");
  dash.led("Link", &linkUp);

  // Control
  dash.toggle("Pump", &pump, [](bool on) { pumpState = on ? 0 : 1; });
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; });
  dash.button("Reboot", "Restart", []() { ESP.restart(); });

  // Sensor preset (expands to several widgets automatically)
  dash.sensor("bme280", &temp, &hum, &pres);

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Simulate some movement so the dashboard looks alive.
  cpu = 30 + (millis() / 200 % 55);
  temp += 0.05f * ((millis() / 1000 % 2) ? 1 : -1);
  dash.update();
}
