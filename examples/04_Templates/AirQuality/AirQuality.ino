// Air-quality station — a full indoor monitor built with NO hardware. dash.sensor("sen55", …)
// expands the Sensirion SEN55 preset (PM1.0/2.5/4/10 + VOC + NOx + temperature + humidity — 8
// quantities) into the right widgets automatically; RisalFakeAir drives them with a realistic
// "room fills up then gets aired out" cycle. Swap the fake for a real SEN5x driver later and the
// dashboard code doesn't change. Served over a plain access point — connect to "RisalDash-Air"
// and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Air Quality");

RisalFakeAir air;
// Bound variables (the SEN55 preset maps each to its widget/unit/range).
float pm1 = 4, pm25 = 6, pm4 = 7, pm10 = 9, voc = 100, nox = 1, temp = 22, hum = 45;
float co2 = 450, hcho = 5;
int aqi = 0;  // 0 good · 1 fair · 2 poor — derived from PM2.5 for the status badge

void setup() {
  dash.timezone(180);

  // The whole SEN55 in one call — 8 readouts, auto-laid-out. .chart() adds a trend to each.
  dash.layout("SEN55", RICON_LEAF);
  dash.sensor("sen55", &pm1, &pm25, &pm4, &pm10, &voc, &nox, &temp, &hum);
  dash.badge("Air quality", &aqi).labels("good", "fair", "poor");

  // A second preset on its own page — SCD40-style CO2 + a formaldehyde readout (SFA30).
  dash.layout("CO2 / HCHO", RICON_THERMOMETER);
  dash.sensor("scd40", &co2, &temp, &hum).chart();
  dash.gauge("Formaldehyde", &hcho, 0, 1000, "ppb");

  dash.beginAP("RisalDash-Air", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {   // 4 Hz
    last = millis();
    air.update();
    pm1 = air.pm1(); pm25 = air.pm25(); pm4 = air.pm4(); pm10 = air.pm10();
    voc = air.vocIndex(); nox = air.noxIndex();
    temp = air.temperature(); hum = air.humidity();
    co2 = air.co2(); hcho = air.formaldehyde();
    aqi = pm25 < 15 ? 0 : pm25 < 35 ? 1 : 2;   // WHO-ish PM2.5 thresholds
  }
}
