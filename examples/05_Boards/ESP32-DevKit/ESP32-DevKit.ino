// RisalDash on a classic ESP32 DevKit (WROOM/WROVER, dual-core, Wi-Fi + BT). A ready dashboard showing
// the board's own signals — a capacitive touch pad and the internal chip temperature — plus a fake
// weather bundle for a lively demo and an on-board LED toggle. First boot with no saved Wi-Fi raises a
// captive-portal AP ("RisalDash-ESP32") for setup; then it serves the dashboard on your LAN.
//
// Board: esp32dev (any ESP32 WROOM/WROVER dev board). Serial over the CP2102/CH340 USB.
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("ESP32 DevKit");
RisalFakeWeather wx;

float touchVal = 0, chipTemp = 0, wind = 0, gust = 0;
bool  led = false;

#define LED_PIN 2      // on-board LED on most ESP32 dev boards
#define TOUCH_PIN T0   // GPIO4 capacitive touch pad

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // known state at boot — GPIO2 can latch its level across ESP.restart()
  dash.timezone(300);   // language defaults to English; Settings has the EN/RU/UZ/AR switcher

  dash.layout("ESP32", RICON_HOME);
  dash.separator("Board");
  dash.metric("Touch T0", &touchVal);
  dash.metric("Chip temp", &chipTemp, "°C");
  dash.toggle("LED", &led, [](bool on) { digitalWrite(LED_PIN, on); });

  dash.separator("Weather (fake)");
  dash.gauge("Wind", &wind, 0, 40, "km/h");
  dash.metric("Gust", &gust, "km/h");
  dash.chart("Wind · trend", &wind, "km/h");

  dash.apName("RisalDash-ESP32");
  dash.begin();
}

void loop() {
  dash.update();
  static uint32_t last = 0;
  if (millis() - last > 200) {
    last = millis();
    touchVal = touchRead(TOUCH_PIN);   // capacitive touch (classic ESP32)
    chipTemp = temperatureRead();       // internal temperature sensor
    wx.update();
    wind = wx.windSpeed();
    gust = wx.gust();
  }
}
