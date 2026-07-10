// Garage door controller — a WiFi opener dashboard with NO hardware. One button pulses a "relay" to
// start/stop the door; a modelled position travels 0→100 % while a reed switch reports open/closed.
// Swap in a real relay pulse and reed-switch read — the dashboard and its activity log are unchanged.
// Served over a plain access point — connect to "RisalDash-Garage" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Garage Door");

LogWidget* doorlog = nullptr;
bool  open = false, moving = false;
float position = 0;  // 0 = closed, 100 = fully open
int   dir = 0;       // -1 closing, +1 opening

void trigger() {
  // A real opener pulses the relay: digitalWrite(RELAY, HIGH); delay(300); digitalWrite(RELAY, LOW);
  if (moving) dir = -dir;               // a mid-travel press reverses the door
  else dir = open ? -1 : +1;
  moving = true;
  if (doorlog) doorlog->print(dir > 0 ? "Opening..." : "Closing...");
}

void setup() {
  dash.timezone(180);
  dash.accent(1);

  dash.layout("Door", RICON_HOME);
  dash.led("Open", &open);
  dash.stat("Position", &position, "%");
  dash.led("Moving", &moving);
  dash.button("Toggle door", "Open / Close", []() { trigger(); });

  dash.layout("Log", RICON_CLOCK);
  doorlog = &dash.log("Activity", 6);

  dash.beginAP("RisalDash-Garage", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 120) {
    last = millis();
    if (moving) {
      position += dir * 3.0f;
      if (position >= 100) { position = 100; moving = false; open = true;  if (doorlog) doorlog->print("Fully open"); }
      if (position <= 0)   { position = 0;   moving = false; open = false; if (doorlog) doorlog->print("Closed"); }
    }
  }
}
