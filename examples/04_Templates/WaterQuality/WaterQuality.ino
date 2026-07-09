// Water quality — TDS / pH / turbidity / soil, no hardware. Each preset (tds, ph, turbidity, soil,
// water-level) maps to the right widget + unit + range; the values are driven by generic RisalFake
// signals (drift + wobble + noise) so the dashboard looks alive. Swap each for a real probe later —
// the dashboard is unchanged. Connect to "RisalDash-Water" → http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Water");

// Generic drifting fakes stand in for the probes (center, amplitude, noise, speed).
RisalFake tdsFake(320, 140, 6, 0.8f);
RisalFake ecFake(640, 260, 10, 0.8f);
RisalFake phFake(7.1f, 0.6f, 0.03f, 0.5f);
RisalFake turbFake(35, 30, 3, 1.2f);
RisalFakeEnv env;  // for a realistic soil-moisture day cycle

float tds = 320, ec = 640, ph = 7.1f, turb = 35, level = 60;
int soil = 60;

void setup() {
  dash.timezone(180);

  dash.layout("Water", RICON_WATER);
  dash.sensor("tds", &tds, &ec);          // TDS ppm gauge + EC metric
  dash.sensor("ph", &ph);                  // pH gauge 0..14
  dash.sensor("turbidity", &turb);          // NTU gauge

  dash.layout("Soil / Tank", RICON_LEAF);
  dash.progress("Soil moisture", &soil, "%");   // soil is an int here -> progress bar
  dash.sensor("water-level", &level);            // tank level % gauge

  dash.beginAP("RisalDash-Water", "12345678");
  env.begin();
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {   // 4 Hz
    last = millis();
    tds = tdsFake.read(); ec = ecFake.read(); ph = phFake.read(); turb = turbFake.read();
    env.update(); soil = env.soil();
    level = 40 + 30 * (0.5f + 0.5f * sinf(millis() * 0.0002f));
  }
}
