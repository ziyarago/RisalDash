// First-boot flow: on a fresh device dash.begin() raises an access point with a captive
// portal (a "RisalDash-Setup" Wi-Fi). Pick your network, enter the password — the creds are
// saved to NVS and the device reboots onto your Wi-Fi, then serves the dashboard there.
#include <RisalUI.h>

RisalUI dash("Greenhouse");

float temp = 24.3f, volts = 12.1f;
bool  pump = false;

void setup() {
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.toggle("Pump", &pump, [](bool on) { /* digitalWrite(PUMP_PIN, on); */ });

  dash.apName("Greenhouse-Setup");  // captive-portal AP name (optional)
  dash.begin();                     // saved Wi-Fi -> STA; otherwise -> setup portal
}

void loop() {
  // temp = readTemp(); volts = readVolts();
  dash.update();  // also services the captive-portal DNS while provisioning
}
