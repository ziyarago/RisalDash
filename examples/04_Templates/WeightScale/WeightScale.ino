// Weight scale — a load-cell readout with NO hardware. dash.sensor("hx711", &kg) expands the HX711
// preset (a weight gauge in kg); RisalFakeWeight settles onto a target and swaps "items" every few
// seconds so the scale looks alive. A big stat shows grams. Swap the fake for a real HX711/NAU7802
// driver later — the dashboard is unchanged. Connect to "RisalDash-Scale" → http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Scale");

RisalFakeWeight scale;
float kg = 0, grams = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Scale", RICON_GAUGE);
  dash.sensor("hx711", &kg).size(RSIZE_L);   // big weight gauge (kg)
  dash.stat("Grams", &grams, "g").decimals(0);
  dash.chart("Trend", &kg, "kg");

  dash.beginAP("RisalDash-Scale", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 100) {   // 10 Hz
    last = millis();
    scale.update();
    kg = scale.weight();
    grams = scale.grams();
  }
}
