// Sensor templates. dash.sensor("id", &vars…) expands a preset into the right widgets (units,
// ranges, gauge/metric/chart already chosen for each quantity). Chain to tune the template:
//   .chart()  — add a live trend chart for every quantity (not just the ones that default to one)
//   .size(s)  — resize the readouts (RSIZE_S compact · RSIZE_M · RSIZE_L big)
// Served over a plain access point — connect to "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Sensors");

// Bound variables (feed these from real drivers; here they just drift so the demo looks alive).
float t = 24, h = 55, p = 1013;      // BME280: temperature · humidity · pressure
float v = 12.1, i = 0.8, w = 9.7;    // INA219: voltage · current · power
float pr = 1, dist = 180, mot = 40;  // LD2410 mmWave: presence · distance · motion

void setup() {
  // ── Compact readouts (the default template) ──
  dash.layout("Climate", RICON_THERMOMETER);
  dash.sensor("bme280", &t, &h, &p);

  // ── Same preset, but "with charts": each quantity also gets a live trend graph ──
  dash.layout("Power", RICON_FLASH);
  dash.sensor("ina219", &v, &i, &w).chart();

  // ── Big readouts + charts (a full-detail template) ──
  dash.layout("Presence", RICON_MOTION);
  dash.sensor("ld2410", &pr, &dist, &mot).chart().size(RSIZE_L);

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  t += 0.02f * ((millis() / 1500 % 2) ? 1 : -1);
  w = v * i;
  dash.update();  // stream changed values (and chart history) to the browser
}
