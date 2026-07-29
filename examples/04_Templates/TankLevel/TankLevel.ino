// Water tank level monitor & predictor — an ultrasonic (HC-SR04 / JSN-SR04T) tank gauge with NO
// hardware. The sketch models a tank that drains and auto-refills; the ultrasonic "distance" is
// derived from the level, just like the real echo. On top of the gauge it PREDICTS time-to-empty
// from the smoothed drain rate and flags a LEAK when water drops abnormally fast while the pump is
// off. Swap in a real ping and drive a pump relay — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Tank" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Water Tank");

float levelPct = 68, liters = 0, distCm = 0;
const float TANK_L = 1000, TANK_CM = 150;  // capacity and sensor-to-bottom height
bool  pump = false, low = false, leak = false;
float hoursLeft = 0;        // predicted time to empty at the current usage
float ratePctMin = 0;       // smoothed drain rate (% per minute) the prediction runs on
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);
  dash.accent(1);  // blue

  dash.layout("Tank", RICON_WATER);
  dash.gauge("Level", &levelPct, 0, 100, "%").variant("bar");
  dash.stat("Volume", &liters, "L");
  dash.stat("Empty in", &hoursLeft, "h");   // the predictor
  dash.chart("Trend", &levelPct, "%");

  dash.layout("Sensor", RICON_SIGNAL);
  dash.metric("Air gap", &distCm, "cm");  // what the ultrasonic actually measures
  dash.metric("Drain rate", &ratePctMin, "%/min");
  dash.led("Low level", &low);
  dash.led("Leak suspected", &leak);
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP_PIN, on) */ });
  evlog = &dash.log("Events", 5);

  dash.beginAP("RisalDash-Tank", "12345678");
}

uint32_t last = 0;
float prevPct = 68;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    // Auto-fill control: pump on below 25 %, off above 90 %.
    if (levelPct < 25) pump = true;
    else if (levelPct > 90) pump = false;
    // Demo model: steady household draw, an occasional "burst pipe" leak episode.
    float drain = 0.18f + (random(1000) < 3 ? 1.2f : 0);      // rare leak bursts
    levelPct += (pump ? 0.5f : -drain) + rfNoise(0.05f);
    if (levelPct > 100) levelPct = 100;
    if (levelPct < 0) levelPct = 0;

    // --- the predictor: smooth the drain rate, project time-to-empty ---
    float dPct = (prevPct - levelPct) * 240.0f;               // %/min at 250 ms per tick
    prevPct = levelPct;
    if (!pump) ratePctMin += 0.05f * (dPct - ratePctMin);     // EMA — calm, not jumpy
    hoursLeft = ratePctMin > 0.01f ? levelPct / ratePctMin / 60.0f : 99;
    if (hoursLeft > 99) hoursLeft = 99;

    // --- leak detection: dropping much faster than the household norm, pump off ---
    bool leakNow = !pump && ratePctMin > 0.9f;
    if (leakNow && !leak && evlog) evlog->print("LEAK suspected - close the valve");
    leak = leakNow;

    low = levelPct < 20;
    liters = TANK_L * levelPct / 100.0f;
    distCm = TANK_CM * (1.0f - levelPct / 100.0f);  // real: distCm = pingCm();
  }
}
