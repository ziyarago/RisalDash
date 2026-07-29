// Posture monitor — a wearable slouch coach with NO hardware: an MPU6050 on the upper back reports
// the forward-lean angle; past the threshold a grace timer runs, then the buzzer nags and the "bad
// posture" clock starts counting. The dashboard shows the live angle, a Good/Slouching verdict,
// today's slouch total and warning count, plus a threshold you can tune from the phone.
// RisalFakeIMU stands in for the sensor (its pitch wanders through good and bad zones), so the whole
// coaching loop is watchable with nothing strapped on. Real build: MPU6050 on I2C, buzzer on a GPIO,
// an 18650 + TP4056 — pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * 57.3f from the accelerometer.
// Served over a plain access point — connect to "RisalDash-Posture" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Posture");
RisalFakeIMU imu;

float lean = 0;                 // forward lean, degrees (0 = upright)
int   limit = 15;               // slouch threshold, tunable from the dashboard
int   state = 0;                // 0 good · 1 slouching  -> badge
float badMin = 0, warns = 0;    // today's slouch minutes + warning count
bool  buzz = false;
LogWidget* evlog = nullptr;
uint32_t overSince = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Posture", RICON_MOTION);
  dash.gauge("Forward lean", &lean, 0, 60, "deg");
  dash.badge("Verdict", &state).labels("Good", "Slouching", "-");
  dash.stat("Slouch today", &badMin, "min");
  dash.stat("Warnings", &warns, "");
  dash.chart("Lean trend", &lean, "deg");

  dash.layout("Settings", RICON_GAUGE);
  dash.number("Threshold", &limit, 5, 40, 1);   // tune the slouch angle live
  dash.led("Buzzer", &buzz);
  evlog = &dash.log("Events", 5);

  dash.beginAP("RisalDash-Posture", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    imu.update();
    lean = fabsf(imu.pitch());   // real: atan2f(ax, sqrtf(ay*ay+az*az)) * 57.3f;

    bool over = lean > limit;
    if (over && !overSince) overSince = millis();
    if (!over) { overSince = 0; buzz = false; }

    // 5 s grace, then the buzzer nags and the bad-posture clock counts.
    if (over && millis() - overSince > 5000) {
      if (!buzz) { warns += 1; if (evlog) evlog->print("Slouching - sit up!"); }
      buzz = true;               // real: tone(BUZZER_PIN, 2000, 100);
      badMin += 0.25f / 60.0f * 4;  // this tick (250 ms) in minutes
    }
    state = over ? 1 : 0;
  }
}
