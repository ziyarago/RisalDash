// Reference of every card-style RisalDash widget, grouped by purpose. (Structural widgets —
// dash.layout() pages and dash.tab() tabs — have their own examples.) Served over a plain
// access point: connect to "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Showcase");

// Display values
float cpu = 42, volts = 12.1f, temp = 24.3f, rssi = -52, hum = 55, pres = 1013;
int   ram = 60, pumpState = 0;            // pumpState: 0 ok · 1 warn · 2 bad
bool  linkUp = true;
// Control values
int   bright = 128, fanSpeed = 2, mode = 1, lightMode = 0;
bool  pump = false;
// Input values (controls bound to String state)
String devName = "greenhouse-01", wifiKey = "", boot = "08:30", svcDay = "2026-06-27", led = "#22d3ee";
String note = "All systems nominal — no action needed.";

void setup() {
  dash.timezone(180);

  dash.group("Display");
  dash.metric("CPU", &cpu, "%").decimals(0).zone(70, 90);
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C").span(2);
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.progress("RAM", &ram, "%");
  dash.badge("Pump state", &pumpState).labels("running", "idle", "fault");
  dash.led("Link", &linkUp);
  dash.label("Device", &devName);

  dash.group("Controls");
  dash.toggle("Pump", &pump, [](bool on) { pumpState = on ? 0 : 1; });
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; });
  dash.number("Fan speed", &fanSpeed, 0, 5, 1, [](int v) { (void)v; });
  dash.select("Mode", "Eco,Comfort,Boost", &mode, [](int i) { (void)i; });
  dash.radio("Light", "Off,Warm,Cool", &lightMode, [](int i) { (void)i; });
  dash.button("Reboot", "Restart", []() { ESP.restart(); });

  dash.group("Inputs");
  dash.text("Hostname", &devName, [](const String& v) { (void)v; });
  dash.password("Wi-Fi key", &wifiKey, [](const String& v) { (void)v; });
  dash.time("Boot time", &boot);
  dash.date("Service day", &svcDay);
  dash.color("LED color", &led);

  dash.group("Data");
  dash.table("Sensors")
      .row("Temperature", &temp, "C", 1)
      .row("Humidity", &hum, "%", 0)
      .row("Pressure", &pres, "hPa", 0);
  dash.ai("Assistant", &note);

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Simulate some movement so the dashboard looks alive.
  cpu = 30 + (millis() / 200 % 55);
  temp += 0.05f * ((millis() / 1000 % 2) ? 1 : -1);
  dash.update();
}
