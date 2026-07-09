// Health monitor — heart rate + SpO2 with no hardware. dash.sensor("max30102", &hr, &spo2) expands
// the pulse-oximeter preset; RisalFakeHealth drives a resting heart rate with beat-to-beat
// variability and a healthy SpO2 that dips now and then. Swap the fake for a real MAX30102 driver
// later — the dashboard is unchanged. Connect to "RisalDash-Health" → http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Health");

RisalFakeHealth hm;
float hr = 72, spo2 = 98;

void setup() {
  dash.timezone(180);

  dash.layout("Vitals", RICON_HOME);
  dash.sensor("max30102", &hr, &spo2).chart().size(RSIZE_L);  // bpm gauge + SpO2, with trends

  dash.beginAP("RisalDash-Health", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 100) {
    last = millis();
    hm.update();
    hr = hm.heartRate();
    spo2 = hm.spo2();
  }
}
