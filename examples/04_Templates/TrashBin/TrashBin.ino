// Smart trash bin — a fill-level monitor for one or many bins with NO hardware. An ultrasonic sensor
// (HC-SR04) on the lid measures the air gap down to the garbage; fill % is derived from it exactly
// like the tank gauge, but inverted: a SHORT echo means a FULL bin. Three simulated bins fill at
// different rates and get "collected" when full — swap each fill model for a real pingCm() and the
// dashboard (and its MCP tools) is unchanged. A smart-city block, one ESP per bin or one per site.
// Served over a plain access point — connect to "RisalDash-Trash" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Trash Bins");

const float BIN_CM = 60;                    // sensor-to-bottom depth of a standard bin
float fill[3] = {35, 62, 88};               // fill % per bin (what the site manager cares about)
float gapCm = 0;                            // the raw echo of bin A — what the sensor really reads
int   state[3] = {0, 0, 0};                 // 0 ok · 1 almost full · 2 full  -> badge colours
bool  alarmOut = false;                     // LED + buzzer output when any bin is full
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);
  dash.accent(2);  // green — it's a city-services build

  dash.layout("Bins", RICON_HOME);
  dash.gauge("Bin A - Park", &fill[0], 0, 100, "%").variant("bar");
  dash.gauge("Bin B - Market", &fill[1], 0, 100, "%").variant("bar");
  dash.gauge("Bin C - School", &fill[2], 0, 100, "%").variant("bar");
  dash.badge("A status", &state[0]).labels("OK", "Almost", "FULL");
  dash.badge("B status", &state[1]).labels("OK", "Almost", "FULL");
  dash.badge("C status", &state[2]).labels("OK", "Almost", "FULL");
  evlog = &dash.log("Events", 6);

  dash.layout("Sensor", RICON_SIGNAL);
  dash.metric("Air gap A", &gapCm, "cm");   // the actual ultrasonic reading
  dash.led("Pickup needed", &alarmOut);     // real build: buzzer + red LED on the lid
  dash.button("Collected", "Mark bin A empty", []() {
    fill[0] = 2;                            // the truck emptied it — reset the model
    if (evlog) evlog->print("Bin A collected");
  });

  dash.beginAP("RisalDash-Trash", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 400) {
    last = millis();
    alarmOut = false;
    for (int i = 0; i < 3; i++) {
      fill[i] += 0.04f * (i + 1) + rfNoise(0.06f);          // each site fills at its own pace
      if (fill[i] >= 100) { fill[i] = 2; if (evlog) evlog->print(String("Bin ") + char('A' + i) + " collected"); }
      int s = fill[i] > 90 ? 2 : (fill[i] > 75 ? 1 : 0);
      if (s == 2 && state[i] != 2 && evlog) evlog->print(String("Bin ") + char('A' + i) + " FULL - dispatch pickup");
      state[i] = s;
      if (s == 2) alarmOut = true;
    }
    gapCm = BIN_CM * (1.0f - fill[0] / 100.0f);             // real: gapCm = pingCm(); fill = 100*(1-gap/BIN_CM)
  }
}
