// Live weather from Open-Meteo (no API key), fetched over HTTPS. The request BLOCKS (TLS handshake +
// round-trip take ~1-2 s), so poll() must run in a background FreeRTOS task — never in loop(). The
// sketch reads the cached getters, which never block. ESP32-only (TLS + task); on other cores it's a
// no-op. See the .ino for the task wiring — this is the concrete "move blocking I/O off the loop" case.
#pragma once
#include <Arduino.h>
#include <math.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif

class Weather {
 public:
  void begin(float lat, float lon) { _lat = lat; _lon = lon; }

  // Blocking fetch — call from a task, NOT loop(). Returns true on success and updates the cache.
  bool poll() {
#if defined(ESP32)
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFiClientSecure client;
    client.setInsecure();  // skip cert validation — fine for a public weather read
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(_lat, 4) +
                 "&longitude=" + String(_lon, 4) + "&current=temperature_2m,weather_code,wind_speed_10m";
    if (!http.begin(client, url)) return false;
    int status = http.GET();
    if (status != 200) { http.end(); return false; }
    String body = http.getString();
    http.end();
    float t = jsonNum(body, "temperature_2m");
    if (isnan(t)) return false;
    _temp = t;
    _code = (int)jsonNum(body, "weather_code");
    _wind = jsonNum(body, "wind_speed_10m");
    _valid = true;
    return true;
#else
    return false;
#endif
  }

  float temperature() const { return _temp; }  // °C
  int code() const { return _code; }            // WMO weather code
  float wind() const { return _wind; }          // km/h
  bool valid() const { return _valid; }
  const char* description() const { return codeText(_code); }

  // WMO weather code -> short label (open-meteo.com/en/docs).
  static const char* codeText(int c) {
    if (c == 0) return "Clear";
    if (c <= 3) return "Cloudy";
    if (c <= 48) return "Fog";
    if (c <= 67) return "Rain";
    if (c <= 77) return "Snow";
    if (c <= 82) return "Showers";
    if (c <= 86) return "Snow";
    return "Storm";
  }

 private:
  // Pull the number right after "key": inside the "current" object (the response also has a
  // "current_units" object with the same keys but string values like "°C", which must be skipped).
  static float jsonNum(const String &s, const char *key) {
    int base = s.indexOf("\"current\":{");  // the values object, after "current_units"
    int i = s.indexOf(String("\"") + key + "\":", base < 0 ? 0 : base);
    if (i < 0) return NAN;
    return s.substring(i + (int)strlen(key) + 3).toFloat();
  }
  float _lat = 0, _lon = 0, _temp = NAN, _wind = 0;
  int _code = 0;
  bool _valid = false;
};
