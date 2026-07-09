// Battery / BMS monitor — state-of-charge, pack voltage, current and temperature, no hardware.
// dash.sensor("bms", …) expands the battery-management preset; RisalFakeBattery discharges under
// load then recharges, with the pack voltage following a Li-ion SoC curve and current flipping sign.
// Swap the fake for a real Daly/JK BMS driver later — the dashboard is unchanged.
// Connect to "RisalDash-BMS" → http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Battery BMS");

RisalFakeBattery bat;
float soc = 72, packV = 49, amps = -10, temp = 27, cellV = 3.8f;
bool charging = false;

void setup() {
  dash.timezone(180);

  dash.layout("Battery", RICON_POWER);
  dash.sensor("bms", &soc, &packV, &amps, &temp).chart();  // SoC gauge · V · A chart · °C
  dash.stat("Cell", &cellV, "V").decimals(2);
  dash.led("Charging", &charging);

  dash.beginAP("RisalDash-BMS", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 100) {
    last = millis();
    bat.update();
    soc = bat.soc(); packV = bat.packVoltage(); amps = bat.current();
    temp = bat.temperature(); cellV = bat.cellVoltage(); charging = bat.charging();
  }
}
