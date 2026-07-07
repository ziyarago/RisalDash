// ─────────────────────────────────────────────────────────────────────────────
//  TEMPLATE · Energy monitor
//  A ready-to-ship power dashboard: live voltage/current/power, today's energy and
//  cost, and a main breaker toggle. Copy it, bind a PZEM-004T or INA219, done.
//  Served over an access point — connect to "EnergyMonitor" and open http://192.168.4.1/
// ─────────────────────────────────────────────────────────────────────────────
#include <RisalUI.h>

RisalUI dash("Energy");

// ── Bind these to a real meter (PZEM-004T over UART, or INA219 over I2C). ──
float volts = 231, amps = 3.4, watts = 785, energyKwh = 4.82;
int   tariff = 12;      // cents per kWh
float cost = 0;
bool  mains = true;

void setup() {
  dash.timezone(180);
  dash.accent(3);  // Amber — an "energy" accent; changeable live in Settings

  // ── Live ──
  dash.layout("Live", RICON_FLASH);
  dash.gauge("Power", &watts, 0, 3000, "W");                 // ring
  dash.gauge("Voltage", &volts, 180, 260, "V").variant("semi");
  dash.gauge("Current", &amps, 0, 16, "A").variant("bar");
  dash.chart("Load", &watts, "W");

  // ── Billing ──
  dash.layout("Billing", RICON_HOME);
  dash.stat("Today", &energyKwh, "kWh").decimals(2);
  dash.stat("Cost", &cost, "$").decimals(2);
  dash.number("Tariff c/kWh", &tariff, 1, 100, 1, [](int v) { (void)v; });
  dash.toggle("Mains", &mains, [](bool on) { (void)on; /* trip the relay */ });

  dash.beginAP("EnergyMonitor", "12345678");
}

void loop() {
  // Replace with real reads: volts = pzem.voltage(); watts = pzem.power(); …
  cost = energyKwh * tariff / 100.0f;
  dash.update();
}
