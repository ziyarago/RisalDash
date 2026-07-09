#pragma once
#include "base.h"

// ── Radio Browser: an editable station list with a real editor UX — drag a row to reorder (or the
// ▲/▼ buttons on touch), edit name/URL inline, ✕ to remove, "+ Add" for a new row. Its *value* is
// the same "Name | URL | Meta" (one per line) a textarea would hold, so the sketch's parsing and NVS
// persistence are unchanged; only the editing surface is richer. A later revision layers an online
// "browse by country" catalog on top (a proxy route + results list) — this class is the foundation.

static const char RW_RADIOBROWSER_CSS[] PROGMEM =
  ".rbr-list{list-style:none;margin:0;padding:0;display:flex;flex-direction:column;gap:6px}"
  ".rbr-row{display:flex;align-items:center;gap:6px;background:var(--field);border:1px solid var(--line2);"
  "border-radius:12px;padding:6px 8px}"
  ".rbr-row.drag{opacity:.45}.rbr-row.over{border-color:var(--acc);box-shadow:0 0 0 1px var(--acc)}"
  ".rbr-h{cursor:grab;color:var(--ink3);font-size:15px;user-select:none;padding:0 2px;touch-action:none}"
  ".rbr-n{width:34%;flex:none}.rbr-u{flex:1;min-width:0}"
  ".rbr-n,.rbr-u{height:34px;border-radius:9px;border:1px solid var(--line2);background:var(--bg2);"
  "color:var(--ink1);font:13px var(--font);padding:0 8px;box-sizing:border-box}"
  ".rbr-mv{display:flex;flex-direction:column;line-height:1}"
  ".rbr-mv button,.rbr-x{border:none;background:transparent;color:var(--ink3);cursor:pointer}"
  ".rbr-mv button{font-size:9px;padding:1px 3px}.rbr-x{font-size:15px;padding:4px}"
  ".rbr-add{margin-top:8px;height:36px;width:100%;border:1px dashed var(--line2);border-radius:10px;"
  "background:transparent;color:var(--ink2);font:600 13px var(--font);cursor:pointer}"
  ".rbr-add:hover{border-color:var(--acc);color:var(--ink1)}";

// Client behaviour. The value round-trips as newline-delimited "Name | URL | Meta"; rows carry the
// (unedited) meta in data-meta so reordering preserves it. Commit rebuilds the value from the DOM.
static const char RW_RADIOBROWSER_JS[] PROGMEM =
  "R.W.radiobrowser={"
  "rows:function(el){return el.querySelectorAll('.rbr-row');},"
  "build:function(el){var t=[];this.rows(el).forEach(function(r){"
  "var n=r.querySelector('.rbr-n').value.replace(/[|\\r\\n]/g,' ').trim();"
  "var u=r.querySelector('.rbr-u').value.replace(/[|\\r\\n]/g,' ').trim();"
  "var m=(r.dataset.meta||'').replace(/[|\\r\\n]/g,' ').trim();"
  "if(!n&&!u)return;t.push(n+' | '+u+(m?' | '+m:''));});return t.join('\\n');},"
  "commit:function(el){var v=this.build(el);var s=el.querySelector('.rbr-src');if(s)s.value=v;R.send(el.dataset.key,v);},"
  "mkRow:function(el,n,u,m){var W=this;var li=document.createElement('li');li.className='rbr-row';"
  "li.draggable=true;li.dataset.meta=m||'';"
  "li.innerHTML='<span class=\"rbr-h\">\\u283f</span><input class=\"rbr-n\" placeholder=\"Name\">"
  "<input class=\"rbr-u\" placeholder=\"https://...mp3\"><div class=\"rbr-mv\">"
  "<button type=\"button\" data-d=\"-1\">\\u25b2</button><button type=\"button\" data-d=\"1\">\\u25bc</button>"
  "</div><button type=\"button\" class=\"rbr-x\">\\u2715</button>';"
  "li.querySelector('.rbr-n').value=n||'';li.querySelector('.rbr-u').value=u||'';"
  "li.querySelector('.rbr-n').addEventListener('change',function(){W.commit(el);});"
  "li.querySelector('.rbr-u').addEventListener('change',function(){W.commit(el);});"
  "li.querySelector('.rbr-x').addEventListener('click',function(){li.remove();W.commit(el);});"
  "li.querySelectorAll('.rbr-mv button').forEach(function(b){b.addEventListener('click',function(){"
  "var d=+b.dataset.d,p=li.parentNode;"
  "if(d<0&&li.previousElementSibling)p.insertBefore(li,li.previousElementSibling);"
  "else if(d>0&&li.nextElementSibling)p.insertBefore(li.nextElementSibling,li);W.commit(el);});});"
  "li.addEventListener('dragstart',function(){li.classList.add('drag');el._drag=li;});"
  "li.addEventListener('dragend',function(){li.classList.remove('drag');"
  "W.rows(el).forEach(function(r){r.classList.remove('over');});W.commit(el);});"
  "li.addEventListener('dragover',function(e){e.preventDefault();if(el._drag&&el._drag!==li)li.classList.add('over');});"
  "li.addEventListener('dragleave',function(){li.classList.remove('over');});"
  "li.addEventListener('drop',function(e){e.preventDefault();li.classList.remove('over');var d=el._drag;"
  "if(!d||d===li)return;var p=li.parentNode,rs=[].slice.call(W.rows(el));"
  "if(rs.indexOf(d)<rs.indexOf(li))p.insertBefore(d,li.nextElementSibling);else p.insertBefore(d,li);});"
  "return li;},"
  "render:function(el,text){var ul=el.querySelector('.rbr-list');if(!ul)return;ul.innerHTML='';var W=this;"
  "(text||'').split('\\n').forEach(function(ln){ln=ln.trim();if(!ln)return;var p=ln.split('|');"
  "var n=(p[0]||'').trim(),u=(p[1]||'').trim(),m=(p[2]||'').trim();if(!n&&!u)return;ul.appendChild(W.mkRow(el,n,u,m));});},"
  "init:function(el){var s=el.querySelector('.rbr-src');this.render(el,s?s.value:'');var W=this;"
  "var a=el.querySelector('.rbr-add');if(a)a.addEventListener('click',function(){"
  "el.querySelector('.rbr-list').appendChild(W.mkRow(el,'','',''));});},"
  "update:function(el,v){if(el.contains(document.activeElement))return;"
  "var s=el.querySelector('.rbr-src');if(s)s.value=v;this.render(el,v);}};";

class RadioBrowserWidget : public Widget {
 public:
  using Cb = std::function<void(const String&)>;
  RadioBrowserWidget(const char* key, const char* title, String* val, Cb cb)
      : Widget(key, title), _val(val), _cb(cb) {}
  const char* typeId() const override { return "radiobrowser"; }
  const char* css() const override { return RW_RADIOBROWSER_CSS; }
  const char* js() const override { return RW_RADIOBROWSER_JS; }
  void card(Print& out) override {
    cardOpen(out);
    // The current value rides along in a hidden field so init() can paint before the first WS push.
    out.print(F("<div class=\"rbr\"><textarea class=\"rbr-src\" hidden>"));
    if (_val) rwAttr(out, *_val);
    out.print(F("</textarea><ul class=\"rbr-list\"></ul>"
                "<button class=\"rbr-add\" type=\"button\">+ Add station</button></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_val ? *_val : String()); }
  void writeKV(String& out) override {
    out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_val ? *_val : String()); out += '"';
  }
  // Incoming WS values arrive JSON-escaped (the transport doesn't unescape) — restore \n, \" and \\.
  void applyCommand(const String& v) override {
    String s; s.reserve(v.length());
    for (uint16_t i = 0; i < v.length(); i++) {
      char c = v[i];
      if (c == '\\' && i + 1 < v.length()) {
        char n = v[++i];
        s += (n == 'n') ? '\n' : (n == 't') ? '\t' : n;   // \n \t \" \\ -> literal
      } else {
        s += c;
      }
    }
    if (_val) *_val = s;
    if (_cb) _cb(s);
  }
 private:
  String* _val; Cb _cb; RwTracked<String> _trk;
};
