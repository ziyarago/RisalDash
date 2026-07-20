#pragma once
#include "base.h"

// Rich visual widgets: robot face, live map (Leaflet, online), 3D orientation cube,
// thermal heatmap.

// ── Display: robot face — two animated eyes that show an emotion (bound to an int) ──
// A modern "AI companion" face: glowing accent eyes on a dark panel, idle blinking, and an emotion
// index 0..9 (Neutral/Happy/Sad/Angry/Surprised/Sleepy/Love/Wink/Dizzy/Look) that morphs or animates
// the eyes. Set the bound variable from your logic (or an AI agent) and it reacts over the WebSocket.
static const char RW_FACE_CSS[] PROGMEM =
  ".rface{display:flex;align-items:center;justify-content:center;gap:26px;height:132px;border-radius:14px;"
  "background:radial-gradient(120% 130% at 50% 28%,#16203a,#080d18);overflow:hidden}"
  ".reye{transition:transform .28s}"
  ".eyeball{width:44px;height:62px;border-radius:16px;background:var(--acc);position:relative;overflow:hidden;"
  "box-shadow:0 0 24px var(--acc),0 0 42px var(--acc);animation:rblink 4.6s infinite;"
  "transition:width .28s,height .28s,border-radius .28s,background .28s,box-shadow .28s}"
  ".eyeball::after{content:'';position:absolute;left:-30%;width:160%;height:150%;background:#0a1120;"
  "border-radius:50%;top:150%;transition:top .28s}"
  "@keyframes rblink{0%,90%,100%{transform:scaleY(1)}94%{transform:scaleY(.08)}}"
  ".rface[data-emo=\"1\"] .eyeball::after{top:52%}"                                    // happy
  ".rface[data-emo=\"2\"] .eyeball{height:46px}.rface[data-emo=\"2\"] .eyeball::after{top:56%}"
  ".rface[data-emo=\"2\"] .reye.l{transform:rotate(-16deg)}.rface[data-emo=\"2\"] .reye.r{transform:rotate(16deg)}"  // sad
  ".rface[data-emo=\"3\"] .eyeball::after{top:-56%}"
  ".rface[data-emo=\"3\"] .reye.l{transform:rotate(18deg)}.rface[data-emo=\"3\"] .reye.r{transform:rotate(-18deg)}"  // angry
  ".rface[data-emo=\"4\"] .eyeball{width:54px;height:54px;border-radius:50%}"          // surprised
  ".rface[data-emo=\"5\"] .eyeball{height:20px;border-radius:10px}"                    // sleepy
  ".rface[data-emo=\"6\"] .eyeball{background:#ff5c8a;box-shadow:0 0 24px #ff5c8a,0 0 44px #ff5c8a;border-radius:50% 50% 12px 12px}"  // love
  ".rface[data-emo=\"7\"] .reye.r .eyeball{height:9px;border-radius:5px}"              // wink (right eye)
  ".rface[data-emo=\"8\"] .eyeball{width:34px;height:34px;border-radius:50%}"
  ".rface[data-emo=\"8\"] .reye{animation:rwob 1.1s infinite}"                          // dizzy — wobble
  ".rface[data-emo=\"9\"] .reye{animation:rlook 2s infinite}"                           // look around
  "@keyframes rwob{0%,100%{transform:rotate(-11deg)}50%{transform:rotate(11deg)}}"
  "@keyframes rlook{0%,100%{transform:translateX(-9px)}50%{transform:translateX(9px)}}";
static const char RW_FACE_JS[] PROGMEM =
  "R.W.face={init:function(el){},update:function(el,v){var f=el.querySelector('.rface');if(f)f.setAttribute('data-emo',v);}};";
class FaceWidget : public Widget {
 public:
  FaceWidget(const char* key, const char* title, int* mood) : Widget(key, title), _mood(mood) {}
  const char* typeId() const override { return "face"; }
  const char* css() const override { return RW_FACE_CSS; }
  const char* js() const override { return RW_FACE_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rface\" data-emo=\""));
    out.print(_mood ? *_mood : 0);
    out.print(F("\"><div class=\"reye l\"><div class=\"eyeball\"></div></div>"
                "<div class=\"reye r\"><div class=\"eyeball\"></div></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_mood ? *_mood : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_mood ? *_mood : 0); }
 private:
  int* _mood; RwTracked<int> _trk;
};

// ── Display: live map (Leaflet) — a marker + trail that follow a bound lat/lon ──
// NEEDS INTERNET on the client (Leaflet + dark CARTO tiles load from a CDN), so it's an opt-in online
// widget, unlike the offline-first core. Dark basemap to match the theme. Bind two floats; the marker
// moves and leaves a trail.
static const char RW_MAP_CSS[] PROGMEM =
  // A map always spans the full grid width (span-4 stays span-4 even on wide screens, where .card.l
  // would otherwise fall back to span-2).
  ".card[data-type=map]{grid-column:1/-1}"
  ".rmap{height:300px;border-radius:12px;overflow:hidden;background:#0a1120}.rmap-c{height:100%;width:100%}";
static const char RW_MAP_JS[] PROGMEM = R"js(R.W.map={
_ld:function(cb){if(window.L)return cb();
if(!document.getElementById('lfcss')){var c=document.createElement('link');c.id='lfcss';c.rel='stylesheet';c.href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css';document.head.appendChild(c);}
var s=document.getElementById('lfjs');if(s){s.addEventListener('load',cb);return;}
s=document.createElement('script');s.id='lfjs';s.src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js';s.onload=cb;document.head.appendChild(s);},
init:function(el){var self=this,box=el.querySelector('.rmap-c');self._ld(function(){
var m=L.map(box,{zoomControl:false,attributionControl:false}).setView([0,0],14);
L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png',{maxZoom:19,subdomains:'abcd'}).addTo(m);
el._mk=L.circleMarker([0,0],{radius:7,color:'#22d3ee',fillColor:'#22d3ee',fillOpacity:1,weight:2}).addTo(m);
el._tr=L.polyline([],{color:'#22d3ee',weight:3,opacity:.75}).addTo(m);
el._m=m;el._first=1;setTimeout(function(){m.invalidateSize();},250);});},
update:function(el,v){if(!el._m||!v)return;var p=(''+v).split(',');var la=parseFloat(p[0]),lo=parseFloat(p[1]);if(isNaN(la))return;
el._mk.setLatLng([la,lo]);var t=el._tr.getLatLngs();t.push([la,lo]);if(t.length>150)t.shift();el._tr.setLatLngs(t);
if(el._first){el._m.setView([la,lo],15);el._m.invalidateSize();el._first=0;}else{el._m.panTo([la,lo],{animate:true});}}
};)js";
class MapWidget : public Widget {
 public:
  MapWidget(const char* key, const char* title, float* lat, float* lon) : Widget(key, title), _lat(lat), _lon(lon) {}
  const char* typeId() const override { return "map"; }
  const char* css() const override { return RW_MAP_CSS; }
  const char* js() const override { return RW_MAP_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rmap\"><div class=\"rmap-c\"></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override {
    float a = _lat ? *_lat : 0, b = _lon ? *_lon : 0;
    if (a != _la || b != _lo) { _la = a; _lo = b; return true; }
    return false;
  }
  void writeKV(String& out) override {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.5f,%.5f", _lat ? *_lat : 0.0f, _lon ? *_lon : 0.0f);
    out += '"'; out += _key; out += "\":\""; out += buf; out += '"';
  }
 private:
  float* _lat; float* _lon; float _la = 0, _lo = 0;
};

// ── Display: 3D orientation cube — a CSS cube that rotates with a bound pitch/roll/yaw (IMU) ──
static const char RW_CUBE_CSS[] PROGMEM =
  ".rcube-s{height:220px;display:flex;align-items:center;justify-content:center;perspective:620px}"
  ".rcube{width:108px;height:108px;position:relative;transform-style:preserve-3d;transition:transform .12s linear}"
  ".rcube i{position:absolute;width:108px;height:108px;box-sizing:border-box;border:2px solid var(--acc);"
  "background:linear-gradient(135deg,var(--acc),var(--acc2));opacity:.82;display:flex;align-items:center;"
  "justify-content:center;font:800 22px var(--font);color:var(--acc-ink)}"
  ".rcf1{transform:translateZ(54px)}.rcf2{transform:rotateY(180deg) translateZ(54px)}"
  ".rcf3{transform:rotateY(90deg) translateZ(54px)}.rcf4{transform:rotateY(-90deg) translateZ(54px)}"
  ".rcf5{transform:rotateX(90deg) translateZ(54px)}.rcf6{transform:rotateX(-90deg) translateZ(54px)}";
static const char RW_CUBE_JS[] PROGMEM =
  "R.W.cube={init:function(el){},update:function(el,v){var c=el.querySelector('.rcube');if(!c||!v)return;"
  "var p=(''+v).split(',');c.style.transform='rotateX('+(-p[0])+'deg) rotateZ('+p[1]+'deg) rotateY('+p[2]+'deg)';}};";
class CubeWidget : public Widget {
 public:
  CubeWidget(const char* key, const char* title, float* pitch, float* roll, float* yaw)
      : Widget(key, title), _p(pitch), _r(roll), _y(yaw) {}
  const char* typeId() const override { return "cube"; }
  const char* css() const override { return RW_CUBE_CSS; }
  const char* js() const override { return RW_CUBE_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rcube-s\"><div class=\"rcube\"><i class=\"rcf1\">F</i><i class=\"rcf2\">B</i>"
                "<i class=\"rcf3\">R</i><i class=\"rcf4\">L</i><i class=\"rcf5\">U</i><i class=\"rcf6\">D</i></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override {
    float a = _p ? *_p : 0, b = _r ? *_r : 0, c = _y ? *_y : 0;
    if (a != _lp || b != _lr || c != _ly) { _lp = a; _lr = b; _ly = c; return true; }
    return false;
  }
  void writeKV(String& out) override {
    char buf[40];
    snprintf(buf, sizeof(buf), "%.1f,%.1f,%.1f", _p ? *_p : 0.0f, _r ? *_r : 0.0f, _y ? *_y : 0.0f);
    out += '"'; out += _key; out += "\":\""; out += buf; out += '"';
  }
 private:
  float *_p, *_r, *_y;
  float _lp = 0, _lr = 0, _ly = 0;
};

// ── Display: heatmap / thermal — a colours grid (e.g. MLX90640 32x24). Push frames with .frame() ──
static const char RW_HEAT_CSS[] PROGMEM =
  ".rheat{width:100%;height:auto;display:block;image-rendering:pixelated;border-radius:10px;background:#0a1120}";
static const char RW_HEAT_JS[] PROGMEM =
  "R.W.heat={init:function(el){},update:function(el,v){var c=el.querySelector('canvas');if(!c||!v)return;"
  "var p=(''+v).split(',');var w=+p[0],h=+p[1];if(!w||!h)return;c.width=w;c.height=h;"
  "var x=c.getContext('2d'),im=x.createImageData(w,h),d=im.data;"
  "for(var i=0;i<w*h;i++){var t=(+p[2+i])/255;"
  "d[i*4]=255*Math.min(1,Math.max(0,1.5-Math.abs(4*t-3)));"
  "d[i*4+1]=255*Math.min(1,Math.max(0,1.5-Math.abs(4*t-2)));"
  "d[i*4+2]=255*Math.min(1,Math.max(0,1.5-Math.abs(4*t-1)));d[i*4+3]=255;}x.putImageData(im,0,0);}};";
class HeatmapWidget : public Widget {
 public:
  HeatmapWidget(const char* key, const char* title, uint8_t cols, uint8_t rows)
      : Widget(key, title), _c(cols), _r(rows) { _buf = new uint8_t[(uint16_t)cols * rows](); }
  const char* typeId() const override { return "heat"; }
  const char* css() const override { return RW_HEAT_CSS; }
  const char* js() const override { return RW_HEAT_JS; }
  // Push a frame of cols*rows temperatures; auto-scaled to the frame's own min..max.
  void frame(const float* d) {
    uint16_t n = (uint16_t)_c * _r;
    float mn = 1e9f, mx = -1e9f;
    for (uint16_t i = 0; i < n; i++) { if (d[i] < mn) mn = d[i]; if (d[i] > mx) mx = d[i]; }
    float rng = mx - mn; if (rng < 1e-3f) rng = 1;
    for (uint16_t i = 0; i < n; i++) _buf[i] = (uint8_t)(255.0f * (d[i] - mn) / rng);
    _dirty = true;
  }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<canvas class=\"rheat\"></canvas>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { if (_dirty) { _dirty = false; return true; } return false; }
  void writeKV(String& out) override {
    out += '"'; out += _key; out += "\":\""; out += _c; out += ','; out += _r;
    uint16_t n = (uint16_t)_c * _r;
    for (uint16_t i = 0; i < n; i++) { out += ','; out += _buf[i]; }
    out += '"';
  }
 private:
  uint8_t _c, _r; uint8_t* _buf; bool _dirty = false;
};
