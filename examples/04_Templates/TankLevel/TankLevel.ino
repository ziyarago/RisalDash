// Water tank level monitor — an ultrasonic (HC-SR04 / JSN-SR04T) tank gauge with NO hardware. The sketch
// models a tank that drains and auto-refills; the ultrasonic "distance" is derived from the level, just
// like the real echo. Swap in a real ping and drive a pump relay — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Tank" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Water Tank");

float levelPct = 68, liters = 0, distCm = 0;
const float TANK_L = 1000, TANK_CM = 150;  // capacity and sensor-to-bottom height
bool  pump = false, low = false;

void setup() {
  dash.timezone(180);
  dash.accent(1);  // blue

  dash.layout("Tank", RICON_WATER);
  dash.gauge("Level", &levelPct, 0, 100, "%").variant("bar");
  dash.stat("Volume", &liters, "L");
  dash.chart("Trend", &levelPct, "%");

  dash.layout("Sensor", RICON_SIGNAL);
  dash.metric("Air gap", &distCm, "cm");  // what the ultrasonic actually measures
  dash.led("Low level", &low);
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP_PIN, on) */ });

  dash.beginAP("RisalDash-Tank", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    // Auto-fill control: pump on below 25 %, off above 90 %.
    if (levelPct < 25) pump = true;
    else if (levelPct > 90) pump = false;
    levelPct += (pump ? 0.5f : -0.18f) + rfNoise(0.05f);
    if (levelPct > 100) levelPct = 100;
    if (levelPct < 0) levelPct = 0;
    low = levelPct < 20;
    liters = TANK_L * levelPct / 100.0f;
    distCm = TANK_CM * (1.0f - levelPct / 100.0f);  // real: distCm = pingCm();
  }
}
