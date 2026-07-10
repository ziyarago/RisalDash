// CNC machine monitor — spindle RPM, motor current, spindle temperature, runtime and an emergency
// stop, with NO hardware. RPM/current/temp are RisalFake signals; a status badge and an error log
// round it out. Swap the fakes for a tachometer + ACS712 + thermocouple later; UI is unchanged.
// Served over a plain access point — connect to "RisalDash-CNC" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("CNC");

RisalFake rpmFake(12000, 3500, 120, 1.4f);   // spindle RPM
RisalFake curFake(6.2f, 3.0f, 0.2f, 1.2f);    // motor current, A
RisalFake tempFake(48, 14, 0.6f, 0.7f);       // spindle temp, °C
float rpm = 12000, current = 6.2f, stemp = 48, runtime = 0;
int status = 0;                               // 0 running · 1 idle · 2 fault
bool estop = false;
LogWidget *errLog = nullptr;

void setup() {
  dash.timezone(180);

  dash.layout("Spindle", RICON_GAUGE);
  dash.sensor("rpm", &rpm);                   // RPM gauge
  dash.sensor("acs712", &current);             // current gauge
  dash.metric("Spindle temp", &stemp, "C");
  dash.badge("Status", &status).labels("running", "idle", "fault");

  dash.layout("Machine", RICON_POWER);
  dash.toggle("Emergency stop", &estop, [](bool on) { status = on ? 1 : 0; });
  dash.stat("Runtime", &runtime, "h").decimals(1);
  errLog = &dash.log("Errors", 5);

  dash.beginAP("RisalDash-CNC", "12345678");
}

uint32_t last = 0, tick = 0;
void loop() {
  dash.update();
  if (millis() - last > 200) {
    last = millis();
    if (estop) { rpm = 0; current = 0; }
    else { rpm = rpmFake.read(); current = curFake.read(); if (rpm < 0) rpm = 0; }
    stemp = tempFake.read();
    if (!estop) runtime += 0.2f / 3600.0f;                 // accumulate running hours
    if (stemp > 60 && millis() - tick > 8000) { tick = millis(); status = 2; if (errLog) errLog->print("WARN spindle temp high"); }
  }
}
