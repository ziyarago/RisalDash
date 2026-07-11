#pragma once
#include "base.h"

// ── Device card: one composite tile for a whole device — emoji icon, name, a status line (a live
// online/offline dot + a transport label + On/Off), a primary Power toggle, and an optional intensity
// slider. Replaces the old "a toggle here, a status LED there" fragmentation with a single card that a
// driver can drive. State rides one key as "power,online,level" (e.g. "1,1,64"); commands come back as
// "p1"/"p0" (toggle) or "l64" (slider).
static const char RW_DEVCARD_CSS[] PROGMEM =
  ".devcard{padding:14px 14px 13px}"
  ".dc-hd{display:flex;align-items:flex-start;justify-content:space-between}"
  ".dc-ic{flex:none;width:38px;height:38px;border-radius:11px;display:grid;place-items:center;font-size:19px;"
    "background:var(--field);border:1px solid var(--line2);transition:.25s}"
  ".devcard.on .dc-ic{background:var(--grad);border-color:transparent}"
  ".dc-nm{font:700 14px var(--font);color:var(--ink1);letter-spacing:-.01em;margin-top:12px;"
    "white-space:nowrap;overflow:hidden;text-overflow:ellipsis}"
  ".dc-sub{display:flex;align-items:center;gap:6px;margin-top:3px;font-size:11.5px;color:var(--ink2)}"
  ".dc-dot{width:6px;height:6px;border-radius:50%;background:var(--ink3);flex:none}"
  ".dc-dot.ok{background:oklch(0.78 0.15 152);box-shadow:0 0 8px oklch(0.78 0.15 152 / .8)}"
  ".dc-dot.bad{background:oklch(0.72 0.18 15);box-shadow:0 0 8px oklch(0.72 0.18 15 / .7)}"
  ".dc-sw{flex:none;width:42px;height:25px;border-radius:99px;border:none;background:var(--line2);"
    "padding:3px;cursor:pointer;transition:.22s}"
  ".dc-sw i{display:block;width:19px;height:19px;border-radius:50%;background:#fff;transition:transform .22s;box-shadow:0 1px 3px rgba(0,0,0,.4)}"
  ".devcard.on .dc-sw{background:var(--grad)}.devcard.on .dc-sw i{transform:translateX(17px)}"
  ".dc-lv{margin-top:14px}"
  ".dc-lvl{display:flex;justify-content:space-between;font:700 10px var(--font);letter-spacing:.1em;text-transform:uppercase;color:var(--ink3)}"
  ".dc-lv input{width:100%;margin-top:8px;accent-color:var(--acc);height:6px}";

static const char RW_DEVCARD_JS[] PROGMEM =
  "R.W.devcard={"
  "c:function(e){return e.closest('.devcard');},"
  "tog:function(b){var c=this.c(b),on=!c.classList.contains('on');c.classList.toggle('on',on);"
  "c.querySelector('.dc-state').textContent=on?c.dataset.on:c.dataset.off;R.send(c.dataset.key,on?'p1':'p0');},"
  "lv:function(i){var c=this.c(i);c.querySelector('.dc-lvv').textContent=i.value+'%';R.send(c.dataset.key,'l'+i.value);},"
  "update:function(el,v){var a=(''+v).split(',');var on=a[0]==='1';"
  "el.classList.toggle('on',on);var st=el.querySelector('.dc-state');if(st)st.textContent=on?el.dataset.on:el.dataset.off;"
  "var d=el.querySelector('.dc-dot');if(d)d.className='dc-dot '+(a[1]==='1'?'ok':(a[1]==='0'?'bad':''));"
  "if(a[2]!==undefined&&a[2]!=='-1'){var r=el.querySelector('input[type=range]');"
  "if(r&&document.activeElement!==r)r.value=a[2];var vv=el.querySelector('.dc-lvv');if(vv)vv.textContent=a[2]+'%';}}};";

class DeviceCardWidget : public Widget {
 public:
  using PowerCb = std::function<void(bool)>;
  using LevelCb = std::function<void(int)>;
  DeviceCardWidget(const char* key, const char* title, const char* emoji, bool* power, bool* online, PowerCb cb)
      : Widget(key, title), _emoji(emoji), _power(power), _online(online), _pcb(cb) {}

  DeviceCardWidget& sub(const char* s) { _sub = s; return *this; }                    // transport label, e.g. "BLE"
  DeviceCardWidget& level(int* lv, LevelCb cb) { _level = lv; _lcb = cb; return *this; }  // adds an intensity slider

  const char* typeId() const override { return "devcard"; }
  const char* css() const override { return RW_DEVCARD_CSS; }
  const char* js() const override { return RW_DEVCARD_JS; }

  void card(Print& out) override {
    bool on = _power && *_power;
    out.print(F("<section class=\"card "));
    out.print(_level ? F("m") : F("s"));   // tile (2-up) unless it carries a slider
    out.print(F(" devcard"));
    if (on) out.print(F(" on"));
    out.print(F("\" data-type=\"devcard\" data-key=\""));
    out.print(_key);
    out.print(F("\" data-on=\""));  out.print(rwOnOff(true));
    out.print(F("\" data-off=\"")); out.print(rwOnOff(false));
    out.print(F("\"><div class=\"dc-hd\"><span class=\"dc-ic\">"));
    out.print(_emoji ? _emoji : "\xC2\xB7");
    out.print(F("</span><button class=\"dc-sw\" onclick=\"R.W.devcard.tog(this)\"><i></i></button></div>"));
    out.print(F("<div class=\"dc-nm\">"));
    out.print(rI18n(_title));
    out.print(F("</div><div class=\"dc-sub\"><span class=\"dc-dot "));
    out.print(_online ? (*_online ? "ok" : "bad") : "");
    out.print(F("\"></span>"));
    if (_sub) { out.print(_sub); out.print(F(" · ")); }
    out.print(F("<span class=\"dc-state\">"));
    out.print(rwOnOff(on));
    out.print(F("</span></div>"));
    if (_level) {
      out.print(F("<div class=\"dc-lv\"><div class=\"dc-lvl\"><span>Intensity</span><span class=\"dc-lvv\">"));
      out.print(*_level); out.print(F("%</span></div><input type=\"range\" min=\"0\" max=\"100\" value=\""));
      out.print(*_level);
      out.print(F("\" oninput=\"R.W.devcard.lv(this)\"></div>"));
    }
    out.print(F("</section>"));
  }

  bool hasState() const override { return true; }
  bool poll() override {
    String s;
    s += (_power && *_power) ? '1' : '0';
    s += (_online && *_online) ? '1' : '0';
    s += _level ? String(*_level) : String();
    return _trk.changed(s);
  }
  void writeKV(String& out) override {
    out += '"'; out += _key; out += "\":\"";
    out += (_power && *_power) ? '1' : '0'; out += ',';
    out += (_online && *_online) ? '1' : '0'; out += ',';
    out += _level ? String(*_level) : String("-1");
    out += '"';
  }
  void applyCommand(const String& v) override {
    if (v.length() < 1) return;
    if (v[0] == 'p') { bool on = v.length() > 1 && v[1] == '1'; if (_power) *_power = on; if (_pcb) _pcb(on); }
    else if (v[0] == 'l') { int lv = v.substring(1).toInt(); if (_level) *_level = lv; if (_lcb) _lcb(lv); }
  }

 private:
  const char* _emoji;
  const char* _sub = nullptr;
  bool* _power;
  bool* _online;
  int* _level = nullptr;
  PowerCb _pcb;
  LevelCb _lcb;
  RwTracked<String> _trk;
};

// ── Home summary: the at-a-glance hero at the top of an overview — an eyebrow, a big headline
// (e.g. "All good" / "2 offline") and a detail line (counts). A mood (0 good / 1 warn / 2 alarm)
// tints the gradient. Bind the two strings + the mood int; update them from your logic each tick.
static const char RW_SUMMARY_CSS[] PROGMEM =
  ".summary{padding:20px;background:linear-gradient(135deg,oklch(0.82 0.15 200 / .16),oklch(0.85 0.16 152 / .10));"
    "border-color:oklch(0.85 0.16 152 / .32)}"
  ".summary.warn{background:linear-gradient(135deg,oklch(0.83 0.16 85 / .16),oklch(0.83 0.16 85 / .05));border-color:oklch(0.83 0.16 85 / .4)}"
  ".summary.bad{background:linear-gradient(135deg,oklch(0.72 0.18 15 / .16),oklch(0.72 0.18 15 / .05));border-color:oklch(0.72 0.18 15 / .4)}"
  ".sm-eb{font:700 10px var(--font);letter-spacing:.16em;text-transform:uppercase;color:oklch(0.82 0.15 152)}"
  ".summary.warn .sm-eb{color:oklch(0.83 0.16 85)}.summary.bad .sm-eb{color:oklch(0.74 0.18 15)}"
  ".sm-big{font:800 25px var(--font);letter-spacing:-.02em;margin-top:6px;color:var(--ink1)}"
  ".sm-sub{font-size:13px;color:var(--ink2);margin-top:8px}";

static const char RW_SUMMARY_JS[] PROGMEM =
  "R.W.summary={update:function(el,v){var s=''+v,m=s[0],r=s.slice(1).split('\\n');"
  "el.classList.remove('warn','bad');if(m==='1')el.classList.add('warn');else if(m==='2')el.classList.add('bad');"
  "var b=el.querySelector('.sm-big');if(b)b.textContent=r[0]||'';"
  "var d=el.querySelector('.sm-sub');if(d)d.textContent=r[1]||'';}};";

class SummaryWidget : public Widget {
 public:
  SummaryWidget(const char* key, const char* eyebrow, String* headline, String* detail, int* mood)
      : Widget(key, eyebrow), _eb(eyebrow), _head(headline), _det(detail), _mood(mood) {}
  const char* typeId() const override { return "summary"; }
  const char* css() const override { return RW_SUMMARY_CSS; }
  const char* js() const override { return RW_SUMMARY_JS; }
  void card(Print& out) override {
    int m = _mood ? *_mood : 0;
    out.print(F("<section class=\"card m summary"));
    out.print(m == 1 ? F(" warn") : (m == 2 ? F(" bad") : F("")));
    out.print(F("\" data-type=\"summary\" data-key=\""));
    out.print(_key);
    out.print(F("\"><div class=\"sm-eb\">"));
    out.print(rI18n(_eb));
    out.print(F("</div><div class=\"sm-big\">"));
    if (_head) rwAttr(out, *_head);
    out.print(F("</div><div class=\"sm-sub\">"));
    if (_det) rwAttr(out, *_det);
    out.print(F("</div></section>"));
  }
  bool hasState() const override { return true; }
  bool poll() override {
    String s = String(_mood ? *_mood : 0) + (_head ? *_head : String()) + '|' + (_det ? *_det : String());
    return _trk.changed(s);
  }
  void writeKV(String& out) override {
    out += '"'; out += _key; out += "\":\"";
    out += String(_mood ? *_mood : 0);
    out += rwJsonEsc((_head ? *_head : String()) + "\n" + (_det ? *_det : String()));
    out += '"';
  }
 private:
  const char* _eb;
  String* _head;
  String* _det;
  int* _mood;
  RwTracked<String> _trk;
};
