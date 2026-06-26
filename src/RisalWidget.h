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
  "var t=el.querySelector('.tg span');if(t)t.textContent=v?'On':'Off';}};";

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

 protected:
  virtual void extraAttrs(Print& out) { (void)out; }    // extra data-* on the card
  void cardOpen(Print& out) {
    out.print(F("<section class=\"card\" data-type=\""));
    out.print(typeId());
    out.print(F("\" data-key=\""));
    out.print(_key);
    out.print('"');
    extraAttrs(out);
    out.print(F("><h3><i class=\"eb\"></i>"));
    out.print(_title);
    out.print(F("</h3>"));
  }
  void cardClose(Print& out) { out.print(F("</section>")); }

  const char* _key;
  const char* _title;
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
  ".spark .fl{fill:url(#rgf);stroke:none}";

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
  "var b=el.querySelector('.led b');if(b)b.textContent=v?'on':'off';}};";
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
  "var F=el.querySelector('.fl');if(F)F.setAttribute('d',ln+' L'+w+' '+ht+' L0 '+ht+' Z');}};";

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
                "<path class=\"fl\"></path><path class=\"ln\"></path></svg>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { float v = _val ? *_val : 0; if (v != _last) { _last = v; return true; } return false; }
  void writeKV(String& out) override { char b[16]; dtostrf(_val ? *_val : 0.0f, 0, 1, b); out += '"'; out += _key; out += "\":"; out += b; }
 private:
  float* _val; const char* _unit; float _last = NAN;
};
