// Gas safety — smoke / CO alarm with no hardware. dash.sensor("mq2"/"mq7", …) expands the gas
// presets; RisalFakeGas keeps mostly-clean air with occasional spikes over the alarm threshold so
// you can build and test the alarm UI. An LED + badge trip when a channel goes over. Swap the fakes
// for real MQ-x drivers later — the dashboard is unchanged. Connect to "RisalDash-Gas" → 192.168.4.1
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Gas Safety");

RisalFakeGas smoke(600);   // MQ-2 smoke/LPG, alarm at 600 ppm
RisalFakeGas co(150);      // MQ-7 CO, alarm at 150 ppm
float smokePpm = 80, coPpm = 40;
bool smokeAlarm = false, coAlarm = false;

void setup() {
  dash.timezone(180);

  dash.layout("Air safety", RICON_FLASH);
  dash.sensor("mq2", &smokePpm).chart();   // smoke / LPG gauge + trend
  dash.led("Smoke alarm", &smokeAlarm);
  dash.sensor("mq7", &coPpm).chart();       // CO gauge + trend
  dash.led("CO alarm", &coAlarm);

  dash.beginAP("RisalDash-Gas", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 200) {
    last = millis();
    smoke.update(); co.update();
    smokePpm = smoke.ppm(); smokeAlarm = smoke.alarm();
    coPpm = co.ppm(); coAlarm = co.alarm();
  }
}
