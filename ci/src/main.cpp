// Build smoke test — mirrors examples/AllWidgets (every widget type + a sensor preset).
#include <Arduino.h>
#include <RisalUI.h>

RisalUI dash("Showcase");

float cpu = 42, volts = 12.1, temp = 24.3, rssi = -52;
float hum = 55, pres = 1013;
int   ram = 60, bright = 128, setpoint = 24, mode = 0, fanLvl = 1;
bool  pump = false, linkUp = true;
int   pumpState = 0;
String statusMsg = "All systems nominal", devName = "greenhouse";
String wifiPass = "secret", alarmTime = "08:00", ledColor = "#22d3ee", camUrl = "/snapshot.jpg";
LogWidget* evlog = nullptr;

void setup() {
  dash.lang("ru");
  dash.group("System");
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
  dash.group("Settings");
  dash.number("Setpoint", &setpoint, 10, 35, 1);
  dash.select("Mode", "Auto,Manual,Off", &mode);
  dash.label("Status", &statusMsg);
  dash.text("Device name", &devName);
  evlog = &dash.log("Events", 5);
  evlog->print("system started");
  dash.password("Wi-Fi pass", &wifiPass);
  dash.time("Alarm", &alarmTime);
  dash.color("LED color", &ledColor);
  dash.image("Camera", &camUrl);
  dash.table("System").row("CPU", &cpu, "%").row("Voltage", &volts, "V", 1);
  dash.sensor("bme280", &temp, &hum, &pres);
  dash.separator("Layout");
  dash.ai("Assistant", &statusMsg).span(2);
  dash.chart("Temp wide", &temp, "C").span(2);
  dash.radio("Fan", "Low,Mid,High", &fanLvl);
  dash.tab("Tab A");
  dash.stat("A value", &cpu, "%").decimals(0);
  dash.tab("Tab B");
  dash.led("B flag", &linkUp);
  dash.enableMCP("risal_pat_demo");
  dash.enableOTA();
#ifdef RISAL_ENABLE_MQTT
  dash.mqtt("192.168.1.10", 1883, "greenhouse");
#endif
  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  cpu = 30 + (millis() / 200 % 55);
  dash.update();
}
