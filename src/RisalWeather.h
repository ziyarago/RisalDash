// RisalWeather.h — live weather from Open-Meteo (no API key). Opt-in library helper: #include it only
// where you want weather. Both geocode() and poll() BLOCK on an HTTPS request, so call them from a
// background FreeRTOS task — never from loop(). ESP32-only for now (TLS); a graceful no-op elsewhere.
// See the ESP32-C6 example for the task wiring + a "City" field that re-geocodes when it changes.
#pragma once
#include <Arduino.h>
#include <math.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif

class RisalWeather {
 public:
  void setLocation(float lat, float lon, const String &name = "") {
    _lat = lat;
    _lon = lon;
    if (name.length()) _city = name;
  }

  // Blocking: resolve a place name to coordinates via Open-Meteo geocoding, updating the location and
  // city name on success. Returns false if the place isn't found (location left unchanged).
  bool geocode(const String &place) {
#if defined(ESP32)
    String body;
    if (!httpGet("https://geocoding-api.open-meteo.com/v1/search?count=1&name=" + urlEncode(place), body)) return false;
    float la = numAfter(body, "\"results\"", "latitude");
    float lo = numAfter(body, "\"results\"", "longitude");
    if (isnan(la) || isnan(lo)) return false;
    _lat = la;
    _lon = lo;
    _city = strValue(body, "name", place);
    return true;
#else
    (void)place;
    return false;
#endif
  }

  // Blocking: fetch the current weather for the stored location. Call from a task.
  bool poll() {
#if defined(ESP32)
    String body;
    if (!httpGet("https://api.open-meteo.com/v1/forecast?current=temperature_2m,weather_code,wind_speed_10m&latitude=" +
                     String(_lat, 4) + "&longitude=" + String(_lon, 4),
                 body))
      return false;
    float t = numAfter(body, "\"current\":{", "temperature_2m");  // skip the "current_units" object
    if (isnan(t)) return false;
    _temp = t;
    _code = (int)numAfter(body, "\"current\":{", "weather_code");
    _wind = numAfter(body, "\"current\":{", "wind_speed_10m");
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
  const String &city() const { return _city; }
  const char *description() const { return codeText(_code); }

  // WMO weather code -> short label (open-meteo.com/en/docs).
  static const char *codeText(int c) {
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
#if defined(ESP32)
  static bool httpGet(const String &url, String &out) {
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFiClientSecure client;
    client.setInsecure();  // skip cert validation — fine for a public read
    HTTPClient http;
    if (!http.begin(client, url)) return false;
    int st = http.GET();
    if (st == 200) out = http.getString();
    http.end();
    return st == 200;
  }
  static String urlEncode(const String &s) {
    String o;
    char b[4];
    for (size_t i = 0; i < s.length(); i++) {
      char c = s[i];
      if (isalnum((uint8_t)c) || c == '-' || c == '_' || c == '.' || c == '~') o += c;
      else { sprintf(b, "%%%02X", (uint8_t)c); o += b; }
    }
    return o;
  }
#endif
  // The number after "key": searched from `after` (skips e.g. "current_units" or the results header).
  static float numAfter(const String &s, const char *after, const char *key) {
    int base = s.indexOf(after);
    int i = s.indexOf(String("\"") + key + "\":", base < 0 ? 0 : base);
    if (i < 0) return NAN;
    return s.substring(i + (int)strlen(key) + 3).toFloat();
  }
  static String strValue(const String &s, const char *key, const String &def) {
    int i = s.indexOf(String("\"") + key + "\":\"");
    if (i < 0) return def;
    i += (int)strlen(key) + 4;
    int e = s.indexOf('"', i);
    return e < 0 ? def : s.substring(i, e);
  }
  float _lat = 0, _lon = 0, _temp = NAN, _wind = 0;
  int _code = 0;
  bool _valid = false;
  String _city = "";
};
