// shows the difference between certain controls:
// dash.toggle() - automatically updates the referenced value via a WebSocket, but introduces some latency;
// dash.button() - instantly toggles the value on click;
// dash.push() - works as a temporary toggle when held – client-side, no latency.
#include <RisalUI.h>

RisalUI dash("ToggleControls");

bool light = false;
// const uint8_t LED_PIN = 4;  //real ESP32-CAM flash led pin

void setup() {

  // pinMode(LED_PIN, OUTPUT); //real usage
  // digitalWrite(LED_PIN, light); //real usage

  dash.led("Status", &light).size(RSIZE_M);
  dash.toggle("Toggle", &light, [](bool on) { /* digitalWrite(LED_PIN, light); /* real usage */ }).size(RSIZE_M);
  dash.button("Button", "Click", []() { updateLED(!light); }).size(RSIZE_M);
  dash.push("Push", "Hold", [](bool on) { updateLED(!light); }).size(RSIZE_M);

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  dash.update();
}

void updateLED(bool state) {
  if (light == state) return;
  light = state;
  // digitalWrite(LED_PIN, light); //real usage
}
