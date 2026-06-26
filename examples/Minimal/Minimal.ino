// RisalDash — a greenhouse dashboard in a few lines. Brings up an access point and a
// live dashboard at http://192.168.4.1/ ; values update over WebSocket, controls are live.
#include <RisalUI.h>

RisalUI dash("Greenhouse");

float temp = 24.3f, volts = 12.1f;
int   bright = 128;
bool  pump = false;

void setup() {
  dash.theme(RisalUI::DARK);
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.slider("Brightness", &bright, 0, 255, [](int v) { /* analogWrite(LED_PIN, v); */ });
  dash.toggle("Pump", &pump, [](bool on) { /* digitalWrite(PUMP_PIN, on); */ });
  dash.beginAP("Greenhouse", "12345678");
}

void loop() {
  // temp = readTemp(); volts = readVolts();
  dash.update();  // pushes changed values to the browser
}
