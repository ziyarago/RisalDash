// Power meter — a mains energy monitor with NO hardware. dash.sensor("pzem004t", …) expands the
// PZEM-004T preset (voltage / current / power / energy) into the right widgets; RisalFakePower drives
// them with a realistic household load that cycles as "appliances" switch on/off, and the kWh total
// accumulates. Swap the fake for a real PZEM/HLW8012/INA226 driver later — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Power" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Power Meter");

RisalFakePower meter;
float volts = 230, amps = 0, watts = 0, kwh = 0, pf = 0.9f, freq = 50;

void setup() {
  dash.timezone(180);

  dash.layout("Live", RICON_FLASH);
  dash.sensor("pzem004t", &volts, &amps, &watts, &kwh);  // V gauge · A metric · W chart · kWh stat
  dash.metric("Power factor", &pf);
  dash.metric("Frequency", &freq, "Hz");

  dash.beginAP("RisalDash-Power", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 50) {   // 20 Hz for smooth power swings
    last = millis();
    meter.update();
    volts = meter.voltage(); amps = meter.current(); watts = meter.power();
    kwh = meter.energy(); pf = meter.powerFactor(); freq = meter.frequency();
  }
}
