// Smart gas cylinder monitor — weigh an LPG bottle with NO hardware and know exactly how much gas is
// left. A load cell under the cylinder + an HX711 give the TOTAL weight; subtract the empty-bottle
// TARE and you have the net gas in kg and a % of a full charge. A green LED means plenty, red + a
// buzzer nag when it's nearly empty. RisalFakeWeight drains the "bottle" so the whole gauge + alert
// loop runs on the bench; swap in a real HX711 read (bogde/HX711 library) and calibrate the tare.
// Served over a plain access point — connect to "RisalDash-Gas" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Gas Cylinder");

// LPG "3 kg" bottle model: ~5 kg empty (tare), 3 kg of gas full -> 8 kg total full.
const float TARE = 5.0, NET_FULL = 3.0;
float total = 8.0, net = 3.0, pct = 100;
int   state = 0;                 // 0 OK · 1 low · 2 EMPTY  -> badge
bool  ledOk = true, ledLow = false, buzz = false;
LogWidget* evlog = nullptr;
RisalFake tank(6.5f, 1.6f, 0.01f, 0.25f);   // fake TOTAL weight, drifting down over a "use" cycle

void setup() {
  dash.timezone(180);
  dash.accent(2);

  dash.layout("Gas", RICON_FLASH);
  dash.gauge("Gas left", &pct, 0, 100, "%").variant("bar").size(RSIZE_L);
  dash.stat("Net gas", &net, "kg").decimals(2);
  dash.stat("Total", &total, "kg").decimals(2);
  dash.badge("Status", &state).labels("OK", "Low", "EMPTY");
  evlog = &dash.log("Events", 5);

  dash.layout("Indicators", RICON_BULB);
  dash.led("Safe (green)", &ledOk);
  dash.led("Refill (red)", &ledLow);
  dash.led("Buzzer", &buzz);

  dash.beginAP("RisalDash-Gas", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    total = tank.read();                       // real: total = scale.get_units();  (kg, tared to 0 = nothing)
    net = total - TARE;                        // the gas alone
    if (net < 0) net = 0;
    pct = net / NET_FULL * 100.0f;
    if (pct > 100) pct = 100;

    int s = pct < 8 ? 2 : (pct < 20 ? 1 : 0);  // empty <8 %, low <20 %
    if (s && state == 0 && evlog) evlog->print("Gas low - order a refill");
    state = s;
    ledOk = s == 0;
    ledLow = s >= 1;
    buzz = s == 2;                             // real: tone(BUZZER_PIN, ...) when empty
  }
}
