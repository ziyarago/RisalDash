// First-boot provisioning. On a fresh device dash.begin() raises an access point with a
// captive portal: pick your Wi-Fi (networks show signal strength + a lock), enter the
// password, choose a timezone. The credentials and timezone are saved to NVS and the device
// reboots onto your network, then serves the dashboard there. Re-running begin() reuses them.
#include <RisalUI.h>

RisalUI dash("Greenhouse");

float temp = 24.3f, volts = 12.1f;
bool  pump = false;

void setup() {
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP_PIN, on) */ });

  dash.apName("Greenhouse-Setup");  // captive-portal AP name (optional)
  dash.timezone(180);               // default timezone offered in the portal (+03:00)
  dash.begin();                     // saved Wi-Fi -> STA; otherwise -> setup portal
}

void loop() {
  // temp = readTemp(); volts = readVolts();
  dash.update();  // also services the captive-portal DNS while provisioning
}
