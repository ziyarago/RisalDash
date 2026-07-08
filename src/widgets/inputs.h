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

// Custom themed popovers — a native <input type=time|color> opens the OS picker, which
// clashes with the dashboard look (same reasoning as the custom select and date widgets).
static const char RW_TIME_CSS[] PROGMEM =
  ".tpw{position:relative}"
  ".tpf{display:flex;align-items:center;justify-content:space-between;width:100%;height:42px;border-radius:12px;"
  "border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px;cursor:pointer}"
  ".tpf b{font-weight:500}"
  ".tpf svg{width:16px;height:16px;color:var(--ink3);fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round}"
  ".tpp{position:absolute;z-index:120;inset-block-start:48px;inset-inline-start:0;display:none;gap:4px;width:176px;"
  "background:var(--bg2);border:1px solid var(--line2);border-radius:14px;padding:8px;box-shadow:0 16px 40px rgba(0,0,0,.45)}"
  ".tpp.open{display:flex}.tpp.up{inset-block-start:auto;inset-block-end:48px}"
  ".tpp ul{flex:1;margin:0;padding:0;list-style:none;max-height:196px;overflow:auto;scrollbar-width:none}"
  ".tpp ul::-webkit-scrollbar{display:none}"
  ".tpp li{text-align:center;padding:7px 0;border-radius:8px;font:14px var(--font);cursor:pointer}"
  ".tpp li:hover{background:var(--bg3)}"
  ".tpp li.on{background:var(--grad);color:var(--acc-ink);font-weight:700}";
static const char RW_COLOR_CSS[] PROGMEM =
  ".cpw{position:relative}"
  ".cpf{display:flex;align-items:center;gap:10px;width:100%;height:42px;border-radius:12px;border:1px solid var(--line2);"
  "background:var(--field);padding:0 12px;cursor:pointer}"
  ".cpf i{width:22px;height:22px;border-radius:8px;box-shadow:inset 0 0 0 1px oklch(1 0 0/.25)}"
  ".cpf b{font:600 14px var(--mono);color:var(--ink2)}"
  ".cpp{position:absolute;z-index:120;inset-block-start:48px;inset-inline-start:0;display:none;"
  "grid-template-columns:repeat(4,28px);gap:9px;background:var(--bg2);border:1px solid var(--line2);border-radius:14px;"
  "padding:12px;box-shadow:0 16px 40px rgba(0,0,0,.45)}"
  ".cpp.open{display:grid}.cpp.up{inset-block-start:auto;inset-block-end:48px}"
  ".cpp i{width:28px;height:28px;border-radius:9px;cursor:pointer;box-shadow:inset 0 0 0 1px oklch(1 0 0/.2)}"
  ".cpp i.on{outline:2px solid var(--ink1);outline-offset:2px}";

static const char RW_PASSWORD_JS[] PROGMEM =
  "R.W.password={init:function(el){var i=el.querySelector('input');if(i)i.addEventListener('change',function(){R.send(el.dataset.key,i.value);});},"
  "update:function(el,v){var i=el.querySelector('input');if(i)i.value=v;}};";
static const char RW_TIME_JS[] PROGMEM = R"js(R.W.time={
init:function(el){var f=el.querySelector('.tpf'),p=el.querySelector('.tpp'),b=f.querySelector('b');
function pad(n){return (n<10?'0':'')+n;}
function cur(){var m=/^(\d\d):(\d\d)$/.exec(b.textContent);return m?[+m[1],+m[2]]:[8,0];}
function draw(){var c=cur(),h='<ul>',j;
for(j=0;j<24;j++)h+='<li data-h="'+j+'"'+(j===c[0]?' class="on"':'')+'>'+pad(j)+'</li>';
h+='</ul><ul>';for(j=0;j<60;j++)h+='<li data-m="'+j+'"'+(j===c[1]?' class="on"':'')+'>'+pad(j)+'</li>';
p.innerHTML=h+'</ul>';
p.querySelectorAll('li.on').forEach(function(li){var u=li.parentNode;u.scrollTop=li.offsetTop-u.clientHeight/2+li.clientHeight/2;});}
function open(o){p.classList.toggle('open',o);el.style.zIndex=o?'46':'';}
f.addEventListener('click',function(e){e.stopPropagation();var o=!p.classList.contains('open');
document.body.click();
open(o);if(o){var r=f.getBoundingClientRect();p.classList.toggle('up',r.bottom>innerHeight-236);draw();}});
p.addEventListener('click',function(e){e.stopPropagation();var t=e.target;if(t.tagName!=='LI')return;var c=cur();
if(t.dataset.h!=null){b.textContent=pad(+t.dataset.h)+':'+pad(c[1]);draw();}
else{b.textContent=pad(c[0])+':'+pad(+t.dataset.m);open(false);}
R.send(el.dataset.key,b.textContent);});
document.addEventListener('click',function(){open(false);});},
update:function(el,v){var b=el.querySelector('.tpf b');if(b)b.textContent=v;}
};)js";
static const char RW_COLOR_JS[] PROGMEM = R"js(R.W.color={
PAL:['#ff5c5c','#ff8c42','#ffd24a','#a3e635','#4ade80','#2ddebe','#22d3ee','#60a5fa','#818cf8','#a78bfa','#f472b6','#ff5c8a','#ffffff','#ffd9a0','#94a3b8','#1e293b'],
init:function(el){var f=el.querySelector('.cpf'),p=el.querySelector('.cpp'),sw=f.querySelector('i'),hx=f.querySelector('b');
function draw(){p.innerHTML=R.W.color.PAL.map(function(c){return '<i data-c="'+c+'"'+(c===hx.textContent?' class="on"':'')+' style="background:'+c+'"></i>';}).join('');}
function open(o){p.classList.toggle('open',o);el.style.zIndex=o?'46':'';}
f.addEventListener('click',function(e){e.stopPropagation();var o=!p.classList.contains('open');
document.body.click();
if(o){var r=f.getBoundingClientRect();p.classList.toggle('up',r.bottom>innerHeight-190);draw();}open(o);});
p.addEventListener('click',function(e){e.stopPropagation();var t=e.target.closest('i');if(!t)return;
hx.textContent=t.dataset.c;sw.style.background=t.dataset.c;open(false);R.send(el.dataset.key,t.dataset.c);});
document.addEventListener('click',function(){open(false);});},
update:function(el,v){var sw=el.querySelector('.cpf i'),hx=el.querySelector('.cpf b');if(sw)sw.style.background=v;if(hx)hx.textContent=v;}
};)js";

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
  const char* css() const override { return RW_TIME_CSS; }
  const char* js() const override { return RW_TIME_JS; }
  void card(Print& out) override {
    cardOpen(out);
    // Themed trigger + hour/minute popover (built by JS) — no native OS time picker.
    out.print(F("<div class=\"tpw\"><div class=\"tpf\"><b>"));
    if (_val && _val->length()) rwAttr(out, *_val);
    else out.print(F("--:--"));
    out.print(F("</b><svg viewBox=\"0 0 24 24\"><circle cx=\"12\" cy=\"12\" r=\"9\"/>"
                "<path d=\"M12 7v5l3 3\"/></svg></div><div class=\"tpp\"></div></div>"));
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
    // Current swatch + hex; a themed swatch-grid popover (built by JS) replaces the OS dialog.
    String hex = _val && _val->length() ? *_val : String("#22d3ee");
    out.print(F("<div class=\"cpw\"><div class=\"cpf\"><i style=\"background:"));
    rwAttr(out, hex);
    out.print(F("\"></i><b>"));
    out.print(hex);
    out.print(F("</b></div><div class=\"cpp\"></div></div>"));
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
  "f.addEventListener('click',function(e){e.stopPropagation();var o=!p.classList.contains('open');"
  "document.body.click();p.classList.toggle('open',o);el.style.zIndex=o?'46':'';"
  "if(o){var r=f.getBoundingClientRect();p.classList.toggle('up',r.bottom>innerHeight-280);R.W.date.draw(el);}});"
  "p.addEventListener('click',function(e){e.stopPropagation();});"
  "document.addEventListener('click',function(){p.classList.remove('open');el.style.zIndex='';});},"
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
