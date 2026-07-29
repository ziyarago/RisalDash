// Pocket oscilloscope — measure an analog signal's Vpp / Vrms / frequency on the dashboard, with NO
// hardware: a synthetic sine + noise stands in for the ADC so the maths is watchable immediately.
// On a real board, burst-sample an ADC1 pin (GPIO32-39 keep working with Wi-Fi on; analogRead at
// ~10 kSps is plenty for audio-range signals; the I2S/DMA path reaches ~50 kSps+ when you need it).
// Protect the input: the pin tolerates 0-3.3 V ONLY — bias AC signals to mid-rail and clamp with
// diodes. Frequency comes from counting rising zero-crossings of the mean-removed signal.
// Served over a plain access point — connect to "RisalDash-Scope" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Scope");

const int N = 256;                 // burst length
float buf[N];
float vpp = 0, vrms = 0, freq = 0, level = 1.65f;
float sigHz = 50;                  // demo signal frequency (drifts to prove the counter tracks)
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);
  dash.accent(1);

  dash.layout("Scope", RICON_SIGNAL);
  dash.stat("Frequency", &freq, "Hz");
  dash.stat("Vpp", &vpp, "V");
  dash.stat("Vrms", &vrms, "V");
  dash.chart("Signal level", &level, "V");   // live mid-window sample, chart = the "screen"
  evlog = &dash.log("Notes", 4);

  dash.beginAP("RisalDash-Scope", "12345678");
  if (evlog) evlog->print("Demo signal: sine + noise");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 500) {   // a burst twice a second
    last = millis();
    sigHz += rfNoise(0.8f);      // let the demo frequency wander 45..55 Hz
    if (sigHz < 45) sigHz = 45;
    if (sigHz > 55) sigHz = 55;

    // --- acquire a burst. Real board: buf[i] = analogRead(PIN) * 3.3f / 4095.0f; ---
    const float SPS = 5000;      // demo sample rate (real: measure it with micros())
    for (int i = 0; i < N; i++)
      buf[i] = 1.65f + 1.2f * sinf(2 * PI * sigHz * i / SPS) + rfNoise(0.03f);

    // --- the scope maths: mean, Vpp, Vrms, zero-cross frequency ---
    float mean = 0, mn = 99, mx = -99;
    for (int i = 0; i < N; i++) { mean += buf[i]; if (buf[i] < mn) mn = buf[i]; if (buf[i] > mx) mx = buf[i]; }
    mean /= N;
    float sq = 0; int crossings = 0;
    for (int i = 1; i < N; i++) {
      float a = buf[i - 1] - mean, b = buf[i] - mean;
      sq += b * b;
      if (a < 0 && b >= 0) crossings++;              // rising zero-crossings
    }
    vpp  = mx - mn;
    vrms = sqrtf(sq / (N - 1));
    freq = crossings * SPS / N;                      // crossings per second
    level = buf[N / 2];
  }
}
