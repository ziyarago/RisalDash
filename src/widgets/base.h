#pragma once
#include <Arduino.h>
#include <functional>
#include <math.h>

// Widget model core. A widget binds to a variable by pointer, renders its card, ships its
// type's CSS/JS once (Zero-Waste), reports value changes (poll) and serializes them
// ("key":value), and — for controls — applies commands arriving over the WebSocket.
// The concrete widget types live in the sibling headers (display/controls/inputs/visual/layout).

// Widget footprint on the iOS-style cell grid. Small = 1 cell, Medium = full width, Large = full
// width + double height. Each type has a sensible default (rDefaultSize); override with .size().
enum RSize : uint8_t { RSIZE_S = 0, RSIZE_M = 1, RSIZE_L = 2, RSIZE_AUTO = 255 };
inline uint8_t rDefaultSize(const char* t) {
  if (!strcmp(t, "gauge") || !strcmp(t, "chart") || !strcmp(t, "table") || !strcmp(t, "ai") ||
      !strcmp(t, "log") || !strcmp(t, "image"))
    return RSIZE_L;  // need vertical room
  if (!strcmp(t, "metric") || !strcmp(t, "stat") || !strcmp(t, "badge") || !strcmp(t, "led") ||
      !strcmp(t, "label") || !strcmp(t, "toggle") || !strcmp(t, "number") || !strcmp(t, "color"))
    return RSIZE_S;  // compact single value / control
  return RSIZE_M;    // slider, progress, select, text, password, time, date, button
}

// ── Optional author-string translation (widget titles, layout/group/tab names, button labels,
// select options). These are strings YOU write, so the library can't translate them itself.
// Register a translator with dash.translate(fn); it's called at render time with the effective
// language ("en"/"ru"/"uz"/"ar") and must return a pointer to a stable (static/PROGMEM-copied)
// string. Return the original for anything you don't map. Zero cost when no translator is set.
typedef const char* (*RTranslator)(const char* text, const char* lang);
extern RTranslator g_rTranslator;
extern const char* g_rActiveLang;  // set per request to the effective language before rendering
inline const char* rI18n(const char* s) {
  return (g_rTranslator && s) ? g_rTranslator(s, g_rActiveLang) : s;
}

class Widget {
 public:
  Widget(const char* key, const char* title) : _key(key), _title(title) {}
  virtual ~Widget() {}
  virtual const char* typeId() const = 0;
  virtual const char* css() const { return nullptr; }
  virtual const char* js() const { return nullptr; }
  virtual void card(Print& out) = 0;

  // Live transport (P3).
  virtual bool hasState() const { return false; }       // contributes a key:value
  virtual bool poll() { return false; }                 // true if changed since last poll
  virtual void writeKV(String& out) {}                  // append "key":value
  virtual void applyCommand(const String& v) { (void)v; }  // control widgets
  // Describe this control for the Settings modal when it is .gear()'d — emit a JSON object
  // {"n","k","t",...params} and return true. Default: not renderable there. Overridden by controls.
  virtual bool writeGearMeta(Print& out) { (void)out; return false; }

  const char* key() const { return _key; }

  // Size preset on the cell grid: RSIZE_S (1 cell), RSIZE_M (full width), RSIZE_L (full width,
  // double height). Overrides the type's default. Call last in a chain: dash.metric(...).size(RSIZE_L).
  Widget& size(RSize s) { _size = s; return *this; }

  // Show an icon in the card header instead of the accent dot. Pass a RICON_* path
  // (or any 24×24 SVG path string). Returns the base type — call it last in a chain.
  Widget& icon(const char* svgPath) { _icon = svgPath; return *this; }

  // Move this control out of the main grid and into the Settings modal (the appbar gear), so it
  // reads as a device setting rather than a dashboard tile. The widget still lives in /api/state
  // and /api/set — the modal drives it there. Currently rendered for toggles. Call it last in a
  // chain: dash.toggle("Auto-slide", &v).gear().
  Widget& gear() { _setting = true; return *this; }
  bool isSetting() const { return _setting; }

 protected:
  virtual void extraAttrs(Print& out) { (void)out; }    // extra data-* on the card
  void cardOpen(Print& out) {
    // Explicit .size() wins; otherwise the type's default footprint.
    uint8_t sz = _size != RSIZE_AUTO ? _size : rDefaultSize(typeId());
    out.print(F("<section class=\"card"));
    out.print(sz == RSIZE_S ? F(" s") : (sz == RSIZE_L ? F(" l") : F(" m")));
    out.print(F("\" data-type=\""));
    out.print(typeId());
    out.print(F("\" data-key=\""));
    out.print(_key);
    out.print('"');
    extraAttrs(out);
    out.print(F("><h3>"));
    if (_icon) {
      out.print(F("<svg class=\"ic\" viewBox=\"0 0 24 24\"><path d=\""));
      out.print(FPSTR(_icon));
      out.print(F("\"/></svg>"));
    } else {
      out.print(F("<i class=\"eb\"></i>"));
    }
    out.print(rI18n(_title));
    out.print(F("</h3>"));
  }
  void cardClose(Print& out) { out.print(F("</section>")); }

  const char* _key;
  const char* _title;
  const char* _icon = nullptr;
  uint8_t _size = RSIZE_AUTO;  // size preset override; RSIZE_AUTO -> use the type's default
  bool _setting = false;       // .gear() -> render in the Settings modal, not the grid
};

// Lowercase, non-alphanumeric → '-'. Used to anchor groups for the nav drawer.
inline void rwSlug(Print& out, const char* s) {
  for (; s && *s; s++) {
    char c = *s;
    if (c >= 'A' && c <= 'Z') c += 32;
    out.print((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ? c : '-');
  }
}

// String helpers for the string-valued widgets below.
inline String rwJsonEsc(const String& s) {
  String o; o.reserve(s.length() + 6);
  for (uint16_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n') { o += "\\n"; }
    else if (c != '\r') { o += c; }
  }
  return o;
}
inline void rwAttr(Print& out, const String& s) {
  for (uint16_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out.print(F("&amp;"));
    else if (c == '"') out.print(F("&quot;"));
    else if (c == '<') out.print(F("&lt;"));
    else out.print(c);
  }
}

// Localized On/Off for a control's FIRST paint (live updates use the client-side R.L
// strings). g_rActiveLang is set per request before rendering, so this is language-correct.
inline const char* rwOnOff(bool on) {
  char l = g_rActiveLang ? g_rActiveLang[0] : 'e';
  if (l == 'r') return on ? "Вкл" : "Выкл";
  if (l == 'u') return on ? "Yoniq" : "Oʻchiq";
  if (l == 'a') return on ? "تشغيل" : "إيقاف";
  return on ? "On" : "Off";
}

// Change tracker behind poll(): remembers the last pushed value; the first call always
// reports a change so new WebSocket clients get seeded. Replaces the per-widget
// `_last/_seen` boilerplate.
template <typename T>
struct RwTracked {
  T last{};
  bool seen = false;
  bool changed(const T& v) {
    if (seen && v == last) return false;
    seen = true;
    last = v;
    return true;
  }
};

// Walk a comma-separated option list: fn(start, end, index) per option, no allocation.
// Shared by select/radio (cards + Settings-modal meta).
template <typename F>
inline void rwCsvEach(const char* csv, F fn) {
  int i = 0;
  for (const char* p = csv; p && *p;) {
    const char* q = p;
    while (*q && *q != ',') q++;
    fn(p, q, i++);
    p = (*q) ? q + 1 : q;
  }
}
