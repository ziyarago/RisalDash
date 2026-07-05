#pragma once
#include <Arduino.h>
#include <functional>
#include <math.h>

// Widget model (ported from the dev/ mock). A widget binds to a variable by pointer,
// renders its card, ships its type's CSS/JS once (Zero-Waste), reports value changes
// (poll) and serializes them ("key":value), and — for controls — applies commands
// arriving over the WebSocket.

// ── Per-type CSS (single TU: included only by RisalUI.cpp) ──
static const char RW_GAUGE_CSS[] PROGMEM =
  ".gauge{display:grid;place-items:center;position:relative;padding:4px 0}"
  ".gauge svg{transform:rotate(-90deg)}"
  ".gauge .gv{position:absolute;font:800 22px/1 var(--font);font-variant-numeric:tabular-nums}"
  ".gauge .gv .unit{font-size:11px}";

static const char RW_TOGGLE_CSS[] PROGMEM =
  ".tg{display:flex;align-items:center;justify-content:space-between}"
  ".tg span{font-size:15px;color:var(--ink2)}"
  ".sw{width:46px;height:26px;border-radius:99px;background:var(--bg3);border:1px solid var(--line2);position:relative;cursor:pointer}"
  ".sw::after{content:'';position:absolute;top:2px;inset-inline-start:2px;width:20px;height:20px;border-radius:50%;background:#fff;transition:transform .25s}"
  ".sw.on{background:var(--grad);border-color:transparent}.sw.on::after{transform:translateX(20px)}";

// ── Per-type client JS: registers R.W[type] = { init?, update } (no ${} template literals) ──
static const char RW_METRIC_JS[] PROGMEM =
  "R.W.metric={update:function(el,v){el.querySelector('.big').textContent=v;"
  "var b=el.querySelector('.bar>i');if(b)b.style.width=Math.max(0,Math.min(100,v))+'%';}};";

static const char RW_GAUGE_JS[] PROGMEM =
  "R.W.gauge={update:function(el,v){var lo=+el.dataset.min,hi=+el.dataset.max,"
  "f=Math.max(0,Math.min(1,(v-lo)/((hi-lo)||1)));"
  "var a=el.querySelector('.arc');if(a)a.style.strokeDashoffset=263.9*(1-f);"
  "var b=el.querySelector('.gv b');if(b)b.textContent=(+v).toFixed(1);}};";

static const char RW_TOGGLE_JS[] PROGMEM =
  "R.W.toggle={init:function(el){var s=el.querySelector('.sw');"
  "if(s)s.addEventListener('click',function(){R.send(el.dataset.key,!s.classList.contains('on'));});},"
  "update:function(el,v){var s=el.querySelector('.sw');if(s)s.classList.toggle('on',!!v);"
  "var t=el.querySelector('.tg span');if(t)t.textContent=v?R.L.on:R.L.off;}};";

// Curated IoT icon set (MDI-style 24×24 path data). Each is its own PROGMEM symbol, so
// --gc-sections keeps only the ones a widget actually references (Zero-Waste). Pass any
// other path string to .icon() to use a different glyph.
static const char RICON_THERMOMETER[] PROGMEM = "M12 3a3 3 0 0 0-3 3v7.1a4 4 0 1 0 6 0V6a3 3 0 0 0-3-3zm0 16a2 2 0 0 1-1-3.7V6a1 1 0 0 1 2 0v9.3A2 2 0 0 1 12 19z";
static const char RICON_WATER[] PROGMEM = "M12 3c-3.2 4.2-6 7.3-6 10.2A6 6 0 0 0 18 13.2C18 10.3 15.2 7.2 12 3z";
static const char RICON_FLASH[] PROGMEM = "M13 2L4 14h6l-1 8 9-12h-6l1-8z";
static const char RICON_BULB[] PROGMEM = "M9 21h6v-1H9v1zm3-19a7 7 0 0 0-4 12.7V17h8v-2.3A7 7 0 0 0 12 2z";
static const char RICON_POWER[] PROGMEM = "M11 3h2v10h-2V3zm5.6 3.6 1.4 1.4a8 8 0 1 1-12 0L7.4 6.6a6 6 0 1 0 9.2 0z";
static const char RICON_GAUGE[] PROGMEM = "M12 16A2 2 0 0 1 10 14C10 13 14 8 14 8C14 8 14 13 14 14A2 2 0 0 1 12 16M12 3A9 9 0 0 0 3 12A9 9 0 0 0 12 21A9 9 0 0 0 21 12A9 9 0 0 0 12 3M12 5A7 7 0 0 1 19 12A7 7 0 0 1 12 19A7 7 0 0 1 5 12A7 7 0 0 1 12 5Z";
static const char RICON_HOME[] PROGMEM = "M12 3 3 11h3v9h5v-6h2v6h5v-9h3L12 3z";
static const char RICON_WIFI[] PROGMEM = "M12 18l3-3a4.2 4.2 0 0 0-6 0l3 3zM5 11a10 10 0 0 1 14 0l-2 2a7 7 0 0 0-10 0l-2-2z";
static const char RICON_CLOCK[] PROGMEM = "M12 20A8 8 0 0 1 4 12A8 8 0 0 1 12 4A8 8 0 0 1 20 12A8 8 0 0 1 12 20M12 2A10 10 0 0 0 2 12A10 10 0 0 0 12 22A10 10 0 0 0 22 12A10 10 0 0 0 12 2M12.5 7H11V13L16.25 16.15L17 14.92L12.5 12.25V7Z";
static const char RICON_SIGNAL[] PROGMEM = "M4 20h3v-7H4v7zm6.5 0h3V8h-3v12zm6.5 0h3V4h-3v16z";
static const char RICON_LEAF[] PROGMEM = "M17 8C8 10 6 16 5 21c0 0 8.5 1 12.5-6 1.6-2.8 1-7-.5-7z";
static const char RICON_MOTION[] PROGMEM = "M13.5 5.5A2 2 0 0 0 15.5 3.5A2 2 0 0 0 13.5 1.5A2 2 0 0 0 11.5 3.5A2 2 0 0 0 13.5 5.5M9.89 19.38L10.89 15L13 17V23H15V15.5L12.89 13.5L13.5 10.5C14.79 12 16.79 13 19 13V11C17.09 11 15.5 10 14.69 8.58L13.69 7C13.29 6.38 12.69 6 12 6C11.69 6 11.5 6.08 11.19 6.08L6 8.28V13H8V9.58L9.79 8.88L8.19 17L3.29 16L2.89 18L9.89 19.38Z";

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

  const char* key() const { return _key; }

  // Make the card span 2 or 3 grid columns on wide screens (collapses on mobile).
  // Returns the base type; call it last in a chain, e.g. dash.chart(...).span(2).
  Widget& span(uint8_t n) { _span = n; return *this; }

  // Size preset on the cell grid: RSIZE_S (1 cell), RSIZE_M (full width), RSIZE_L (full width,
  // double height). Overrides the type's default. Call last in a chain: dash.metric(...).size(RSIZE_L).
  Widget& size(RSize s) { _size = s; return *this; }

  // Show an icon in the card header instead of the accent dot. Pass a RICON_* path
  // (or any 24×24 SVG path string). Returns the base type — call it last in a chain.
  Widget& icon(const char* svgPath) { _icon = svgPath; return *this; }

 protected:
  virtual void extraAttrs(Print& out) { (void)out; }    // extra data-* on the card
  void cardOpen(Print& out) {
    // Explicit .size() wins; otherwise the type's default footprint. (.span() is legacy and no
    // longer drives the cell grid — the size presets replace it.)
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
    out.print(_title);
    out.print(F("</h3>"));
  }
  void cardClose(Print& out) { out.print(F("</section>")); }

  const char* _key;
  const char* _title;
  const char* _icon = nullptr;
  uint8_t _span = 1;
  uint8_t _size = RSIZE_AUTO;  // size preset override; RSIZE_AUTO -> use the type's default
};

// ── Display: metric (big number + bar) ──
class MetricWidget : public Widget {
 public:
  MetricWidget(const char* key, const char* title, float* val, const char* unit)
      : Widget(key, title), _val(val), _unit(unit) {}
  const char* typeId() const override { return "metric"; }
  const char* js() const override { return RW_METRIC_JS; }
  MetricWidget& decimals(uint8_t d) { _dec = d; return *this; }
  MetricWidget& zone(float warn, float bad) { _warn = warn; _bad = bad; return *this; }

  void card(Print& out) override {
    cardOpen(out);
    float v = _val ? *_val : 0.0f;
    out.print(F("<div class=\"row\"><span class=\"big\">"));
    out.print(v, _dec);
    out.print(F("</span><span class=\"unit\">"));
    out.print(_unit ? _unit : "");
    out.print(F("</span></div><div class=\"bar\"><i style=\"width:"));
    float pct = v < 0 ? 0 : (v > 100 ? 100 : v);
    out.print(pct, 0);
    out.print(F("%\"></i></div>"));
    cardClose(out);
  }

  bool hasState() const override { return true; }
  bool poll() override { float v = _val ? *_val : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override {
    char buf[16];
    dtostrf(_val ? *_val : 0.0f, 0, _dec, buf);
    out += '"'; out += _key; out += "\":"; out += buf;
  }

 private:
  float* _val;
  const char* _unit;
  uint8_t _dec = 0;
  float _warn = 0, _bad = 0;
  float _last = NAN;
};

// ── Display: circular gauge ──
class GaugeWidget : public Widget {
 public:
  GaugeWidget(const char* key, const char* title, float* val, float mn, float mx, const char* unit)
      : Widget(key, title), _val(val), _lo(mn), _hi(mx), _unit(unit) {}
  const char* typeId() const override { return "gauge"; }
  const char* css() const override { return RW_GAUGE_CSS; }
  const char* js() const override { return RW_GAUGE_JS; }

  void card(Print& out) override {
    cardOpen(out);
    float v = _val ? *_val : 0.0f;
    float span = (_hi - _lo) != 0 ? (_hi - _lo) : 1;
    float f = (v - _lo) / span;
    f = f < 0 ? 0 : (f > 1 ? 1 : f);
    out.print(F("<div class=\"gauge\"><svg width=\"116\" height=\"116\" viewBox=\"0 0 116 116\">"
                "<circle cx=\"58\" cy=\"58\" r=\"42\" fill=\"none\" stroke=\"var(--bg3)\" stroke-width=\"9\"/>"
                "<circle class=\"arc\" cx=\"58\" cy=\"58\" r=\"42\" fill=\"none\" stroke=\"url(#rg)\" stroke-width=\"9\" "
                "stroke-linecap=\"round\" stroke-dasharray=\"263.9\" stroke-dashoffset=\""));
    out.print(263.9f * (1.0f - f), 1);
    out.print(F("\"/></svg><div class=\"gv\"><b>"));
    out.print(v, 1);
    out.print(F("</b> <span class=\"unit\">"));
    out.print(_unit ? _unit : "");
    out.print(F("</span></div></div>"));
    cardClose(out);
  }

  bool hasState() const override { return true; }
  bool poll() override { float v = _val ? *_val : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override {
    char buf[16];
    dtostrf(_val ? *_val : 0.0f, 0, 2, buf);
    out += '"'; out += _key; out += "\":"; out += buf;
  }

 protected:
  void extraAttrs(Print& out) override {
    out.print(F(" data-min=\""));
    out.print(_lo, 2);
    out.print(F("\" data-max=\""));
    out.print(_hi, 2);
    out.print('"');
  }

 private:
  float* _val;
  float _lo, _hi;
  const char* _unit;
  float _last = NAN;
};

// ── Control: toggle (clickable over the WebSocket) ──
class ToggleWidget : public Widget {
 public:
  using Cb = std::function<void(bool)>;
  ToggleWidget(const char* key, const char* title, bool* val, Cb cb)
      : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "toggle"; }
  const char* css() const override { return RW_TOGGLE_CSS; }
  const char* js() const override { return RW_TOGGLE_JS; }

  void card(Print& out) override {
    cardOpen(out);
    bool on = _val ? *_val : false;
    out.print(F("<div class=\"tg\"><span>"));
    out.print(on ? F("On") : F("Off"));
    out.print(F("</span><div class=\"sw"));
    if (on) out.print(F(" on"));
    out.print(F("\"></div></div>"));
    cardClose(out);
  }

  bool hasState() const override { return true; }
  bool poll() override { int8_t v = (_val && *_val) ? 1 : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += ((_val && *_val) ? "true" : "false"); }
  // Leave _last unchanged so the next poll() rebroadcasts the new state to every client.
  void applyCommand(const String& v) override {
    bool b = (v == "true" || v == "1");
    if (_val) *_val = b;
    if (_cb) _cb(b);
  }

 private:
  bool* _val;
  Cb _cb;
  int8_t _last = -1;
};

// ════════ Additional widget types ════════

static const char RW_SLIDER_CSS[] PROGMEM =
  "input[type=range]{width:100%;-webkit-appearance:none;appearance:none;height:6px;border-radius:99px;background:var(--bg3);outline:none}"
  "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:var(--acc);cursor:pointer;border:3px solid var(--bg2)}"
  "input[type=range]::-moz-range-thumb{width:15px;height:15px;border-radius:50%;background:var(--acc);border:3px solid var(--bg2);cursor:pointer}";
static const char RW_BUTTON_CSS[] PROGMEM =
  ".act{height:46px;border:none;border-radius:13px;background:var(--grad);color:var(--acc-ink);font:700 15px var(--font);cursor:pointer;width:100%}"
  ".act:active{transform:scale(.97)}";
static const char RW_BADGE_CSS[] PROGMEM =
  ".badge{align-self:flex-start;font:700 12px var(--font);padding:6px 11px;border-radius:999px}"
  ".badge.ok{color:#34D399;background:rgba(52,211,153,.14)}"
  ".badge.warn{color:#FBBF24;background:rgba(251,191,36,.14)}"
  ".badge.bad{color:#F87171;background:rgba(248,113,113,.14)}";
static const char RW_LED_CSS[] PROGMEM =
  ".led{display:flex;align-items:center;justify-content:space-between;color:var(--ink2);font-size:15px}"
  ".led i{width:13px;height:13px;border-radius:50%;background:var(--bg3);box-shadow:inset 0 0 0 1px var(--line2)}"
  ".led i.on{background:#34D399;box-shadow:0 0 10px #34D399}";
static const char RW_CHART_CSS[] PROGMEM =
  ".spark{width:100%;height:62px;display:block}"
  ".spark .ln{fill:none;stroke:url(#rg);stroke-width:2.4;stroke-linejoin:round;stroke-linecap:round}"
  ".spark .fl{fill:url(#rgf);stroke:none}"
  ".cstat{display:flex;justify-content:space-between;font:600 11px var(--mono);color:var(--ink3);margin-top:4px}"
  ".cstat b{color:var(--ink2);font-weight:700}";

static const char RW_SLIDER_JS[] PROGMEM =
  "R.W.slider={init:function(el){var i=el.querySelector('input');if(!i)return;"
  "i.addEventListener('input',function(){var b=el.querySelector('.big');if(b)b.textContent=i.value;});"
  "i.addEventListener('change',function(){R.send(el.dataset.key,+i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;var b=el.querySelector('.big');if(b)b.textContent=v;}};";
static const char RW_BUTTON_JS[] PROGMEM =
  "R.W.button={init:function(el){var b=el.querySelector('.act');if(b)b.addEventListener('click',function(){R.send(el.dataset.key,true);});}};";
static const char RW_BADGE_JS[] PROGMEM =
  "R.W.badge={update:function(el,v){var c=['ok','warn','bad'][v]||'ok';var b=el.querySelector('.badge');if(!b)return;"
  "b.className='badge '+c;var t=el.dataset['l'+v];if(t!=null)b.textContent=t;}};";
static const char RW_LED_JS[] PROGMEM =
  "R.W.led={update:function(el,v){var i=el.querySelector('.led i');if(i)i.classList.toggle('on',!!v);"
  "var b=el.querySelector('.led b');if(b)b.textContent=v?R.L.on:R.L.off;}};";
static const char RW_PROGRESS_JS[] PROGMEM =
  "R.W.progress={update:function(el,v){var b=el.querySelector('.big');if(b)b.textContent=v;"
  "var i=el.querySelector('.bar>i');if(i)i.style.width=Math.max(0,Math.min(100,v))+'%';}};";
static const char RW_STAT_JS[] PROGMEM =
  "R.W.stat={update:function(el,v){var b=el.querySelector('.big');if(b)b.textContent=v;}};";
static const char RW_CHART_JS[] PROGMEM =
  "R.W.chart={init:function(el){el._h=[];},update:function(el,v){var h=el._h||(el._h=[]);h.push(+v);if(h.length>30)h.shift();"
  "var bg=el.querySelector('.big');if(bg)bg.textContent=v;if(h.length<2)return;"
  "var w=300,ht=62,mn=Math.min.apply(0,h),mx=Math.max.apply(0,h),r=(mx-mn)||1,st=w/(h.length-1);"
  "var p=h.map(function(y,i){return (i*st).toFixed(1)+' '+(ht-((y-mn)/r)*(ht-8)-4).toFixed(1);});"
  "var ln='M'+p.join(' L');var L=el.querySelector('.ln');if(L)L.setAttribute('d',ln);"
  "var F=el.querySelector('.fl');if(F)F.setAttribute('d',ln+' L'+w+' '+ht+' L0 '+ht+' Z');"
  "var sum=0;for(var i=0;i<h.length;i++)sum+=h[i];var av=sum/h.length;"
  "var m=el.querySelector('.cmn');if(m)m.textContent=mn.toFixed(1);"
  "var a=el.querySelector('.cav');if(a)a.textContent=av.toFixed(1);"
  "var x=el.querySelector('.cmx');if(x)x.textContent=mx.toFixed(1);}};";

// ── Control: slider (int range) ──
class SliderWidget : public Widget {
 public:
  using Cb = std::function<void(int)>;
  SliderWidget(const char* key, const char* title, int* val, int mn, int mx, Cb cb)
      : Widget(key, title), _val(val), _lo(mn), _hi(mx), _cb(cb) {}
  const char* typeId() const override { return "slider"; }
  const char* css() const override { return RW_SLIDER_CSS; }
  const char* js() const override { return RW_SLIDER_JS; }
  void card(Print& out) override {
    cardOpen(out);
    int v = _val ? *_val : 0;
    out.print(F("<div class=\"row\"><span class=\"big\">"));
    out.print(v);
    out.print(F("</span><span class=\"unit\">/ "));
    out.print(_hi);
    out.print(F("</span></div><input type=\"range\" min=\""));
    out.print(_lo);
    out.print(F("\" max=\""));
    out.print(_hi);
    out.print(F("\" value=\""));
    out.print(v);
    out.print(F("\">"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _val ? *_val : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_val) *_val = n; if (_cb) _cb(n); }
 private:
  int* _val; int _lo, _hi; Cb _cb; int _last = 0; bool _seen = false;
};

// ── Control: button (momentary action) ──
class ButtonWidget : public Widget {
 public:
  using Cb = std::function<void()>;
  ButtonWidget(const char* key, const char* title, const char* label, Cb cb)
      : Widget(key, title), _label(label), _cb(cb) {}
  const char* typeId() const override { return "button"; }
  const char* css() const override { return RW_BUTTON_CSS; }
  const char* js() const override { return RW_BUTTON_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<button class=\"act\">"));
    out.print(_label ? _label : _title);
    out.print(F("</button>"));
    cardClose(out);
  }
  void applyCommand(const String& v) override { (void)v; if (_cb) _cb(); }
 private:
  const char* _label; Cb _cb;
};

// ── Display: status badge (0=ok, 1=warn, 2=bad) ──
class BadgeWidget : public Widget {
 public:
  BadgeWidget(const char* key, const char* title, int* val) : Widget(key, title), _val(val) {}
  const char* typeId() const override { return "badge"; }
  const char* css() const override { return RW_BADGE_CSS; }
  const char* js() const override { return RW_BADGE_JS; }
  BadgeWidget& labels(const char* ok, const char* warn, const char* bad) { _l[0] = ok; _l[1] = warn; _l[2] = bad; return *this; }
  void card(Print& out) override {
    cardOpen(out);
    int v = _val ? *_val : 0; if (v < 0) v = 0; if (v > 2) v = 2;
    out.print(F("<span class=\"badge "));
    out.print(v == 2 ? "bad" : (v == 1 ? "warn" : "ok"));
    out.print(F("\">"));
    out.print(_l[v]);
    out.print(F("</span>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _val ? *_val : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
 protected:
  void extraAttrs(Print& out) override {
    for (int i = 0; i < 3; i++) { out.print(F(" data-l")); out.print(i); out.print(F("=\"")); out.print(_l[i]); out.print('"'); }
  }
 private:
  int* _val; const char* _l[3] = {"OK", "Warn", "Bad"}; int _last = 0; bool _seen = false;
};

// ── Display: LED indicator (bool) ──
class LedWidget : public Widget {
 public:
  LedWidget(const char* key, const char* title, bool* val) : Widget(key, title), _val(val) {}
  const char* typeId() const override { return "led"; }
  const char* css() const override { return RW_LED_CSS; }
  const char* js() const override { return RW_LED_JS; }
  void card(Print& out) override {
    cardOpen(out);
    bool on = _val ? *_val : false;
    out.print(F("<div class=\"led\"><b>"));
    out.print(on ? F("on") : F("off"));
    out.print(F("</b><i"));
    if (on) out.print(F(" class=\"on\""));
    out.print(F("></i></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int8_t v = (_val && *_val) ? 1 : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += ((_val && *_val) ? "true" : "false"); }
 private:
  bool* _val; int8_t _last = -1;
};

// ── Display: progress bar (int 0..100) ──
class ProgressWidget : public Widget {
 public:
  ProgressWidget(const char* key, const char* title, int* val, const char* unit) : Widget(key, title), _val(val), _unit(unit) {}
  const char* typeId() const override { return "progress"; }
  const char* js() const override { return RW_PROGRESS_JS; }
  void card(Print& out) override {
    cardOpen(out);
    int v = _val ? *_val : 0;
    out.print(F("<div class=\"row\"><span class=\"big\">"));
    out.print(v);
    out.print(F("</span><span class=\"unit\">"));
    out.print(_unit ? _unit : "%");
    out.print(F("</span></div><div class=\"bar\"><i style=\"width:"));
    out.print(v < 0 ? 0 : (v > 100 ? 100 : v));
    out.print(F("%\"></i></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _val ? *_val : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
 private:
  int* _val; const char* _unit; int _last = 0; bool _seen = false;
};

// ── Display: stat (read-only number) ──
class StatWidget : public Widget {
 public:
  StatWidget(const char* key, const char* title, float* val, const char* unit) : Widget(key, title), _val(val), _unit(unit) {}
  const char* typeId() const override { return "stat"; }
  const char* js() const override { return RW_STAT_JS; }
  StatWidget& decimals(uint8_t d) { _dec = d; return *this; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"row\"><span class=\"big\">"));
    out.print(_val ? *_val : 0.0f, _dec);
    out.print(F("</span><span class=\"unit\">"));
    out.print(_unit ? _unit : "");
    out.print(F("</span></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { float v = _val ? *_val : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override { char b[16]; dtostrf(_val ? *_val : 0.0f, 0, _dec, b); out += '"'; out += _key; out += "\":"; out += b; }
 private:
  float* _val; const char* _unit; uint8_t _dec = 0; float _last = NAN;
};

// ── Display: chart / sparkline (history kept client-side) ──
class ChartWidget : public Widget {
 public:
  ChartWidget(const char* key, const char* title, float* val, const char* unit) : Widget(key, title), _val(val), _unit(unit) {}
  const char* typeId() const override { return "chart"; }
  const char* css() const override { return RW_CHART_CSS; }
  const char* js() const override { return RW_CHART_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"row\"><span class=\"big\">"));
    out.print(_val ? *_val : 0.0f, 1);
    out.print(F("</span><span class=\"unit\">"));
    out.print(_unit ? _unit : "");
    out.print(F("</span></div><svg class=\"spark\" viewBox=\"0 0 300 62\" preserveAspectRatio=\"none\">"
                "<path class=\"fl\"></path><path class=\"ln\"></path></svg>"
                "<div class=\"cstat\"><span>min <b class=\"cmn\">-</b></span>"
                "<span>avg <b class=\"cav\">-</b></span><span>max <b class=\"cmx\">-</b></span></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { float v = _val ? *_val : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override { char b[16]; dtostrf(_val ? *_val : 0.0f, 0, 1, b); out += '"'; out += _key; out += "\":"; out += b; }
 private:
  float* _val; const char* _unit; float _last = NAN;
};

static const char RW_GROUP_CSS[] PROGMEM =
  ".group{grid-column:1/-1;margin:8px 2px 0;font:700 11px/1 var(--font);letter-spacing:.16em;text-transform:uppercase;color:var(--ink3)}";
static const char RW_NUMBER_CSS[] PROGMEM =
  ".ninp{width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:16px var(--font);padding:0 12px}";
static const char RW_SELECT_CSS[] PROGMEM =
  ".rsel{width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px;cursor:pointer}";
static const char RW_NUMBER_JS[] PROGMEM =
  "R.W.number={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,+i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_SELECT_JS[] PROGMEM =
  "R.W.select={init:function(el){var s=el.querySelector('select');if(s)s.addEventListener('change',function(){R.send(el.dataset.key,s.selectedIndex);});},"
  "update:function(el,v){var s=el.querySelector('select');if(s)s.selectedIndex=v;}};";

// Lowercase, non-alphanumeric → '-'. Used to anchor groups for the nav drawer.
inline void rwSlug(Print& out, const char* s) {
  for (; s && *s; s++) {
    char c = *s;
    if (c >= 'A' && c <= 'Z') c += 32;
    out.print((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ? c : '-');
  }
}

// ── Layout: group / section header (spans the grid; anchors the nav drawer) ──
class GroupWidget : public Widget {
 public:
  explicit GroupWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "group"; }
  const char* css() const override { return RW_GROUP_CSS; }
  void card(Print& out) override {
    out.print(F("<div class=\"group\" id=\"g-"));
    rwSlug(out, _title);
    out.print(F("\">"));
    out.print(_title);
    out.print(F("</div>"));
  }
};

// ── Layout: a full switchable page. With layouts declared, the dashboard shows one page at
// a time; a swipe-up sheet lists each as an icon tile. The marker itself renders nothing —
// RisalUI partitions the widgets that follow it into that page. Set the tile glyph via the
// second arg (a RICON_* path); falls back to a generic icon. ──
class LayoutWidget : public Widget {
 public:
  explicit LayoutWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "layout"; }
  void card(Print& out) override { (void)out; }  // a boundary marker, not a card
  const char* iconPath() const { return _icon; }
};

// ── Control: number input (int) ──
class NumberWidget : public Widget {
 public:
  using Cb = std::function<void(int)>;
  NumberWidget(const char* key, const char* title, int* val, int mn, int mx, int step, Cb cb)
      : Widget(key, title), _val(val), _lo(mn), _hi(mx), _step(step), _cb(cb) {}
  const char* typeId() const override { return "number"; }
  const char* css() const override { return RW_NUMBER_CSS; }
  const char* js() const override { return RW_NUMBER_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<input class=\"ninp\" type=\"number\" min=\""));
    out.print(_lo);
    out.print(F("\" max=\""));
    out.print(_hi);
    out.print(F("\" step=\""));
    out.print(_step);
    out.print(F("\" value=\""));
    out.print(_val ? *_val : 0);
    out.print(F("\">"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _val ? *_val : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_val) *_val = n; if (_cb) _cb(n); }
 private:
  int* _val; int _lo, _hi, _step; Cb _cb; int _last = 0; bool _seen = false;
};

// ── Control: select / dropdown (CSV options, bound to a selected index) ──
class SelectWidget : public Widget {
 public:
  using Cb = std::function<void(int)>;
  SelectWidget(const char* key, const char* title, const char* options, int* idx, Cb cb)
      : Widget(key, title), _opts(options), _idx(idx), _cb(cb) {}
  const char* typeId() const override { return "select"; }
  const char* css() const override { return RW_SELECT_CSS; }
  const char* js() const override { return RW_SELECT_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<select class=\"rsel\">"));
    int cur = _idx ? *_idx : 0, i = 0;
    for (const char* p = _opts; p && *p;) {
      const char* q = p;
      while (*q && *q != ',') q++;
      out.print(F("<option"));
      if (i == cur) out.print(F(" selected"));
      out.print('>');
      for (const char* c = p; c < q; c++) out.print(*c);
      out.print(F("</option>"));
      i++;
      p = (*q) ? q + 1 : q;
    }
    out.print(F("</select>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _idx ? *_idx : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_idx ? *_idx : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_idx) *_idx = n; if (_cb) _cb(n); }
 private:
  const char* _opts; int* _idx; Cb _cb; int _last = 0; bool _seen = false;
};

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

static const char RW_LABEL_CSS[] PROGMEM = ".lbl{font:600 16px var(--font);color:var(--ink1);word-break:break-word}";
static const char RW_TEXT_CSS[] PROGMEM =
  ".tinp{width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px}";
static const char RW_LOG_CSS[] PROGMEM =
  ".log{font:11.5px/1.8 var(--mono);color:var(--ink3);background:var(--bg2);border:1px solid var(--line);border-radius:11px;padding:10px;height:96px;overflow:hidden}";
static const char RW_LABEL_JS[] PROGMEM =
  "R.W.label={update:function(el,v){var e=el.querySelector('.lbl');if(e)e.textContent=v;}};";
static const char RW_TEXT_JS[] PROGMEM =
  "R.W.text={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_LOG_JS[] PROGMEM =
  "R.W.log={update:function(el,v){var e=el.querySelector('.log');if(e)e.innerHTML=v;}};";

// ── Display: text label (bound to a String) ──
class LabelWidget : public Widget {
 public:
  LabelWidget(const char* key, const char* title, String* val) : Widget(key, title), _val(val) {}
  const char* typeId() const override { return "label"; }
  const char* css() const override { return RW_LABEL_CSS; }
  const char* js() const override { return RW_LABEL_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"lbl\">"));
    if (_val) rwAttr(out, *_val);
    out.print(F("</div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
 private:
  String* _val; String _last; bool _seen = false;
};

// ── Control: text input (bound to a String) ──
class TextWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  TextWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "text"; }
  const char* css() const override { return RW_TEXT_CSS; }
  const char* js() const override { return RW_TEXT_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<input class=\"tinp\" type=\"text\" value=\""));
    if (_val) rwAttr(out, *_val);
    out.print(F("\">"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; String _last; bool _seen = false;
};

// ── Display: scrolling log / console ──
class LogWidget : public Widget {
 public:
  LogWidget(const char* key, const char* title, uint8_t maxLines) : Widget(key, title), _cap(maxLines > 8 ? 8 : maxLines) {}
  const char* typeId() const override { return "log"; }
  const char* css() const override { return RW_LOG_CSS; }
  const char* js() const override { return RW_LOG_JS; }
  LogWidget& print(const String& line) {
    for (int i = _cap - 1; i > 0; i--) _arr[i] = _arr[i - 1];
    _arr[0] = line;
    if (_n < _cap) _n++;
    _dirty = true;
    return *this;
  }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"log\">"));
    out.print(_joined());
    out.print(F("</div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { if (_dirty) { _dirty = false; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_joined()); out += '"'; }
 private:
  String _joined() {
    String j;
    for (uint8_t i = 0; i < _n; i++) { if (i) j += "<br>"; j += _arr[i]; }
    return j;
  }
  String _arr[8];
  uint8_t _cap, _n = 0;
  bool _dirty = false;
};

// ════════ password / color / time / image / table ════════

static const char RW_COLOR_CSS[] PROGMEM =
  ".col{display:flex;align-items:center;gap:12px}.col input{width:40px;height:40px;border:none;border-radius:10px;background:none;cursor:pointer}"
  ".col b{font:600 14px var(--mono);color:var(--ink2)}";
static const char RW_IMAGE_CSS[] PROGMEM =
  ".imgw{border-radius:12px;overflow:hidden;border:1px solid var(--line);aspect-ratio:16/10;background:var(--bg2)}"
  ".imgw img{width:100%;height:100%;object-fit:cover;display:block}";
static const char RW_TABLE_CSS[] PROGMEM =
  ".kv{display:flex;justify-content:space-between;font-size:13px;padding:7px 0;border-bottom:1px solid var(--line)}"
  ".kv:last-child{border-bottom:none}.kv span:first-child{color:var(--ink3)}"
  ".kv span:last-child{font-family:var(--mono);color:var(--ink2)}";
static const char RW_PASSWORD_JS[] PROGMEM =
  "R.W.password={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_TIME_JS[] PROGMEM =
  "R.W.time={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_COLOR_JS[] PROGMEM =
  "R.W.color={init:function(el){var i=el.querySelector('input');if(!i)return;"
  "i.addEventListener('input',function(){var b=el.querySelector('b');if(b)b.textContent=i.value;});"
  "i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;var b=el.querySelector('b');if(b)b.textContent=v;}};";
static const char RW_IMAGE_JS[] PROGMEM =
  "R.W.image={update:function(el,v){var i=el.querySelector('img');if(i&&v)i.src=v;}};";
static const char RW_TABLE_JS[] PROGMEM =
  "R.W.table={update:function(el,v){var e=el.querySelector('.kvb');if(e)e.innerHTML=v;}};";

// ── Control: password input (String, masked) ──
class PasswordWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  PasswordWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "password"; }
  const char* css() const override { return RW_TEXT_CSS; }
  const char* js() const override { return RW_PASSWORD_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<input class=\"tinp\" type=\"password\" value=\""));
    if (_val) rwAttr(out, *_val);
    out.print(F("\">"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; String _last; bool _seen = false;
};

// ── Control: time input (String "HH:MM") ──
class TimeWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  TimeWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "time"; }
  const char* css() const override { return RW_TEXT_CSS; }
  const char* js() const override { return RW_TIME_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<input class=\"tinp\" type=\"time\" value=\""));
    if (_val) rwAttr(out, *_val);
    out.print(F("\">"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; String _last; bool _seen = false;
};

// ── Control: color picker (String hex "#rrggbb") ──
class ColorWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  ColorWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "color"; }
  const char* css() const override { return RW_COLOR_CSS; }
  const char* js() const override { return RW_COLOR_JS; }
  void card(Print& out) override {
    cardOpen(out);
    String hex = _val && _val->length() ? *_val : String("#22d3ee");
    out.print(F("<div class=\"col\"><input type=\"color\" value=\""));
    rwAttr(out, hex);
    out.print(F("\"><b>"));
    out.print(hex);
    out.print(F("</b></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; String _last; bool _seen = false;
};

// ── Display: image (String URL) ──
class ImageWidget : public Widget {
 public:
  ImageWidget(const char* key, const char* title, String* url) : Widget(key, title), _url(url) {}
  const char* typeId() const override { return "image"; }
  const char* css() const override { return RW_IMAGE_CSS; }
  const char* js() const override { return RW_IMAGE_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"imgw\"><img src=\""));
    if (_url) rwAttr(out, *_url);
    out.print(F("\" alt=\"\"></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _url ? *_url : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_url ? *_url : String()); out += '"'; }
 private:
  String* _url; String _last; bool _seen = false;
};

// ── Display: key/value table (rows bound to float vars) ──
class TableWidget : public Widget {
 public:
  TableWidget(const char* key, const char* title) : Widget(key, title) {}
  const char* typeId() const override { return "table"; }
  const char* css() const override { return RW_TABLE_CSS; }
  const char* js() const override { return RW_TABLE_JS; }
  TableWidget& row(const char* label, float* val, const char* unit = "", uint8_t dec = 0) {
    if (_n < 8) { _rl[_n] = label; _rv[_n] = val; _ru[_n] = unit; _rd[_n] = dec; _last[_n] = NAN; _n++; }
    return *this;
  }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"kvb\">"));
    out.print(_joined());
    out.print(F("</div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override {
    bool ch = false;
    for (uint8_t i = 0; i < _n; i++) { float v = _rv[i] ? *_rv[i] : 0; if (v != _last[i]) { _last[i] = v; ch = true; } }
    return ch;
  }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_joined()); out += '"'; }
 private:
  String _joined() {
    String j;
    char b[16];
    for (uint8_t i = 0; i < _n; i++) {
      dtostrf(_rv[i] ? *_rv[i] : 0.0f, 0, _rd[i], b);
      j += "<div class='kv'><span>";
      j += _rl[i];
      j += "</span><span>";
      j += b;
      if (_ru[i] && _ru[i][0]) { j += ' '; j += _ru[i]; }
      j += "</span></div>";
    }
    return j;
  }
  const char* _rl[8];
  float* _rv[8];
  const char* _ru[8];
  uint8_t _rd[8];
  float _last[8];
  uint8_t _n = 0;
};

// ════════ radio / separator / tab (controls & layout) ════════

static const char RW_RADIO_CSS[] PROGMEM =
  ".seg{display:flex;gap:4px;background:var(--bg3);border-radius:12px;padding:4px}"
  ".seg button{flex:1;height:34px;border:none;border-radius:9px;background:transparent;color:var(--ink2);font:600 13px var(--font);cursor:pointer}"
  ".seg button.on{background:var(--grad);color:var(--acc-ink)}";
static const char RW_SEP_CSS[] PROGMEM =
  ".sep{grid-column:1/-1;display:flex;align-items:center;gap:12px;margin:10px 2px 2px;color:var(--ink3);font:700 11px var(--font);letter-spacing:.14em;text-transform:uppercase}"
  ".sep::after{content:'';flex:1;height:1px;background:var(--line)}";
static const char RW_TAB_CSS[] PROGMEM =
  ".tabbar{display:flex;gap:8px;margin:14px 0 4px;flex-wrap:wrap}"
  ".tabbtn{padding:9px 16px;border-radius:999px;border:1px solid var(--line2);background:transparent;color:var(--ink2);font:600 13px var(--font);cursor:pointer}"
  ".tabbtn.on{background:var(--grad);color:var(--acc-ink);border-color:transparent}"
  ".tabpanel{display:none}.tabpanel.on{display:grid}";
static const char RW_RADIO_JS[] PROGMEM =
  "R.W.radio={init:function(el){var b=el.querySelectorAll('button');for(var i=0;i<b.length;i++)(function(j){"
  "b[j].addEventListener('click',function(){R.send(el.dataset.key,j);});})(i);},"
  "update:function(el,v){var b=el.querySelectorAll('button');for(var i=0;i<b.length;i++)b[i].classList.toggle('on',i==v);}};";
static const char RW_TAB_JS[] PROGMEM =
  "R.W.tab={};document.addEventListener('DOMContentLoaded',function(){var b=document.querySelectorAll('.tabbtn');"
  "for(var i=0;i<b.length;i++)b[i].addEventListener('click',function(){var t=this.getAttribute('data-tab');"
  "var a=document.querySelectorAll('.tabbtn');for(var j=0;j<a.length;j++)a[j].classList.toggle('on',a[j].getAttribute('data-tab')===t);"
  "var p=document.querySelectorAll('.tabpanel');for(var j=0;j<p.length;j++)p[j].classList.toggle('on',p[j].getAttribute('data-panel')===t);});});";

// ── Control: segmented radio (CSV options → bound index) ──
class RadioWidget : public Widget {
 public:
  using Cb = std::function<void(int)>;
  RadioWidget(const char* key, const char* title, const char* options, int* idx, Cb cb)
      : Widget(key, title), _opts(options), _idx(idx), _cb(cb) {}
  const char* typeId() const override { return "radio"; }
  const char* css() const override { return RW_RADIO_CSS; }
  const char* js() const override { return RW_RADIO_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"seg\">"));
    int cur = _idx ? *_idx : 0, i = 0;
    for (const char* p = _opts; p && *p;) {
      const char* q = p;
      while (*q && *q != ',') q++;
      out.print(F("<button"));
      if (i == cur) out.print(F(" class=\"on\""));
      out.print('>');
      for (const char* c = p; c < q; c++) out.print(*c);
      out.print(F("</button>"));
      i++;
      p = (*q) ? q + 1 : q;
    }
    out.print(F("</div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { int v = _idx ? *_idx : 0; if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_idx ? *_idx : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_idx) *_idx = n; if (_cb) _cb(n); }
 private:
  const char* _opts; int* _idx; Cb _cb; int _last = 0; bool _seen = false;
};

// ── Layout: labelled divider (spans the grid) ──
class SeparatorWidget : public Widget {
 public:
  explicit SeparatorWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "separator"; }
  const char* css() const override { return RW_SEP_CSS; }
  void card(Print& out) override { out.print(F("<div class=\"sep\">")); out.print(_title); out.print(F("</div>")); }
};

// ── Layout: tab boundary (widgets after it, until the next tab, form a panel). Rendered by RisalUI. ──
class TabWidget : public Widget {
 public:
  explicit TabWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "tab"; }
  const char* css() const override { return RW_TAB_CSS; }
  const char* js() const override { return RW_TAB_JS; }
  void card(Print&) override {}  // handled by RisalUI::_handleRoot
};

// ── Display: AI note plate (a short assistant message bound to a String) ──
static const char RW_AI_CSS[] PROGMEM =
  ".ai{display:flex;gap:11px;align-items:flex-start}"
  ".ai .ic{flex:none;width:30px;height:30px;border-radius:9px;display:grid;place-items:center;color:var(--acc-ink);background:var(--grad)}"
  ".ai p{font-size:13px;line-height:1.5;color:var(--ink2);margin:0}";
static const char RW_AI_JS[] PROGMEM =
  "R.W.ai={update:function(el,v){var p=el.querySelector('.ai p');if(p)p.textContent=v;}};";

class AiWidget : public Widget {
 public:
  AiWidget(const char* key, const char* title, String* note) : Widget(key, title), _note(note) {}
  const char* typeId() const override { return "ai"; }
  const char* css() const override { return RW_AI_CSS; }
  const char* js() const override { return RW_AI_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"ai\"><span class=\"ic\"><svg viewBox=\"0 0 24 24\" width=\"17\" height=\"17\" fill=\"currentColor\">"
                "<path d=\"M12 2l1.8 5.2L19 9l-5.2 1.8L12 16l-1.8-5.2L5 9z\"/></svg></span><p>"));
    if (_note) out.print(*_note);
    out.print(F("</p></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _note ? *_note : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_note ? *_note : String()); out += '"'; }
 private:
  String* _note; String _last; bool _seen = false;
};

// ── Control: date picker — custom calendar popover (no native input) ──
static const char RW_DATE_CSS[] PROGMEM =
  ".dpw{position:relative}"
  ".dpf{display:flex;align-items:center;justify-content:space-between;width:100%;height:42px;border-radius:12px;"
  "border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px;cursor:pointer}"
  ".dpf b{font-weight:500;color:var(--ink1)}"
  ".dpf svg{width:16px;height:16px;color:var(--ink3);fill:none;stroke:currentColor;stroke-width:2}"
  ".dpp{position:absolute;z-index:120;inset-block-start:48px;inset-inline-start:0;width:248px;background:var(--bg2);"
  "border:1px solid var(--line2);border-radius:14px;padding:12px;box-shadow:0 16px 40px rgba(0,0,0,.45);display:none}"
  ".dpp.open{display:block}.dpp.up{inset-block-start:auto;inset-block-end:48px}"
  ".dph{display:flex;align-items:center;justify-content:space-between;margin-bottom:8px}"
  ".dph b{font:700 13px var(--font)}"
  ".dph button{width:28px;height:28px;border:none;border-radius:8px;background:var(--bg3);color:var(--ink2);cursor:pointer;font-size:15px}"
  ".dpg{display:grid;grid-template-columns:repeat(7,1fr);gap:2px}"
  ".dpg i{text-align:center;font:600 10px var(--font);color:var(--ink3);padding:4px 0;font-style:normal}"
  ".dpg button{height:30px;border:none;border-radius:8px;background:transparent;color:var(--ink1);font:13px var(--font);cursor:pointer}"
  ".dpg button:hover{background:var(--bg3)}.dpg button.sel{background:var(--grad);color:var(--acc-ink);font-weight:700}";
static const char RW_DATE_JS[] PROGMEM =
  "R.W.date={"
  "init:function(el){var f=el.querySelector('.dpf'),p=el.querySelector('.dpp');"
  "var t=el.querySelector('.dpf b').textContent;el._cur=/^\\d{4}-\\d\\d-\\d\\d$/.test(t)?t:'';"
  "var d=el._cur?new Date(el._cur):new Date();el._vy=d.getFullYear();el._vm=d.getMonth();"
  "f.addEventListener('click',function(e){e.stopPropagation();var o=p.classList.toggle('open');"
  "if(o){var r=f.getBoundingClientRect();p.classList.toggle('up',r.bottom>innerHeight-280);R.W.date.draw(el);}});"
  "p.addEventListener('click',function(e){e.stopPropagation();});"
  "document.addEventListener('click',function(){p.classList.remove('open');});},"
  "draw:function(el){var p=el.querySelector('.dpp'),y=el._vy,m=el._vm;"
  "var first=new Date(y,m,1).getDay(),dim=new Date(y,m+1,0).getDate();"
  "var mn='Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec'.split(' ')[m];"
  "var h='<div class=\"dph\"><button data-n=\"-1\">\\u2039</button><b>'+mn+' '+y+'</b><button data-n=\"1\">\\u203a</button></div>';"
  "h+='<div class=\"dpg\"><i>S</i><i>M</i><i>T</i><i>W</i><i>T</i><i>F</i><i>S</i>';"
  "for(var i=0;i<first;i++)h+='<button disabled></button>';"
  "for(var dd=1;dd<=dim;dd++){var ds=y+'-'+String(m+1).padStart(2,'0')+'-'+String(dd).padStart(2,'0');"
  "h+='<button data-d=\"'+ds+'\"'+(ds===el._cur?' class=\"sel\"':'')+'>'+dd+'</button>';}"
  "h+='</div>';p.innerHTML=h;"
  "p.querySelectorAll('.dph button').forEach(function(b){b.addEventListener('click',function(e){e.stopPropagation();"
  "el._vm+=+b.dataset.n;if(el._vm<0){el._vm=11;el._vy--;}if(el._vm>11){el._vm=0;el._vy++;}R.W.date.draw(el);});});"
  "p.querySelectorAll('.dpg button[data-d]').forEach(function(b){b.addEventListener('click',function(e){e.stopPropagation();"
  "el._cur=b.dataset.d;el.querySelector('.dpf b').textContent=b.dataset.d;p.classList.remove('open');R.send(el.dataset.key,b.dataset.d);});});},"
  "update:function(el,v){el._cur=v;var b=el.querySelector('.dpf b');if(b)b.textContent=v;"
  "var d=v?new Date(v):new Date();el._vy=d.getFullYear();el._vm=d.getMonth();}};";

class DateWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  DateWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "date"; }
  const char* css() const override { return RW_DATE_CSS; }
  const char* js() const override { return RW_DATE_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"dpw\"><div class=\"dpf\"><b>"));
    out.print(_val && _val->length() ? *_val : String("Pick a date"));
    out.print(F("</b><svg viewBox=\"0 0 24 24\"><rect x=\"3\" y=\"4\" width=\"18\" height=\"17\" rx=\"2\"/>"
                "<path d=\"M3 9h18M8 2v4M16 2v4\"/></svg></div><div class=\"dpp\"></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { String v = _val ? *_val : String(); if (!_seen || v != _last) { _seen = true; _last = v; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; String _last; bool _seen = false;
};
