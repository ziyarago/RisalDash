// Multi-page dashboard. dash.layout("Name", RICON_*) starts a new page; the widgets declared
// after it belong to that page. A swipe up from the bottom edge (or a tap on the page handle)
// opens a sheet of icon tiles to switch pages. Language, theme and accent live in the appbar
// Settings gear. Served over a plain access point — connect to "RisalDash-Demo" and open
// http://192.168.4.1/
#include <RisalUI.h>
#include <math.h>

RisalUI dash("RisalDash");

float cpu = 42, temp = 24.4f, hum = 55, volts = 12.1f, watts = 1800;
int   ram = 61, target = 24;
bool  heater = true, pump = false, fan = false;

void setup() {
  dash.timezone(180);  // status-bar clock at +03:00 (also the portal's default)
  dash.accent(0);      // default accent (Aqua); the user can change it live in Settings

  // ── Page 1: Overview ──
  dash.layout("Overview", RICON_HOME);
  dash.group("System");
  dash.metric("CPU", &cpu, "%").zone(70, 90);
  dash.progress("RAM", &ram, "%");
  dash.gauge("Temperature", &temp, 0, 50, "C").icon(RICON_THERMOMETER);

  // ── Page 2: Climate ──
  dash.layout("Climate", RICON_THERMOMETER);
  dash.chart("Humidity", &hum, "%");
  dash.slider("Target", &target, 16, 30, [](int v) { (void)v; /* setSetpoint(v) */ });
  dash.toggle("Heater", &heater, [](bool on) { (void)on; /* digitalWrite(HEAT, on) */ });
  dash.toggle("Fan", &fan, [](bool on) { (void)on; });

  // ── Page 3: Power ──
  dash.layout("Power", RICON_FLASH);
  dash.gauge("Voltage", &volts, 0, 14, "V").icon(RICON_FLASH);
  dash.chart("Power", &watts, "W");
  dash.toggle("Pump", &pump, [](bool on) { (void)on; });

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Simulate movement so the dashboard looks alive.
  cpu = 30 + (millis() / 200 % 55);
  temp = 24.0f + 2.0f * sinf(millis() * 0.0005f);
  dash.update();  // push changed values to the browser over WebSocket
}
