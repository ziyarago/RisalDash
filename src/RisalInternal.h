#pragma once
// Internal shared bits for the RisalUI translation units. Not part of the public API.
#include <Arduino.h>
#include <Print.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <Preferences.h>
  #include <Update.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <EEPROM.h>
  #include <Updater.h>
#endif

// Localized library chrome (portal + footer). Kept tiny; widget titles are the dev's own.
enum RLoc { L_SUBTITLE, L_PASSWORD, L_CONNECT, L_SERVED, L_NETWORKS, L_TIMEZONE };
inline const char* rloc(RLoc k, const char* lang) {
  bool ru = lang[0] == 'r';
  bool ar = lang[0] == 'a';
  bool uz = lang[0] == 'u';
  switch (k) {
    case L_SUBTITLE: return ar ? "إعداد Wi-Fi · اختر الشبكة" : ru ? "Настройка Wi-Fi · выберите сеть" : uz ? "Wi-Fi sozlash · tarmoqni tanlang" : "Wi-Fi setup · choose your network";
    case L_PASSWORD: return ar ? "كلمة المرور" : ru ? "Пароль" : uz ? "Parol" : "Password";
    case L_CONNECT:  return ar ? "اتصال وحفظ" : ru ? "Подключить и сохранить" : uz ? "Ulash va saqlash" : "Connect & save";
    case L_SERVED:   return ar ? "يخدمها ESP" : ru ? "обслуживается ESP" : uz ? "ESP tomonidan" : "served by ESP";
    case L_NETWORKS: return ar ? "الشبكات" : ru ? "Сети" : uz ? "Tarmoqlar" : "Networks";
    case L_TIMEZONE: return ar ? "المنطقة الزمنية" : ru ? "Часовой пояс" : uz ? "Vaqt mintaqasi" : "Timezone";
  }
  return "";
}


// Windowing Print sink for chunked streaming. The fill-callback re-runs the whole render on each
// call and this keeps only the bytes of the current window [start, start+max) — flat RAM, no big
// contiguous buffer to hit ESP8266 heap fragmentation (which capped AsyncResponseStream at ~15 KB
// and silently dropped the rest, breaking every page's trailing <script>).
class WindowPrint : public Print {
  uint8_t* _buf;
  size_t _cap, _start, _pos = 0, _n = 0;
 public:
  WindowPrint(uint8_t* buf, size_t maxLen, size_t start) : _buf(buf), _cap(maxLen), _start(start) {}
  size_t produced() const { return _n; }
  size_t write(uint8_t c) override {
    if (_pos >= _start && _n < _cap) _buf[_n++] = c;
    _pos++;
    return 1;
  }
  size_t write(const uint8_t* data, size_t len) override {
    size_t segStart = _pos;
    _pos += len;
    if (_n >= _cap || segStart + len <= _start || segStart >= _start + _cap) return len;
    size_t from = segStart < _start ? _start - segStart : 0;
    size_t to = segStart + len < _start + _cap ? len : _start + _cap - segStart;
    size_t cpy = to - from;
    if (cpy > _cap - _n) cpy = _cap - _n;
    memcpy(_buf + _n, data + from, cpy);
    _n += cpy;
    return len;
  }
};

