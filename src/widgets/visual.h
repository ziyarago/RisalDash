#pragma once
#include "base.h"

// Rich visual widgets: robot face, live map (Leaflet, online), 3D orientation cube,
// thermal heatmap.

// ── Display: robot face — two animated eyes that show an emotion (bound to an int) ──
// A modern "AI companion" face: glowing accent "lids" on a dark panel, idle blinking, and a rich
// emotion set (0..41). 0..9 keep their classic meaning (Neutral/Happy/Sad/Angry/Surprised/Sleepy/
// Love/Wink/Dizzy/Look) for backward compatibility; 10..41 add listening, pondering, laughing,
// worried, nervous, shocked, skeptical, speaking, loading, error, dead, battery and more. The eyes
// are drawn as SVG from a compact spec table in JS (see RW_FACE_JS), so the same shapes render here
// and on-device. Set the bound int from your logic (or an AI agent) and it reacts over the WebSocket.
static const char RW_FACE_CSS[] PROGMEM =
  ".rface{height:132px;border-radius:14px;overflow:hidden;display:flex;align-items:center;justify-content:center;"
  "background:radial-gradient(120% 130% at 50% 28%,#16203a,#080d18)}"
  ".rface svg{width:100%;height:100%;display:block}"
  ".rface .eye{fill:var(--acc);filter:drop-shadow(0 0 6px var(--acc))}"
  ".rface .eye.pk{fill:#ff5c8a;filter:drop-shadow(0 0 7px #ff5c8a)}"
  ".rface .ln{fill:none;stroke:var(--acc);stroke-width:6;stroke-linecap:round;stroke-linejoin:round;filter:drop-shadow(0 0 6px var(--acc))}"
  "@keyframes rblink{0%,90%,100%{transform:scaleY(1)}95%{transform:scaleY(.1)}}"
  "@keyframes rshk{0%,100%{transform:translateX(0)}25%{transform:translateX(-2.5px)}75%{transform:translateX(2.5px)}}"
  "@keyframes rspn{to{transform:rotate(360deg)}}"
  "@keyframes rpls{0%,100%{opacity:1}50%{opacity:.5}}"
  "@keyframes rflt{0%,100%{transform:translateY(0)}50%{transform:translateY(-4px)}}"
  "@keyframes rlook{0%,100%{transform:translateX(-9px)}50%{transform:translateX(9px)}}"
  ".rface .rbl{transform-box:fill-box;transform-origin:center;animation:rblink 4.6s infinite}"
  ".rface .rshk{animation:rshk .5s infinite}"
  ".rface .rspn{transform-box:view-box;transform-origin:120px 78px;animation:rspn 1.4s linear infinite}"
  ".rface .rpls{animation:rpls 1.6s ease-in-out infinite}"
  ".rface .rflt{transform-box:fill-box;transform-origin:center;animation:rflt 2s ease-in-out infinite}"
  ".rface .rlook{animation:rlook 2s ease-in-out infinite}"
  "@media(prefers-reduced-motion:reduce){.rface *{animation:none!important}}";
static const char RW_FACE_JS[] PROGMEM = R"js(R.W.face=(function(){
var CY=78,EXL=74;
function n(x){return Math.round(x*10)/10}
function P(a){return n(a[0])+','+n(a[1])}
function sb(a,b){return[a[0]-b[0],a[1]-b[1]]}
function ad(a,b){return[a[0]+b[0],a[1]+b[1]]}
function ml(a,s){return[a[0]*s,a[1]*s]}
function L(a){return Math.hypot(a[0],a[1])||1e-6}
function nm(a){return ml(a,1/L(a))}
function ds(a,b){return L(sb(a,b))}
function rpoly(p,r){var d='',N=p.length,i;for(i=0;i<N;i++){var p0=p[(i-1+N)%N],p1=p[i],p2=p[(i+1)%N],v1=nm(sb(p0,p1)),v2=nm(sb(p2,p1)),rr=Math.min(r,ds(p0,p1)/2,ds(p2,p1)/2),a=ad(p1,ml(v1,rr)),b=ad(p1,ml(v2,rr));d+=(i?'L':'M')+P(a)+'Q'+P(p1)+' '+P(b)}return d+'Z'}
function ep(cx,cy,e){cy+=e.off||0;var w=e.w,h=e.h,ri=e.ri||0,ro=e.ro||0,s=e.shape||'q',r=e.r||0;
if(s=='circle'){var R=w/2;return{p:'M'+P([cx+R,cy])+'a'+n(R)+','+n(R)+' 0 1,0 '+n(-w)+',0 a'+n(R)+','+n(R)+' 0 1,0 '+n(w)+',0Z'}}
if(s=='spiral'){var d='M'+P([cx,cy]),T=2.6,rm=w/2,a;for(a=0;a<T*6.283;a+=0.25){var q=rm*a/(T*6.283);d+=' L'+P([cx+q*Math.cos(a),cy+q*Math.sin(a)])}return{p:d,st:1}}
if(s=='x'){var q=w/2;return{p:'M'+P([cx-q,cy-q])+'L'+P([cx+q,cy+q])+'M'+P([cx+q,cy-q])+'L'+P([cx-q,cy+q]),st:1}}
if(s=='heart'){var u=w/2;return{p:'M'+P([cx,cy+u*0.3])+'C'+P([cx,cy-u*0.3])+' '+P([cx-u,cy-u*0.3])+' '+P([cx-u,cy+u*0.15])+'C'+P([cx-u,cy+u*0.6])+' '+P([cx,cy+u*0.8])+' '+P([cx,cy+u])+'C'+P([cx,cy+u*0.8])+' '+P([cx+u,cy+u*0.6])+' '+P([cx+u,cy+u*0.15])+'C'+P([cx+u,cy-u*0.3])+' '+P([cx,cy-u*0.3])+' '+P([cx,cy+u*0.3])+'Z'}}
if(s=='happy'){var rr=Math.min(r,w/2,h/2),cv=e.curve!=null?e.curve:h*0.9,d='M'+P([cx-w/2,cy-h/2+rr]);d+='Q'+P([cx-w/2,cy-h/2])+' '+P([cx-w/2+rr,cy-h/2])+'L'+P([cx+w/2-rr,cy-h/2])+'Q'+P([cx+w/2,cy-h/2])+' '+P([cx+w/2,cy-h/2+rr])+'L'+P([cx+w/2,cy+h/2-rr*0.6])+'Q'+P([cx,cy+h/2-cv])+' '+P([cx-w/2,cy+h/2-rr*0.6]);return{p:d+'Z'}}
var tI=[cx+w/2,cy-h/2-ri],tO=[cx-w/2,cy-h/2-ro],bO=[cx-w/2,cy+h/2],bI=[cx+w/2,cy+h/2];return{p:rpoly([tO,tI,bI,bO],r)}}
function es(e,side,pk){var o=ep(EXL,CY,e),cl=o.st?'ln':('eye'+(pk?' pk':'')),g='<path class="'+cl+'" d="'+o.p+'"'+(o.st?' fill="none"':'')+'/>';return side=='L'?g:'<g transform="translate(240,0) scale(-1,1)">'+g+'</g>'}
function dr(x,y,s){return'<path class="eye" d="M'+x+','+(y-s)+' C'+(x+s*0.9)+','+(y+s*0.1)+' '+(x+s*0.7)+','+(y+s)+' '+x+','+(y+s)+' C'+(x-s*0.7)+','+(y+s)+' '+(x-s*0.9)+','+(y+s*0.1)+' '+x+','+(y-s)+'Z"/>'}
function str(x,y,s){return'<path class="eye" d="M'+x+','+(y-s)+' L'+(x+s*0.28)+','+(y-s*0.28)+' L'+(x+s)+','+y+' L'+(x+s*0.28)+','+(y+s*0.28)+' L'+x+','+(y+s)+' L'+(x-s*0.28)+','+(y+s*0.28)+' L'+(x-s)+','+y+' L'+(x-s*0.28)+','+(y-s*0.28)+'Z"/>'}
function hp(x,y,s){return'<path class="rflt" fill="#ff5c8a" d="M'+x+','+(y+s*0.3)+' C'+x+','+(y-s*0.3)+' '+(x-s)+','+(y-s*0.3)+' '+(x-s)+','+(y+s*0.15)+' C'+(x-s)+','+(y+s*0.6)+' '+x+','+(y+s*0.8)+' '+x+','+(y+s)+' C'+x+','+(y+s*0.8)+' '+(x+s)+','+(y+s*0.6)+' '+(x+s)+','+(y+s*0.15)+' C'+(x+s)+','+(y-s*0.3)+' '+x+','+(y-s*0.3)+' '+x+','+(y+s*0.3)+'Z"/>'}
function FX(f){
if(f=='tear')return dr(60,104,5);
if(f=='sweat')return dr(196,42,6);
if(f=='hearts')return hp(44,34,8)+hp(198,34,7);
if(f=='zzz')return'<text class="eye" x="184" y="46" font-size="17" font-weight="800">z</text><text class="eye" x="198" y="34" font-size="13" font-weight="800">z</text>';
if(f=='sparkles')return str(150,44,9)+str(96,50,6);
if(f=='bell')return'<path class="ln" d="M120,32 C109,32 105,41 105,52 C105,63 99,65 99,69 L141,69 C141,65 135,63 135,52 C135,41 131,32 120,32Z"/><circle class="eye" cx="120" cy="28" r="3.5"/><path class="ln" d="M114,73 a6,6 0 0,0 12,0"/>';
if(f=='wave')return'<path class="ln" d="M56,116 q10,-9 20,0 t20,0 t20,0 t20,0 t20,0 t20,0"/>';
if(f=='notes')return'<text class="eye" x="40" y="52" font-size="20">&#9834;</text><text class="eye" x="188" y="46" font-size="16">&#9834;</text>';
if(f=='battery')return'<g transform="translate(120,38)"><rect class="ln" x="-26" y="-9" width="48" height="18" rx="4"/><rect class="eye" x="22" y="-4" width="4" height="8" rx="2"/><rect fill="#ff5f57" x="-23" y="-6" width="9" height="12" rx="2"/></g>';
if(f=='wavymouth')return'<path class="ln" d="M104,110 q8,-8 16,0 t16,0"/>';
return''}
function GL(g){
if(g=='error')return'<g class="ln"><path d="M78,60 L100,75 L78,90"/><path d="M162,60 L140,75 L162,90"/><path d="M108,100 L132,100"/></g>';
if(g=='loading'){var d='',i;for(i=0;i<12;i++){var a=i/12*6.283,x=120+22*Math.cos(a),y=75+22*Math.sin(a);d+='<circle class="eye" cx="'+n(x)+'" cy="'+n(y)+'" r="3" opacity="'+(i/12*0.85+0.15).toFixed(2)+'"/>'}return'<g class="rspn">'+d+'</g>'}
return''}
var F=[
{e:{w:50,h:56,r:16}},{e:{w:54,h:42,r:18,shape:'happy',curve:34}},{e:{w:50,h:22,r:11,ri:2,ro:12,off:6,shape:'happy',curve:-14}},
{e:{w:52,h:40,r:9,ri:16,ro:-8}},{e:{w:56,h:58,r:16}},{e:{w:52,h:15,r:7,ri:1,ro:5,off:3}},
{pk:1,e:{w:46,h:46,shape:'heart'}},{e:{w:50,h:56,r:16},r:{w:52,h:12,r:6}},{anim:'spin',nb:1,e:{w:44,h:44,shape:'spiral'}},
{anim:'look',e:{w:50,h:56,r:16}},{anim:'pulse',e:{w:50,h:50,r:16}},{e:{w:50,h:44,r:14,off:-3,ri:4}},
{e:{w:54,h:24,r:12,shape:'happy',curve:20}},{e:{w:46,h:22,r:11,shape:'happy',curve:18}},{e:{w:52,h:38,r:11,ri:11,ro:-4}},
{anim:'shake',e:{w:52,h:40,r:9,ri:16,ro:-8}},{fx:['tear'],e:{w:50,h:22,r:11,ri:2,ro:12,off:6,shape:'happy',curve:-14}},
{e:{w:56,h:17,r:8,ri:2,ro:8,off:3}},{e:{w:50,h:13,r:6,ri:4,ro:-1,off:2}},{e:{w:50,h:30,r:12,ri:-7,ro:3}},
{fx:['sweat'],e:{w:50,h:30,r:12,ri:-7,ro:3}},{fx:['sweat'],anim:'shake',e:{w:50,h:30,r:12,ri:-9,ro:4}},{e:{w:62,h:64,r:16}},
{anim:'shake',e:{w:58,h:62,r:18}},{e:{w:58,h:58,shape:'circle'}},{e:{w:44,h:34,r:13},r:{w:44,h:15,r:7,off:-2}},
{e:{w:48,h:22,r:9,ri:4,ro:10,off:2,shape:'happy',curve:-10},r:{w:48,h:15,r:7,ro:6,off:2}},{e:{w:48,h:15,r:7}},{e:{w:42,h:13,r:6}},
{e:{w:50,h:16,r:8,ri:2,ro:9,off:2},r:{w:50,h:13,r:6,off:2}},{e:{w:50,h:13,r:6,off:2},r:{w:50,h:13,r:6,ri:4}},
{anim:'shake',e:{w:48,h:50,r:15},r:{w:44,h:44,r:14,off:-3}},{fx:['wave'],e:{w:46,h:50,r:15}},
{fx:['notes'],e:{w:46,h:26,r:13,shape:'happy',curve:22}},{fx:['zzz'],e:{w:52,h:18,r:9,shape:'happy',curve:16}},
{fx:['sparkles'],e:{w:50,h:30,r:16,shape:'happy',curve:26}},{g:'error'},{anim:'pulse',fx:['bell'],e:{w:46,h:20,r:10,off:34}},
{g:'loading'},{fx:['hearts'],e:{w:50,h:30,r:16,shape:'happy',curve:26}},{fx:['wavymouth'],nb:1,e:{w:34,h:34,shape:'x'}},
{fx:['battery'],e:{w:48,h:16,r:8,off:30}}];
function build(v){var s=F[v]||F[0],inner;
if(s.g){inner=GL(s.g)}else{var pk=s.pk?1:0,blink=!s.anim&&!s.nb,ec='ey'+(blink?' rbl':'');inner='<g class="'+ec+'">'+es(s.e,'L',pk)+es(s.r||s.e,'R',pk)+'</g>';(s.fx||[]).forEach(function(f){inner+=FX(f)})}
var g=inner,a=s.anim;
if(a=='shake')g='<g class="rshk">'+g+'</g>';else if(a=='spin')g='<g class="rspn">'+g+'</g>';else if(a=='pulse')g='<g class="rpls">'+g+'</g>';else if(a=='look')g='<g class="rlook">'+g+'</g>';
return'<svg viewBox="0 0 240 150" preserveAspectRatio="xMidYMid meet">'+g+'</svg>'}
function render(f,v){v=+v||0;f.innerHTML=build(v);f.setAttribute('data-emo',v)}
return{init:function(el){var f=el.querySelector('.rface');if(f)render(f,f.getAttribute('data-emo')||0)},update:function(el,v){var f=el.querySelector('.rface');if(f)render(f,v)}}
})();)js";
// Named mood indices for face() — pass any of these (or a raw int) to the bound mood variable.
// 0..9 are the classic set (backward-compatible); 10..41 are the extended set. Mirrors the JS table.
enum FaceMood {
  FACE_NEUTRAL = 0, FACE_HAPPY, FACE_SAD, FACE_ANGRY, FACE_SURPRISED, FACE_SLEEPY, FACE_LOVE, FACE_WINK,
  FACE_DIZZY, FACE_LOOK, FACE_LISTENING, FACE_PONDERING, FACE_LAUGHING, FACE_GLEE, FACE_FURIOUS,
  FACE_FRUSTRATED, FACE_CRYING, FACE_TIRED, FACE_BORED, FACE_WORRIED, FACE_NERVOUS, FACE_ANXIOUS,
  FACE_SHOCKED, FACE_SCARED, FACE_AWE, FACE_SKEPTICAL, FACE_SUSPICIOUS, FACE_FOCUSED, FACE_SQUINT,
  FACE_ANNOYED, FACE_UNIMPRESSED, FACE_CONFUSED, FACE_SPEAKING, FACE_MUSIC, FACE_SLEEP, FACE_SUCCESS,
  FACE_ERROR, FACE_NOTIFICATION, FACE_LOADING, FACE_HEART, FACE_DEAD, FACE_BATTERY, FACE_COUNT
};

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
    out.print(F("\"></div>"));  // JS (R.W.face) draws the SVG eyes from the mood index
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_mood ? *_mood : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_mood ? *_mood : 0); }
 private:
  int* _mood; RwTracked<int> _trk;
};

// ── Display: weather icon — a bound int (0..9) picks a weather glyph, drawn as SVG ──
// A companion to the face: same idea, a compact icon set. 0 Sun · 1 Cloudy · 2 Rain · 3 Snow ·
// 4 Thunder · 5 Fog · 6 Wind · 7 Storm · 8 Partly cloudy · 9 Night. Bind an int (e.g. from a weather
// API or a forecast state) and the icon updates over the WebSocket.
static const char RW_WX_CSS[] PROGMEM =
  ".rwx{height:132px;border-radius:14px;overflow:hidden;display:flex;align-items:center;justify-content:center;"
  "background:radial-gradient(120% 130% at 50% 24%,#1a2436,#0b0f18)}"
  ".rwx svg{width:100%;height:100%;display:block}"
  ".rwx .ic{fill:var(--acc)}.rwx .sun{fill:#ffce57}.rwx .cloud{fill:#c7d0dc}.rwx .snow{fill:#eaf2f8}"
  ".rwx .amber{fill:none;stroke:#ffce57}.rwx .rain{stroke:#5c9bef}"
  ".rwx .ln{fill:none;stroke:#c7d0dc;stroke-linecap:round;stroke-linejoin:round}"
  "@keyframes rwspin{to{transform:rotate(360deg)}}"
  ".rwx .spin{transform-box:view-box;transform-origin:120px 75px;animation:rwspin 3s linear infinite}"
  "@media(prefers-reduced-motion:reduce){.rwx *{animation:none!important}}";
static const char RW_WX_JS[] PROGMEM = R"js(R.W.weather=(function(){
var n=function(x){return Math.round(x*10)/10};
function cloud(cx,cy,sc){return '<path class="cloud" transform="translate('+cx+','+cy+') scale('+sc+')" d="M-34,10 a17,17 0 0,1 4,-33 a22,22 0 0,1 42,4 a15,15 0 0,1 -2,29 Z"/>'}
function sun(cx,cy,r,cnt){var s='<circle class="sun" cx="'+cx+'" cy="'+cy+'" r="'+r+'"/>',i;for(i=0;i<cnt;i++){var a=i/cnt*6.283;s+='<line stroke="#ffce57" stroke-width="4.5" stroke-linecap="round" x1="'+n(cx+(r+7)*Math.cos(a))+'" y1="'+n(cy+(r+7)*Math.sin(a))+'" x2="'+n(cx+(r+16)*Math.cos(a))+'" y2="'+n(cy+(r+16)*Math.sin(a))+'"/>'}return s}
var G=[
function(){return sun(120,75,24,8)},
function(){return cloud(122,80,1.6)},
function(){return cloud(122,60,1.4)+'<g class="rain" stroke-width="4.5" stroke-linecap="round">'+[-30,-8,14].map(function(dx){return '<line x1="'+(120+dx)+'" y1="98" x2="'+(112+dx)+'" y2="118"/>'}).join('')+'</g>'},
function(){return cloud(122,60,1.4)+'<g class="snow">'+[-26,-4,18].map(function(dx){return '<circle cx="'+(114+dx)+'" cy="112" r="3.5"/>'}).join('')+'</g>'},
function(){return cloud(122,60,1.4)+'<path class="amber" stroke-width="5" d="M118,94 L106,114 L118,114 L110,132"/>'},
function(){return '<g class="ln" stroke-width="5">'+[54,72,90,108].map(function(y,i){return '<line x1="'+(62+(i%2)*6)+'" y1="'+y+'" x2="'+(178-(i%2)*6)+'" y2="'+y+'"/>'}).join('')+'</g>'},
function(){return '<g class="ln" stroke-width="5"><path d="M56,60 h60 a10,10 0 1,0 -10,-10"/><path d="M56,82 h84 a11,11 0 1,1 -11,11"/><path d="M56,104 h48 a9,9 0 1,0 -9,9"/></g>'},
function(){var d='',i;for(i=0;i<40;i++){var a=i*0.5,r=3+i*0.9,x=120+r*Math.cos(a),y=75+r*Math.sin(a);d+='<circle cx="'+n(x)+'" cy="'+n(y)+'" r="1.7" class="cloud" opacity="'+(1-i/44).toFixed(2)+'"/>'}return '<g class="spin">'+d+'</g>'},
function(){return sun(96,60,15,6)+cloud(134,84,1.3)},
function(){return '<path class="ic" d="M138,54 a26,26 0 1,0 8,44 a20,20 0 0,1 -8,-44Z"/><circle class="snow" cx="92" cy="62" r="2.5"/><circle class="snow" cx="80" cy="92" r="2"/><circle class="snow" cx="106" cy="100" r="1.6"/>'}
];
function build(v){var f=G[v]||G[0];return '<svg viewBox="0 0 240 150" preserveAspectRatio="xMidYMid meet">'+f()+'</svg>'}
function render(el,v){var f=el.querySelector('.rwx');if(f){f.innerHTML=build(+v||0);f.setAttribute('data-wx',v)}}
return{init:function(el){var f=el.querySelector('.rwx');if(f)render(el,f.getAttribute('data-wx')||0)},update:function(el,v){render(el,v)}}
})();)js";
class WeatherWidget : public Widget {
 public:
  WeatherWidget(const char* key, const char* title, int* code) : Widget(key, title), _code(code) {}
  const char* typeId() const override { return "weather"; }
  const char* css() const override { return RW_WX_CSS; }
  const char* js() const override { return RW_WX_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rwx\" data-wx=\""));
    out.print(_code ? *_code : 0);
    out.print(F("\"></div>"));  // JS (R.W.weather) draws the SVG icon from the code
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_code ? *_code : 0); }
  void writeKV(String& out) override { out += '"'; out += _key; out += "\":"; out += String(_code ? *_code : 0); }
 private:
  int* _code; RwTracked<int> _trk;
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
if(la===0&&lo===0)return;/* skip null island (no fix yet) — no jump from 0,0 to the first fix */
el._mk.setLatLng([la,lo]);
var t=el._tr.getLatLngs(),last=t.length?t[t.length-1]:null;
/* only extend the trail once moved ~5m — filters GPS drift while parked */
if(!last||Math.abs(la-last.lat)>0.00005||Math.abs(lo-last.lng)>0.00005){t.push([la,lo]);if(t.length>200)t.shift();el._tr.setLatLngs(t);}
/* optional geofence circle: parts 3,4,5 = home lat, home lon, radius (m) */
if(p.length>=5){var ha=parseFloat(p[2]),ho=parseFloat(p[3]),r=parseFloat(p[4]);
if(!isNaN(ha)&&(ha!==0||ho!==0)&&r>0){if(!el._fc){el._fc=L.circle([ha,ho],{radius:r,color:'#f59e0b',weight:1.5,fillColor:'#f59e0b',fillOpacity:.07}).addTo(el._m);}else{el._fc.setLatLng([ha,ho]);el._fc.setRadius(r);}}}
if(el._first){el._m.setView([la,lo],16);el._m.invalidateSize();el._first=0;}else{el._m.panTo([la,lo],{animate:true});}}
};)js";
class MapWidget : public Widget {
 public:
  MapWidget(const char* key, const char* title, float* lat, float* lon) : Widget(key, title), _lat(lat), _lon(lon) {}
  const char* typeId() const override { return "map"; }
  const char* css() const override { return RW_MAP_CSS; }
  const char* js() const override { return RW_MAP_JS; }
  // Draw a geofence circle on the map from a home point and a radius (metres):
  //   dash.map("Track", &lat, &lon).geofence(&homeLat, &homeLon, &radiusM);
  MapWidget& geofence(float* homeLat, float* homeLon, int* radiusM) {
    _hlat = homeLat; _hlon = homeLon; _hrad = radiusM; return *this;
  }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"rmap\"><div class=\"rmap-c\"></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override {
    float a = _lat ? *_lat : 0, b = _lon ? *_lon : 0;
    float ha = _hlat ? *_hlat : 0, ho = _hlon ? *_hlon : 0; int hr = _hrad ? *_hrad : 0;
    if (a != _la || b != _lo || ha != _hla || ho != _hlo || hr != _hr) {
      _la = a; _lo = b; _hla = ha; _hlo = ho; _hr = hr; return true;
    }
    return false;
  }
  void writeKV(String& out) override {
    char buf[64];
    if (_hlat)
      snprintf(buf, sizeof(buf), "%.5f,%.5f,%.5f,%.5f,%d", _lat ? *_lat : 0.0f, _lon ? *_lon : 0.0f,
               *_hlat, _hlon ? *_hlon : 0.0f, _hrad ? *_hrad : 0);
    else
      snprintf(buf, sizeof(buf), "%.5f,%.5f", _lat ? *_lat : 0.0f, _lon ? *_lon : 0.0f);
    out += '"'; out += _key; out += "\":\""; out += buf; out += '"';
  }
 private:
  float* _lat; float* _lon; float _la = 0, _lo = 0;
  float* _hlat = nullptr; float* _hlon = nullptr; int* _hrad = nullptr;
  float _hla = 0, _hlo = 0; int _hr = 0;
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
