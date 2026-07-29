// Water quality — TDS / pH / turbidity / temperature with NO hardware, plus the two things a real
// installation needs: TEMPERATURE COMPENSATION (a TDS probe reads ~2 %/°C off; normalise to 25 °C
// before trusting it) and AUTO-ACTUATION (a fresh-water pump kicks in when quality breaches, plus a
// buzzer/LED alert). Each preset (tds, ph, turbidity) maps to the right widget + unit + range; the
// values are driven by RisalFake signals so the dashboard looks alive. Swap each for a real probe
// later — the dashboard is unchanged. Connect to "RisalDash-Water" → http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Water");

// Generic drifting fakes stand in for the probes (center, amplitude, noise, speed).
RisalFake tdsFake(320, 140, 6, 0.8f);
RisalFake phFake(7.1f, 0.6f, 0.03f, 0.5f);
RisalFake turbFake(35, 30, 3, 1.2f);
RisalFake tempFake(26.5f, 2.5f, 0.05f, 0.4f);   // DS18B20 water temperature

float tdsRaw = 320, tds = 320, ec = 640, ph = 7.1f, turb = 35, waterT = 26.5f;
int   quality = 0;              // 0 good · 1 fair · 2 poor  -> badge
bool  pump = false, alarmOut = false;
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);

  dash.layout("Water", RICON_WATER);
  dash.sensor("tds", &tds, &ec);           // TDS ppm gauge + EC metric (compensated)
  dash.sensor("ph", &ph);                  // pH gauge 0..14
  dash.sensor("turbidity", &turb);         // NTU gauge
  dash.sensor("ds18b20", &waterT);         // water temperature (drives the compensation)
  dash.badge("Quality", &quality).labels("Good", "Fair", "POOR");

  dash.layout("Control", RICON_GAUGE);
  dash.metric("TDS raw", &tdsRaw, "ppm");  // what the probe reports before compensation
  dash.led("Alert", &alarmOut);            // buzzer + red LED in a real build
  dash.toggle("Fresh-water pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP_PIN, on) */ });
  evlog = &dash.log("Events", 5);

  dash.beginAP("RisalDash-Water", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {   // 4 Hz
    last = millis();
    tdsRaw = tdsFake.read(); ph = phFake.read(); turb = turbFake.read(); waterT = tempFake.read();

    // Temperature compensation: a TDS/EC probe drifts ~2 %/°C — normalise to 25 °C.
    tds = tdsRaw / (1.0f + 0.02f * (waterT - 25.0f));
    ec  = tds * 2.0f;

    // Quality score from all three probes (drinking-ish thresholds; tune per use).
    int score = 0;
    if (tds > 500 || ph < 6.5f || ph > 8.5f || turb > 50) score = 2;
    else if (tds > 300 || ph < 6.8f || ph > 8.0f || turb > 25) score = 1;
    if (score == 2 && quality != 2 && evlog) evlog->print("POOR water - pump + check filter");
    quality = score;

    // Auto-actuation: poor quality kicks the fresh-water pump in until it recovers.
    if (score == 2) pump = true;
    else if (score == 0) pump = false;
    alarmOut = score == 2;
  }
}
