// Live weather (Open-Meteo via RisalWeather). The HTTPS fetch blocks (~1-2 s), so it runs in a
// background FreeRTOS task and publishes into a shared struct under a mutex; sync() copies that
// into the public globals below and never blocks loop(). Bind wx::city to a text field and set
// wx::dirty in its callback — the task re-geocodes whenever the city changes.
#pragma once
#include <Arduino.h>
#include <RisalWeather.h>

namespace wx {

static const float LAT = 41.3111f, LON = 69.2797f;  // default location (Tashkent) until a city is entered

static String city = "Tashkent";   // bound to the web "City" field (persist it yourself if wanted)
static volatile bool dirty = false;  // set by the field's callback, consumed by the task

// Public read side — what the widgets / LCD display (filled by sync()).
static float temp = 0, wind = 0;
static int code = -1;
static bool valid = false;
static String desc = "...", place = "...";

#if defined(ESP32)
static RisalWeather engine;
static SemaphoreHandle_t mux;
static struct { float temp; int code; float wind; bool valid; String city; } shared;

// Its own task (pinned to core 0; loop() runs on core 1 on dual-core parts). All blocking work —
// geocoding a new city AND fetching weather — stays here, off loop(), published under the mutex.
static void task(void *) {
  engine.setLocation(LAT, LON, city);
  engine.geocode(city);  // resolve the (possibly persisted) city before the first fetch
  bool need = true;
  uint32_t last = 0;
  for (;;) {
    if (dirty) { dirty = false; engine.geocode(city); need = true; }  // resolve city -> lat/lon
    if (need || millis() - last > 600000UL) {                          // fetch on demand / every 10 min
      if (engine.poll()) {
        last = millis();
        need = false;
        xSemaphoreTake(mux, portMAX_DELAY);
        shared.temp = engine.temperature();
        shared.code = engine.code();
        shared.wind = engine.wind();
        shared.city = engine.city();
        shared.valid = true;
        xSemaphoreGive(mux);
        Serial.printf("[wx] %s %.1fC %s wind=%.0f heap=%u\n", engine.city().c_str(),
                      engine.temperature(), engine.description(), engine.wind(), ESP.getFreeHeap());
      } else {
        need = false;
        last = millis() - 570000UL;  // failed -> retry in ~30 s
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));  // wake often enough to react to a city change
  }
}

inline void begin() {  // start the fetcher task (call once Wi-Fi is up)
  mux = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(task, "weather", 12288, nullptr, 1, nullptr, 0);
}

// Copy the task's latest result into the public globals (quick, non-blocking, throttled inside).
inline void sync() {
  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();
  xSemaphoreTake(mux, portMAX_DELAY);
  temp = shared.temp; code = shared.code; wind = shared.wind; valid = shared.valid;
  if (valid) place = shared.city;
  xSemaphoreGive(mux);
  desc = valid ? RisalWeather::codeText(code) : String("...");
}
#else
inline void begin() {}
inline void sync() {}
#endif

}  // namespace wx
