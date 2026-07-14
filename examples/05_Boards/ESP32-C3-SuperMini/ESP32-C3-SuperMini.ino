// RisalDash on the ESP32-C3 SuperMini — the tiny, cheap RISC-V board (single core, Wi-Fi + BLE, native
// USB). A ready dashboard showing the internal chip temperature and the BOOT button, toggling the
// on-board LED, plus a fake-weather bundle for a lively demo. First boot with no saved Wi-Fi raises a
// captive-portal AP ("RisalDash-C3") for setup; then it serves the dashboard on your LAN.
//
// Board: esp32-c3-devkitm-1 (fits the SuperMini). Serial is over the native USB-CDC.
// Note: the C3 has NO capacitive touch pads (unlike the classic ESP32/S2/S3) — so this example uses the
// BOOT button instead. The SuperMini's blue LED on GPIO8 is active-LOW (LOW = on).
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("ESP32-C3 SuperMini");
RisalFakeWeather wx;

float chipTemp = 0, wind = 0, gust = 0;
bool  led = false;
bool  boot = false;   // BOOT button pressed

#define LED_PIN 8      // on-board blue LED (active-LOW on the SuperMini)
#define BOOT_PIN 9     // BOOT button (active-LOW, has a pull-up)

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // LED off (active-LOW)
  pinMode(BOOT_PIN, INPUT_PULLUP);

  dash.timezone(300);   // language defaults to English; Settings has the EN/RU/UZ/AR switcher

  dash.layout("ESP32-C3", RICON_HOME);
  dash.separator("Board");
  dash.metric("Chip temp", &chipTemp, "°C");
  dash.led("BOOT button", &boot);
  dash.toggle("LED", &led, [](bool on) { digitalWrite(LED_PIN, on ? LOW : HIGH); });

  dash.separator("Weather (fake)");
  dash.gauge("Wind", &wind, 0, 40, "km/h");
  dash.metric("Gust", &gust, "km/h");
  dash.chart("Wind · trend", &wind, "km/h");

  dash.apName("RisalDash-C3");
  dash.begin();
}

void loop() {
  dash.update();
  static uint32_t last = 0;
  if (millis() - last > 200) {
    last = millis();
    chipTemp = temperatureRead();          // internal temperature sensor
    boot = (digitalRead(BOOT_PIN) == LOW); // pressed = LOW
    wx.update();
    wind = wx.windSpeed();
    gust = wx.gust();
  }
}
