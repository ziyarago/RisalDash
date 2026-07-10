// Aquarium controller — water temperature, pH, TDS and level, plus pump / filter / heater / light
// control and a feed button, all with NO hardware. Water temp/pH/TDS are driven by RisalFake signals;
// swap them for a DS18B20 + pH probe + TDS meter later and the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Aqua" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Aquarium");

RisalFake tempFake(25.4f, 0.6f, 0.05f, 0.5f);   // water temp ~25.4 °C
RisalFake phFake(7.2f, 0.4f, 0.02f, 0.4f);       // pH ~7.2
RisalFake tdsFake(280, 60, 4, 0.6f);             // dissolved solids ppm
float wtemp = 25.4f, ph = 7.2f, tds = 280, level = 82;
bool pump = true, filter = true, heater = false, light = true;

void feed() { /* pulse the feeder servo here */ }

void setup() {
  dash.timezone(180);

  dash.layout("Tank", RICON_WATER);
  dash.sensor("ds18b20", &wtemp);          // water temperature gauge
  dash.sensor("ph", &ph);                   // pH gauge 0..14
  dash.sensor("tds", &tds);                  // TDS ppm gauge
  dash.gauge("Water level", &level, 0, 100, "%");

  dash.layout("Control", RICON_POWER);
  dash.toggle("Pump", &pump);
  dash.toggle("Filter", &filter);
  dash.toggle("Heater", &heater);
  dash.toggle("Light", &light);
  dash.button("Feeder", "Feed now", feed);

  dash.beginAP("RisalDash-Aqua", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    wtemp = tempFake.read(); ph = phFake.read(); tds = tdsFake.read();
    if (heater) wtemp += 0.3f;                       // heater nudges the water warmer
    level = 78 + 6 * sinf(millis() * 0.00008f);       // slow evaporation/top-up
  }
}
