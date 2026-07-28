// Live weather with no API key and no sensors — the current conditions for any city, straight from
// Open-Meteo. #include <RisalWeather.h> does the HTTPS fetch and turns a city name into coordinates.
// Those calls BLOCK (~1-2 s), so they run in a background FreeRTOS task and loop() never stalls; type
// a new city in the web "City" field and it re-geocodes on the fly.
//
// ESP32-only for the live data (needs TLS + real internet), so this example joins your Wi-Fi in STA
// mode instead of serving an offline AP. On ESP8266 it still compiles and serves the dashboard —
// RisalWeather is simply a no-op there.
#include <RisalUI.h>
#include <RisalWeather.h>

// --- your network: Open-Meteo needs real internet, so we connect in STA mode ---
const char *WIFI_SSID = "YOUR_WIFI";      // wrong/blank creds -> dash.begin() raises a setup portal
const char *WIFI_PASS = "YOUR_PASSWORD";

RisalUI dash("Weather");

// What the widgets show (refreshed once a second from the background task):
float wxTemp = 0;                 // °C
int wxCode = -1;                  // WMO weather code -> a sky icon
String city = "Tashkent";         // bound to the web "City" field
String wxPlace = "...", wxSky = "...";
volatile bool cityDirty = false;  // set by the field callback, consumed by the task

#if defined(ESP32)
// Everything that touches the internet lives in one task — geocode()/poll() block on HTTPS and must
// never run inside loop(). It publishes into `shared` under a mutex; syncWeather() copies that out.
RisalWeather engine;
SemaphoreHandle_t mux;
struct { float temp; int code; bool valid; String place; } shared = {0, -1, false, "..."};

void weatherTask(void *) {
  engine.geocode(city);                        // resolve the (possibly persisted) city first
  bool need = true;
  uint32_t last = 0;
  for (;;) {
    if (cityDirty) { cityDirty = false; engine.geocode(city); need = true; }  // city -> lat/lon
    if (need || millis() - last > 600000UL) {  // fetch on change, then every 10 minutes
      if (engine.poll()) {
        last = millis();
        need = false;
        xSemaphoreTake(mux, portMAX_DELAY);
        shared.temp = engine.temperature();
        shared.code = engine.code();
        shared.place = engine.city();
        shared.valid = true;
        xSemaphoreGive(mux);
      } else {
        need = false;
        last = millis() - 570000UL;            // fetch failed -> retry in ~30 s
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));           // wake often enough to notice a city change
  }
}

void startWeather() {
  mux = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(weatherTask, "weather", 12288, nullptr, 1, nullptr, 0);
}

void syncWeather() {                           // non-blocking copy of the task's latest result
  xSemaphoreTake(mux, portMAX_DELAY);
  wxTemp = shared.temp;
  wxCode = shared.code;
  bool ok = shared.valid;
  if (ok) wxPlace = shared.place;
  xSemaphoreGive(mux);
  wxSky = ok ? RisalWeather::codeText(wxCode) : String("...");
}
#else  // ESP8266: no TLS — RisalWeather is a no-op, the dashboard still builds and serves.
void startWeather() {}
void syncWeather() {}
#endif

void setup() {
  Serial.begin(115200);
  dash.timezone(180);                          // your UTC offset in minutes (used for timestamps)

  dash.layout("Weather", RICON_WATER);
  dash.weather("Sky", &wxCode);                // an icon that follows the WMO weather code
  dash.stat("Outside", &wxTemp, "C");          // current temperature
  dash.label("Place", &wxPlace);               // the resolved city name
  dash.label("Conditions", &wxSky);            // Clear / Cloudy / Rain / Snow / ...
  dash.text("City", &city, [](const String &v) { (void)v; cityDirty = true; });  // type a city -> re-geocode

  dash.begin(WIFI_SSID, WIFI_PASS);            // STA; on wrong creds a setup portal opens at 192.168.4.1
  startWeather();                              // launch the fetcher once Wi-Fi is up
}

void loop() {
  dash.update();
  static uint32_t last = 0;
  if (millis() - last > 1000) {                // copy the weather task's latest result once a second
    last = millis();
    syncWeather();
  }
}
