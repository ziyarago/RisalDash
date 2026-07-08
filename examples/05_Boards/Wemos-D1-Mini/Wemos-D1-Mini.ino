// Wemos/LOLIN D1 mini (ESP8266) board showcase. The board's REAL hardware — onboard LED
// (toggle + adjustable blink), the A0 analog input, live heap (watch the chunked streaming
// keep a 40+ KB page flowing through ~40 KB of free RAM), Wi-Fi RSSI and uptime — plus a
// "Demo sensors" group from <RisalFake.h>, so a bare board still shows a lively greenhouse.
// RisalDash's streaming renderer was built for exactly this board — 80 KB RAM total.
//
// First boot: dash.begin() raises the "D1Mini-Setup" portal; pick your Wi-Fi and the board
// reboots onto it. After that it serves the dashboard at the IP printed on the serial monitor.
#include <RisalUI.h>
#include <RisalFake.h>  // day/night greenhouse emulator for the demo group (no wiring needed)
#include <ESP8266WiFi.h>

RisalUI dash("D1 Mini");

// Onboard LED: GPIO2, ACTIVE LOW (LOW = lit) on the D1 mini.
bool led = false;      // desired state (toggle)
int blinkMs = 0;       // 0 = steady, otherwise blink half-period in ms (slider)
// Live board readings.
float a0 = 0;          // A0, raw 0..1023 (wire a pot/LDR/soil probe — or leave floating)
float heapKb = 40, rssi = -60, uptimeS = 0;
float cpuMhz = 80, flashMb = 4;
// Demo sensors — fake but realistic (swap for real drivers without touching the dashboard).
RisalFakeEnv env;
float temp = 22, hum = 55;
int soil = 60;

void applyLed(bool on) { digitalWrite(LED_BUILTIN, on ? LOW : HIGH); }  // active low

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  applyLed(false);

  // Two swipe pages: the real board vs the fake demo — the top nav strip comes with layout().
  dash.layout("Board", RICON_POWER);
  dash.group("Onboard LED");
  dash.toggle("LED", &led, [](bool on) { applyLed(on); });
  dash.slider("Blink ms", &blinkMs, 0, 1000);  // 0 = steady; else the LED blinks while ON

  dash.group("Inputs");
  dash.gauge("Analog A0", &a0, 0, 1023).variant("bar");  // raw ADC — pot, LDR, soil probe…

  dash.group("Board health");
  dash.chart("Free heap", &heapKb, "KB");  // the page itself costs heap — watch it breathe
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.stat("Uptime", &uptimeS, "s").decimals(0);
  dash.table("Chip")
      .row("CPU", &cpuMhz, "MHz", 0)
      .row("Flash", &flashMb, "MB", 0);

  dash.layout("Demo", RICON_LEAF);  // RisalFakeEnv: a greenhouse day in ~3 minutes, no wiring
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil", &soil, "%");

  env.begin();
  dash.apName("D1Mini-Setup");
  dash.begin();  // saved Wi-Fi -> STA; otherwise the captive setup portal
  Serial.printf("[net] IP: %s\n", WiFi.localIP().toString().c_str());

  cpuMhz = ESP.getCpuFreqMHz();
  flashMb = ESP.getFlashChipRealSize() / 1048576.0f;
}

uint32_t lastSample = 0, lastBlink = 0;
bool blinkPhase = false;

void loop() {
  dash.update();

  if (millis() - lastSample > 500) {  // sample the real board twice a second
    lastSample = millis();
    env.update();
    temp = env.temperature();
    hum = env.humidity();
    soil = env.soil();
    a0 = analogRead(A0);
    heapKb = ESP.getFreeHeap() / 1024.0f;
    rssi = WiFi.RSSI();
    uptimeS = millis() / 1000.0f;
  }

  // Blink pattern: when the LED is ON and Blink ms > 0, square-wave it in hardware.
  if (led && blinkMs > 0) {
    if (millis() - lastBlink > (uint32_t)blinkMs) {
      lastBlink = millis();
      blinkPhase = !blinkPhase;
      applyLed(blinkPhase);
    }
  }
  delay(2);
}
