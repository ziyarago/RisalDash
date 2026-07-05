// The smallest useful dashboard: raise an access point and serve a live page at
// http://192.168.4.1/ — values stream over WebSocket, controls call back instantly.
#include <RisalUI.h>

RisalUI dash("Greenhouse");

float temp = 24.3f, volts = 12.1f;
int   bright = 128;
bool  pump = false;

void setup() {
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Temperature", &temp, "C");
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; /* analogWrite(LED, v) */ });
  dash.toggle("Pump", &pump, [](bool on) { (void)on; /* digitalWrite(PUMP, on) */ });
  dash.beginAP("Greenhouse", "12345678");
}

void loop() {
  // temp = readTemp(); volts = readVolts();
  dash.update();  // push changed values to the browser
}
