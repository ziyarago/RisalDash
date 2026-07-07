#pragma once
#include "base.h"

// Interactive controls (write over the WebSocket): toggle, slider, button, number,
// select, radio, terminal.

static const char RW_TOGGLE_CSS[] PROGMEM =
  ".tg{display:flex;align-items:center;justify-content:space-between}"
  ".tg span{font-size:15px;color:var(--ink2)}"
  ".sw{width:46px;height:26px;border-radius:99px;background:var(--bg3);border:1px solid var(--line2);position:relative;cursor:pointer}"
  ".sw::after{content:'';position:absolute;top:2px;inset-inline-start:2px;width:20px;height:20px;border-radius:50%;background:#fff;transition:transform .25s}"
  ".sw.on{background:var(--grad);border-color:transparent}.sw.on::after{transform:translateX(20px)}";

static const char RW_TOGGLE_JS[] PROGMEM =
  "R.W.toggle={init:function(el){var s=el.querySelector('.sw');"
  "if(s)s.addEventListener('click',function(){R.send(el.dataset.key,!s.classList.contains('on'));});},"
  "update:function(el,v){var s=el.querySelector('.sw');if(s)s.classList.toggle('on',!!v);"
  "var t=el.querySelector('.tg span');if(t)t.textContent=v?R.L.on:R.L.off;}};";

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
    out.print(rwOnOff(on));
    out.print(F("</span><div class=\"sw"));
    if (on) out.print(F(" on"));
    out.print(F("\"></div></div>"));
    cardClose(out);
  }

  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val && *_val); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += ((_val && *_val) ? "true" : "false"); }
  // Leave the tracker unchanged so the next poll() rebroadcasts the new state to every client.
  void applyCommand(const String& v) override {
    bool b = (v == "true" || v == "1");
    if (_val) *_val = b;
    if (_cb) _cb(b);
  }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"toggle\"}"));
    return true;
  }

 private:
  bool* _val;
  Cb _cb;
  RwTracked<bool> _trk;
};

static const char RW_SLIDER_CSS[] PROGMEM =
  "input[type=range]{width:100%;-webkit-appearance:none;appearance:none;height:6px;border-radius:99px;background:var(--bg3);outline:none}"
  "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:var(--acc);cursor:pointer;border:3px solid var(--bg2)}"
  "input[type=range]::-moz-range-thumb{width:15px;height:15px;border-radius:50%;background:var(--acc);border:3px solid var(--bg2);cursor:pointer}";
static const char RW_BUTTON_CSS[] PROGMEM =
  ".act{height:46px;border:none;border-radius:13px;background:var(--grad);color:var(--acc-ink);font:700 15px var(--font);cursor:pointer;width:100%}"
  ".act:active{transform:scale(.97)}";

static const char RW_SLIDER_JS[] PROGMEM =
  "R.W.slider={init:function(el){var i=el.querySelector('input');if(!i)return;"
  "i.addEventListener('input',function(){var b=el.querySelector('.big');if(b)b.textContent=i.value;});"
  "i.addEventListener('change',function(){R.send(el.dataset.key,+i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;var b=el.querySelector('.big');if(b)b.textContent=v;}};";
static const char RW_BUTTON_JS[] PROGMEM =
  "R.W.button={init:function(el){var b=el.querySelector('.act');if(b)b.addEventListener('click',function(){R.send(el.dataset.key,true);});}};";

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
  bool poll() override { return _trk.changed(_val ? *_val : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_val) *_val = n; if (_cb) _cb(n); }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"range\",\"lo\":")); out.print(_lo); out.print(F(",\"hi\":")); out.print(_hi); out.print('}');
    return true;
  }
 private:
  int* _val; int _lo, _hi; Cb _cb; RwTracked<int> _trk;
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
    out.print(rI18n(_label ? _label : _title));
    out.print(F("</button>"));
    cardClose(out);
  }
  void applyCommand(const String& v) override { (void)v; if (_cb) _cb(); }
 private:
  const char* _label; Cb _cb;
};

static const char RW_NUMBER_CSS[] PROGMEM =
  ".ninp{width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:16px var(--font);padding:0 12px}";
static const char RW_SELECT_CSS[] PROGMEM =
  // Fully custom dropdown so the option list matches the theme (a native <select> popup can't be
  // styled). .rsel is the trigger; .rsel-list overlays on .open. Chevron rotates; RTL flips sides.
  ".rsel{position:relative;min-height:42px;border-radius:12px;border:1px solid var(--line2);"
  "background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 40px 0 13px;display:flex;"
  "align-items:center;cursor:pointer;user-select:none;outline:none}"
  ".rsel:focus,.rsel.open{border-color:var(--acc)}"
  ".rsel-cur{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}"
  ".rsel-chev{position:absolute;right:13px;top:50%;margin-top:-9px;width:18px;height:18px;fill:none;"
  "stroke:#7c8699;stroke-width:2.2;stroke-linecap:round;stroke-linejoin:round;transition:transform .2s;"
  "pointer-events:none}"
  ".rsel.open .rsel-chev{transform:rotate(180deg)}"
  ".rsel-list{position:absolute;top:calc(100% + 6px);left:0;right:0;z-index:50;margin:0;padding:5px;"
  "list-style:none;background:oklch(0.23 0.025 255);border:1px solid var(--line2);border-radius:12px;"
  "box-shadow:0 18px 44px oklch(0 0 0 / .5);max-height:min(320px,62vh);overflow:auto;display:none}"
  ".rsel.open .rsel-list{display:block}"
  ".rsel-opt{padding:9px 12px;border-radius:8px;font:14.5px var(--font);color:var(--ink1);"
  "white-space:nowrap;overflow:hidden;text-overflow:ellipsis;cursor:pointer}"
  ".rsel-opt:hover{background:oklch(0.7 0.03 255 / .14)}"
  ".rsel-opt.on{background:var(--grad);color:var(--acc-ink);font-weight:700}"
  ".light .rsel-list{background:oklch(0.99 0.004 255)}"
  "[dir=rtl] .rsel{padding:0 13px 0 40px}"
  "[dir=rtl] .rsel-chev{right:auto;left:13px}";
static const char RW_NUMBER_JS[] PROGMEM =
  "R.W.number={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,+i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_SELECT_JS[] PROGMEM =
  "R.W.select={init:function(el){var box=el.querySelector('.rsel');if(!box)return;"
  "var cur=box.querySelector('.rsel-cur'),opts=box.querySelectorAll('.rsel-opt');"
  // Cards have backdrop-filter -> each is its own stacking context, so the popup can't paint over
  // sibling cards from inside. Lift the whole card while open (46: above the fixed footer, below the
  // sticky appbar so it never bleeds over the header on scroll).
  "function open(o){box.classList.toggle('open',o);el.style.zIndex=o?'46':'';}"
  "box.addEventListener('click',function(e){var o=e.target.closest('.rsel-opt');"
  "if(o){opts.forEach(function(x){x.classList.remove('on');});o.classList.add('on');"
  "cur.textContent=o.textContent;open(false);R.send(el.dataset.key,+o.dataset.i);}"
  "else{open(!box.classList.contains('open'));}});"
  "document.addEventListener('click',function(e){if(!box.contains(e.target))open(false);});},"
  "update:function(el,v){var box=el.querySelector('.rsel');if(!box)return;"
  "box.querySelectorAll('.rsel-opt').forEach(function(o){var on=+o.dataset.i===+v;o.classList.toggle('on',on);"
  "if(on)box.querySelector('.rsel-cur').textContent=o.textContent;});}};";

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
  bool poll() override { return _trk.changed(_val ? *_val : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_val ? *_val : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_val) *_val = n; if (_cb) _cb(n); }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"number\",\"lo\":")); out.print(_lo); out.print(F(",\"hi\":")); out.print(_hi); out.print('}');
    return true;
  }
 private:
  int* _val; int _lo, _hi, _step; Cb _cb; RwTracked<int> _trk;
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
    int cur = _idx ? *_idx : 0;
    // Custom dropdown (not a native <select>) so the option list matches the UI instead of the OS
    // popup. The trigger shows the current label; the list overlays on click. JS keeps it in sync.
    // Options are author strings, so run the whole comma-list through the translator (keep order/count).
    const char* opts = rI18n(_opts);
    out.print(F("<div class=\"rsel\" tabindex=\"0\"><span class=\"rsel-cur\">"));
    rwCsvEach(opts, [&](const char* a, const char* b, int i) {  // print the current label
      if (i == cur) while (a < b) out.print(*a++);
    });
    out.print(F("</span><svg class=\"rsel-chev\" viewBox=\"0 0 24 24\"><path d=\"M6 9l6 6 6-6\"/></svg>"
                "<ul class=\"rsel-list\">"));
    rwCsvEach(opts, [&](const char* a, const char* b, int i) {
      out.print(F("<li class=\"rsel-opt"));
      if (i == cur) out.print(F(" on"));
      out.print(F("\" data-i=\""));
      out.print(i);
      out.print(F("\">"));
      while (a < b) out.print(*a++);
      out.print(F("</li>"));
    });
    out.print(F("</ul></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_idx ? *_idx : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_idx ? *_idx : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_idx) *_idx = n; if (_cb) _cb(n); }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"select\",\"opts\":\"")); out.print(rI18n(_opts)); out.print(F("\"}"));
    return true;
  }
 private:
  const char* _opts; int* _idx; Cb _cb; RwTracked<int> _trk;
};

static const char RW_RADIO_CSS[] PROGMEM =
  ".seg{display:flex;gap:4px;background:var(--bg3);border-radius:12px;padding:4px}"
  ".seg button{flex:1;height:34px;border:none;border-radius:9px;background:transparent;color:var(--ink2);font:600 13px var(--font);cursor:pointer}"
  ".seg button.on{background:var(--grad);color:var(--acc-ink)}";

static const char RW_RADIO_JS[] PROGMEM =
  "R.W.radio={init:function(el){var b=el.querySelectorAll('button');for(var i=0;i<b.length;i++)(function(j){"
  "b[j].addEventListener('click',function(){R.send(el.dataset.key,j);});})(i);},"
  "update:function(el,v){var b=el.querySelectorAll('button');for(var i=0;i<b.length;i++)b[i].classList.toggle('on',i==v);}};";

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
    int cur = _idx ? *_idx : 0;
    rwCsvEach(rI18n(_opts), [&](const char* a, const char* b, int i) {  // options are author strings -> translate
      out.print(F("<button"));
      if (i == cur) out.print(F(" class=\"on\""));
      out.print('>');
      while (a < b) out.print(*a++);
      out.print(F("</button>"));
    });
    out.print(F("</div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_idx ? *_idx : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_idx ? *_idx : 0); }
  void applyCommand(const String& v) override { int n = v.toInt(); if (_idx) *_idx = n; if (_cb) _cb(n); }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"select\",\"opts\":\"")); out.print(rI18n(_opts)); out.print(F("\"}"));
    return true;
  }
 private:
  const char* _opts; int* _idx; Cb _cb; RwTracked<int> _trk;
};

// ── Control: terminal / console — a scrolling output + a command input (over the WebSocket) ──
static const char RW_TERM_CSS[] PROGMEM =
  ".rterm{background:#0a0f18;border:1px solid var(--line2);border-radius:12px;overflow:hidden;font-family:ui-monospace,SFMono-Regular,Menlo,monospace}"
  ".rterm-o{height:150px;overflow:auto;padding:10px 12px;font-size:12.5px;line-height:1.55;color:#7ee0b0;white-space:pre-wrap;word-break:break-word}"
  ".rterm-in{display:flex;align-items:center;gap:7px;padding:8px 12px;border-top:1px solid var(--line2);color:#7ee0b0}"
  ".rterm-i{flex:1;background:transparent;border:none;outline:none;color:#e6fff2;font:12.5px ui-monospace,monospace}";
static const char RW_TERM_JS[] PROGMEM =
  "R.W.term={init:function(el){var i=el.querySelector('.rterm-i');if(i)i.addEventListener('keydown',function(e){"
  "if(e.key==='Enter'&&i.value){R.send(el.dataset.key,i.value);i.value='';}});},"
  "update:function(el,v){var o=el.querySelector('.rterm-o');if(o){o.innerHTML=v;o.scrollTop=o.scrollHeight;}}};";
class TerminalWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  TerminalWidget(const char* key, const char* title, Cb cb) : Widget(key, title), _cb(cb) {}
  const char* typeId() const override { return "term"; }
  const char* css() const override { return RW_TERM_CSS; }
  const char* js() const override { return RW_TERM_JS; }
  TerminalWidget& print(const String& line) {  // append output
    for (int i = 7; i > 0; i--) _arr[i] = _arr[i - 1];
    _arr[0] = line;
    if (_n < 8) _n++;
    _dirty = true;
    return *this;
  }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rterm\"><div class=\"rterm-o\">"));
    out.print(_joined());
    out.print(F("</div><div class=\"rterm-in\"><span>&gt;</span>"
                "<input class=\"rterm-i\" placeholder=\"command\" autocomplete=\"off\"></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { if (_dirty) { _dirty = false; return true; } return false; }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_joined()); out += '"'; }
  void applyCommand(const String& v) override { print(String("> ") + v); if (_cb) _cb(v); }  // echo + handle
 private:
  String _joined() {
    String j;
    for (int i = _n - 1; i >= 0; i--) { if (j.length()) j += "<br>"; j += _arr[i]; }  // oldest -> newest
    return j;
  }
  Cb _cb;
  String _arr[8];
  uint8_t _n = 0;
  bool _dirty = false;
};
