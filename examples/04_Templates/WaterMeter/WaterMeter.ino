// Water meter — flow rate + running total, no hardware. dash.sensor("flow", &rate, &total) expands
// the YF-S201 pulse-flow preset (L/min gauge + a litres total); RisalFakeFlow turns the tap on and
// off with a smooth ramp and accumulates the total. A pulse-frequency readout shows the raw signal.
// Swap the fake for a real flow-sensor pulse counter later. Connect to "RisalDash-Flow" → 192.168.4.1
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Water Meter");

RisalFakeFlow flow;
float rate = 0, total = 0, freq = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Flow", RICON_WATER);
  dash.sensor("flow", &rate, &total);      // L/min gauge + litres total (stat)
  dash.chart("Rate", &rate, "L/min");
  dash.metric("Pulse", &freq, "Hz");

  dash.beginAP("RisalDash-Flow", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 100) {
    last = millis();
    flow.update();
    rate = flow.rate(); total = flow.total(); freq = flow.frequency();
  }
}
