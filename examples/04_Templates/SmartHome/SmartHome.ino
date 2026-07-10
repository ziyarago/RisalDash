// Smart home — lights, motion, climate, door, curtains and power, plus scenes, with NO hardware.
// Climate is a RisalFakeEnv bundle; presence is faked; the rest are UI state you'd wire to relays,
// a mmWave sensor and an INA219. Swap the fakes for real drivers later; the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Home" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Smart Home");

RisalFakeEnv env;
RisalFake powerFake(240, 180, 12, 1.1f);   // household draw, W
float temp = 22, hum = 50, power = 240, presence = 1, motion = 40;
int curtains = 60, scene = 0;              // scene: Home/Away/Night/Movie
bool living = true, kitchen = false, door = false;

void setup() {
  dash.timezone(180);

  dash.layout("Living room", RICON_HOME);
  dash.toggle("Ceiling light", &living);
  dash.slider("Curtains", &curtains, 0, 100);
  dash.sensor("ld2410", &presence, &motion, &motion);  // presence · distance · motion (mmWave preset)
  dash.select("Scene", "Home,Away,Night,Movie", &scene, [](int i) { (void)i; /* apply scene */ });

  dash.layout("Climate", RICON_THERMOMETER);
  dash.sensor("aht20", &temp, &hum).chart();
  dash.metric("Power", &power, "W");

  dash.layout("Doors & rooms", RICON_MOTION);
  dash.toggle("Kitchen light", &kitchen);
  dash.led("Front door", &door);           // open/closed reed switch

  dash.beginAP("RisalDash-Home", "12345678");
  env.begin();
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    env.update();
    temp = env.temperature(); hum = env.humidity();
    power = powerFake.read(); if (power < 0) power = 0;
    presence = (env.hourOfDay() > 7 && env.hourOfDay() < 23) ? 1 : 0;   // "someone home" by hour
    motion = presence ? 40 + 40 * sinf(millis() * 0.001f) : 0;
  }
}
