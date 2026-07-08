// Tabbed dashboard. dash.tab("Name") splits the widgets that follow it into switchable
// panels (a tab bar appears). Cards declared *before* the first tab() stay pinned above the
// tabs. Served over a plain access point — connect to "RisalDash-Demo" and open
// http://192.168.4.1/   (For full multi-page navigation with a swipe-up sheet, see Layouts.)
#include <RisalUI.h>
#include <math.h>

RisalUI dash("Showcase");

float ram = 60, temp = 24.3f, hum = 55, volts = 12.1f, power = 42;
int   bright = 128;
bool  pump = false, light = true;

void setup() {
  dash.timezone(180);

  // Pinned above the tabs (always visible).
  dash.metric("RAM", &ram, "%").icon(RICON_GAUGE);

  dash.tab("Climate");
  dash.gauge ("Temperature", &temp, 0, 50, "C").icon(RICON_THERMOMETER);
  dash.metric("Humidity", &hum, "%").icon(RICON_WATER);
  dash.toggle("Grow light", &light);

  dash.tab("Power");
  dash.gauge("Voltage", &volts, 0, 14, "V").icon(RICON_FLASH);
  dash.chart("Power", &power, "W").icon(RICON_FLASH);
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP, on) */ });

  dash.tab("Settings");
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; /* analogWrite(LED, v) */ });

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  ram = 50 + (millis() / 300 % 40);
  temp = 24.0f + 2.0f * sinf(millis() * 0.0005f);
  dash.update();
}
