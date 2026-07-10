// Motion security — a PIR / mmWave alarm dashboard with NO hardware. The sketch fakes motion triggers
// across a few zones, keeps an event log and a daily counter, and only "sounds" the siren while armed.
// Swap the fake trigger for a real PIR (HC-SR501) or LD2410 read and drive a buzzer pin — UI unchanged.
// Served over a plain access point — connect to "RisalDash-Motion" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Motion Security");

LogWidget* evlog = nullptr;
bool  motion = false, armed = true, siren = false;
int   events = 0;
float sinceLast = 0;  // seconds since the last trigger

void setup() {
  dash.timezone(180);
  dash.accent(2);  // red — security feel

  dash.layout("Status", RICON_MOTION);
  dash.led("Motion now", &motion);
  dash.badge("Events today", &events);
  dash.toggle("Armed", &armed, [](bool on) { (void)on; });
  dash.led("Siren", &siren);
  dash.stat("Since last", &sinceLast, "s");

  dash.layout("Log", RICON_CLOCK);
  evlog = &dash.log("Event log", 6);

  dash.beginAP("RisalDash-Motion", "12345678");
}

uint32_t last = 0, lastTrig = 0, nextGap = 8000;
void loop() {
  dash.update();
  uint32_t now = millis();
  if (now - last > 200) {
    last = now;
    sinceLast = (now - lastTrig) / 1000.0f;
    if (now - lastTrig > nextGap) {  // real: if (digitalRead(PIR_PIN)) { ... }
      lastTrig = now;
      nextGap = random(6000, 15000);
      motion = true;
      events++;
      siren = armed;
      if (evlog) evlog->print("Motion — zone " + String(random(1, 4)));
    } else if (now - lastTrig > 1500) {
      motion = false;
      siren = false;
    }
  }
}
