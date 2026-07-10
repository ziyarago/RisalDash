// 3D printer monitor — hotend / bed temperature, print progress, fan and print speed, plus an
// emergency stop and status, with NO hardware. Temperatures ramp to their targets; progress climbs
// over a fake print. Swap the fakes for thermistor reads + your firmware's M105/M27 later.
// Served over a plain access point — connect to "RisalDash-3DP" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("3D Printer");

float hotend = 25, bed = 25, progress = 0, speed = 100;
int status = 1;              // 0 printing · 1 idle · 2 error
int fan = 80;
bool estop = false;
float hotTarget = 210, bedTarget = 60;
int progPct = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Temperatures", RICON_THERMOMETER);
  dash.gauge("Hotend", &hotend, 0, 260, "C").icon(RICON_FLASH);
  dash.gauge("Bed", &bed, 0, 100, "C");
  dash.chart("Hotend trend", &hotend, "C");

  dash.layout("Print", RICON_MOTION);
  dash.progress("Progress", &progPct, "%");
  dash.badge("Status", &status).labels("printing", "idle", "error");
  dash.slider("Fan", &fan, 0, 100);
  dash.stat("Speed", &speed, "%").decimals(0);
  dash.toggle("Emergency stop", &estop, [](bool on) { if (on) { status = 1; hotTarget = bedTarget = 25; } });

  dash.beginAP("RisalDash-3DP", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    hotend += (hotTarget - hotend) * 0.05f + (float)random(-30, 30) / 100.0f;
    bed += (bedTarget - bed) * 0.06f;
    bool hot = hotend > hotTarget - 5 && bed > bedTarget - 3;
    if (hot && !estop) { status = 0; if (progress < 100) progress += 0.05f; }
    if (progress >= 100) { status = 1; }
    progPct = (int)progress;
    speed = 90 + 10 * sinf(millis() * 0.0003f);
  }
}
