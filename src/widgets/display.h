#pragma once
#include "base.h"

// Read-only display widgets: metric, gauge, stat, progress, badge, led, chart, label,
// log, image, table, ai.

// ── Per-type CSS (single TU: included only by RisalUI.cpp) ──
// ring/semi centre the value over the SVG (grid + absolute); the bar variant is plain flow —
// value line above the track, no overrides to lose (robust even if a preview drops a rule).
static const char RW_GAUGE_CSS[] PROGMEM =
  ".gauge{position:relative;padding:4px 0}"
  ".gauge.ring,.gauge.semi{display:grid;place-items:center}"
  ".gauge.ring svg{transform:rotate(-90deg)}"
  ".gauge .gv{font:800 22px/1 var(--font);font-variant-numeric:tabular-nums}"
  ".gauge.ring .gv,.gauge.semi .gv{position:absolute;text-align:center}"
  ".gauge .gv .unit{font-size:11px}"
  ".gauge.semi .gv{top:auto;bottom:16px}"
  ".gauge.lin{padding:10px 2px}"      /* variant class is `lin`, not `bar` — the global .bar (metric track) would restyle the container */
  ".gauge.lin .gv{margin-bottom:9px}"
  ".gauge .gbar{width:100%;height:10px;border-radius:6px;background:var(--bg3);overflow:hidden}"
  ".gauge .gbar>i{display:block;height:100%;background:var(--grad);border-radius:6px;transition:width .4s}";

static const char RW_METRIC_JS[] PROGMEM =
  "R.W.metric={update:function(el,v){el.querySelector('.big').textContent=v;"
  "var b=el.querySelector('.bar>i');if(b)b.style.width=Math.max(0,Math.min(100,v))+'%';}};";

static const char RW_GAUGE_JS[] PROGMEM =
  "R.W.gauge={update:function(el,v){var lo=+el.dataset.min,hi=+el.dataset.max,"
  "f=Math.max(0,Math.min(1,(v-lo)/((hi-lo)||1)));"
  "var a=el.querySelector('.arc');if(a)a.style.strokeDashoffset=100*(1-f);"       // ring + semi (pathLength=100)
  "var bar=el.querySelector('.gbar>i');if(bar)bar.style.width=(f*100)+'%';"        // bar variant
  "var b=el.querySelector('.gv b');if(b)b.textContent=(+v).toFixed(1);}};";

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
  bool poll() override { return _trk.changed(_val ? *_val : 0.0f); }
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
  RwTracked<float> _trk;
};

// ── Display: circular gauge ──
class GaugeWidget : public Widget {
 public:
  GaugeWidget(const char* key, const char* title, float* val, float mn, float mx, const char* unit)
      : Widget(key, title), _val(val), _lo(mn), _hi(mx), _unit(unit) {}
  const char* typeId() const override { return "gauge"; }
  const char* css() const override { return RW_GAUGE_CSS; }
  const char* js() const override { return RW_GAUGE_JS; }
  // Visual variant: "ring" (default, full circle) · "semi" (speedometer half-circle) · "bar" (linear).
  GaugeWidget& variant(const char* v) { _variant = v; return *this; }

  void card(Print& out) override {
    cardOpen(out);
    float v = _val ? *_val : 0.0f;
    float span = (_hi - _lo) != 0 ? (_hi - _lo) : 1;
    float f = (v - _lo) / span;
    f = f < 0 ? 0 : (f > 1 ? 1 : f);
    const bool bar = _variant[0] == 'b', semi = _variant[0] == 's';
    out.print(F("<div class=\"gauge "));
    out.print(bar ? "lin" : semi ? "semi" : "ring");  // `lin`, not `bar`: the core .bar (metric track) must not match this div
    out.print(F("\">"));
    if (bar) {
      out.print(F("<div class=\"gv\"><b>"));
      out.print(v, 1);
      out.print(F("</b> <span class=\"unit\">"));
      out.print(_unit ? _unit : "");
      out.print(F("</span></div><div class=\"gbar\"><i style=\"width:"));
      out.print(f * 100.0f, 0);
      out.print(F("%\"></i></div>"));
    } else {
      out.print(F("<svg width=\"116\" height=\"116\" viewBox=\"0 0 116 116\">"));
      if (semi) {  // top half-circle, endpoints on the horizontal diameter (unambiguous arc)
        out.print(F("<path d=\"M16 58 A42 42 0 0 1 100 58\" fill=\"none\" stroke=\"var(--bg3)\" stroke-width=\"9\" stroke-linecap=\"round\" pathLength=\"100\"/>"
                    "<path class=\"arc\" d=\"M16 58 A42 42 0 0 1 100 58\" fill=\"none\" stroke=\"url(#rg)\" stroke-width=\"9\" stroke-linecap=\"round\" pathLength=\"100\" stroke-dasharray=\"100\" stroke-dashoffset=\""));
      } else {  // full ring
        out.print(F("<circle cx=\"58\" cy=\"58\" r=\"42\" fill=\"none\" stroke=\"var(--bg3)\" stroke-width=\"9\" pathLength=\"100\"/>"
                    "<circle class=\"arc\" cx=\"58\" cy=\"58\" r=\"42\" fill=\"none\" stroke=\"url(#rg)\" stroke-width=\"9\" stroke-linecap=\"round\" pathLength=\"100\" stroke-dasharray=\"100\" stroke-dashoffset=\""));
      }
      out.print(100.0f * (1.0f - f), 1);
      out.print(F("\"/></svg><div class=\"gv\"><b>"));
      out.print(v, 1);
      out.print(F("</b> <span class=\"unit\">"));
      out.print(_unit ? _unit : "");
      out.print(F("</span></div>"));
    }
    out.print(F("</div>"));
    cardClose(out);
  }

  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val ? *_val : 0.0f); }
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
  const char* _variant = "ring";
  RwTracked<float> _trk;
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
    out.print(rI18n(_l[v]));  // .labels() strings are author text -> translate
    out.print(F("</span>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val ? *_val : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
 protected:
  void extraAttrs(Print& out) override {
    for (int i = 0; i < 3; i++) { out.print(F(" data-l")); out.print(i); out.print(F("=\"")); out.print(rI18n(_l[i])); out.print('"'); }
  }
 private:
  int* _val; const char* _l[3] = {"OK", "Warn", "Bad"}; RwTracked<int> _trk;
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
    out.print(rwOnOff(on));
    out.print(F("</b><i"));
    if (on) out.print(F(" class=\"on\""));
    out.print(F("></i></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val && *_val); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += ((_val && *_val) ? "true" : "false"); }
 private:
  bool* _val; RwTracked<bool> _trk;
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
  bool poll() override { return _trk.changed(_val ? *_val : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
 private:
  int* _val; const char* _unit; RwTracked<int> _trk;
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
  bool poll() override { return _trk.changed(_val ? *_val : 0.0f); }
  void writeKV(String& out) override { char b[16]; dtostrf(_val ? *_val : 0.0f, 0, _dec, b); out += '"'; out += _key; out += "\":"; out += b; }
 private:
  float* _val; const char* _unit; uint8_t _dec = 0; RwTracked<float> _trk;
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
  bool poll() override { return _trk.changed(_val ? *_val : 0.0f); }
  void writeKV(String& out) override { char b[16]; dtostrf(_val ? *_val : 0.0f, 0, 1, b); out += '"'; out += _key; out += "\":"; out += b; }
 private:
  float* _val; const char* _unit; RwTracked<float> _trk;
};

static const char RW_LABEL_CSS[] PROGMEM = ".lbl{font:600 16px var(--font);color:var(--ink1);word-break:break-word}";

static const char RW_LOG_CSS[] PROGMEM =
  ".log{font:11.5px/1.8 var(--mono);color:var(--ink3);background:var(--bg2);border:1px solid var(--line);border-radius:11px;padding:10px;height:96px;overflow:hidden}";
static const char RW_LABEL_JS[] PROGMEM =
  "R.W.label={update:function(el,v){var e=el.querySelector('.lbl');if(e)e.textContent=v;}};";

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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
 private:
  String* _val; RwTracked<String> _trk;
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

static const char RW_IMAGE_CSS[] PROGMEM =
  ".imgw{border-radius:12px;overflow:hidden;border:1px solid var(--line);aspect-ratio:16/10;background:var(--bg2)}"
  ".imgw img{width:100%;height:100%;object-fit:cover;display:block}";
static const char RW_TABLE_CSS[] PROGMEM =
  ".kv{display:flex;justify-content:space-between;font-size:13px;padding:7px 0;border-bottom:1px solid var(--line)}"
  ".kv:last-child{border-bottom:none}.kv span:first-child{color:var(--ink3)}"
  ".kv span:last-child{font-family:var(--mono);color:var(--ink2)}";

static const char RW_IMAGE_JS[] PROGMEM =
  "R.W.image={update:function(el,v){var i=el.querySelector('img');if(i&&v)i.src=v;}};";
static const char RW_TABLE_JS[] PROGMEM =
  "R.W.table={update:function(el,v){var e=el.querySelector('.kvb');if(e)e.innerHTML=v;}};";

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
  bool poll() override { return _trk.changed(_url ? *_url : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_url ? *_url : String()); out += '"'; }
 private:
  String* _url; RwTracked<String> _trk;
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
  bool poll() override { return _trk.changed(_note ? *_note : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_note ? *_note : String()); out += '"'; }
 private:
  String* _note; RwTracked<String> _trk;
};
