#pragma once
#include "base.h"

// String-valued input controls: text, textarea, password, time, date, color.

static const char RW_TEXT_CSS[] PROGMEM =
  ".tinp{width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px}";

static const char RW_TEXT_JS[] PROGMEM =
  "R.W.text={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_TEXTAREA_CSS[] PROGMEM =
  ".tarea{width:100%;min-height:92px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:14.5px/1.5 var(--font);padding:10px 12px;resize:vertical;box-sizing:border-box}";
static const char RW_TEXTAREA_JS[] PROGMEM =
  "R.W.textarea={init:function(el){var t=el.querySelector('textarea');if(t)t.addEventListener('change',function(){R.send(el.dataset.key,t.value);});},"
  "update:function(el,v){var t=el.querySelector('textarea');if(t&&document.activeElement!==t)t.value=v;}};";

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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
};

// ── Control: multi-line text input (bound to a String) ──
class TextareaWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  TextareaWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "textarea"; }
  const char* css() const override { return RW_TEXTAREA_CSS; }
  const char* js() const override { return RW_TEXTAREA_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<textarea class=\"tarea\" rows=\"4\">"));
    if (_val) rwAttr(out, *_val);  // element content, still needs &/</> escaping
    out.print(F("</textarea>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
};

static const char RW_COLOR_CSS[] PROGMEM =
  ".col{display:flex;align-items:center;gap:12px}.col input{width:40px;height:40px;border:none;border-radius:10px;background:none;cursor:pointer}"
  ".col b{font:600 14px var(--mono);color:var(--ink2)}";

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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
};

// ── Control: color picker (String hex "#rrggbb") ──
class ColorWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  ColorWidget(const char* key, const char* title, String* val, Cb cb) : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "color"; }
  const char* css() const override { return RW_COLOR_CSS; }
  const char* js() const override { return RW_COLOR_JS; }
  bool writeGearMeta(Print& out) override {
    out.print(F("{\"n\":\"")); out.print(rI18n(_title)); out.print(F("\",\"k\":\"")); out.print(_key);
    out.print(F("\",\"t\":\"color\"}"));
    return true;
  }
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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
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
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"'; }
  void applyCommand(const String& v) override { if (_val) *_val = v; if (_cb) _cb(v); }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
};
