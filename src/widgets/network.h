#pragma once
#include "base.h"

// ── Network map: a radial topology of the hub and its devices, à la a Zigbee gateway's mesh view.
// The hub is the star at the centre; each device is a node on a ring, joined by an edge coloured and
// labelled by its link quality (green strong · amber weak · red poor · grey offline). Hover a node for
// its details (name / address / link). Bind a String* that you fill with one device per record:
//   "name~emoji~online(0/1)~link(0..99)~addr;"  (records joined, no trailing needed)
static const char RW_NETWORK_CSS[] PROGMEM =
  ".netmap{position:relative;width:100%;margin-top:6px}"
  ".nm-canvas svg{width:100%;height:auto;display:block;overflow:visible}"
  ".nm-lbl{font:600 10px var(--font);fill:var(--ink3)}"
  ".nm-name{font:700 11px var(--font);fill:var(--ink1)}"
  ".nm-node{cursor:pointer}.nm-node:hover circle{stroke-width:3}"
  ".nm-nd{fill:var(--field)}";

static const char RW_NETWORK_JS[] PROGMEM =
  "R.W.network={update:function(el,v){"
  "var ds=(''+v).split(';').filter(function(x){return x;}).map(function(d){var p=d.split('~');"
  "return{n:p[0]||'',e:p[1]||'',o:p[2]==='1',l:+p[3]||0,a:p[4]||''};});"
  "var W=320,H=210,cx=W/2,cy=H/2,R=Math.min(cx,cy)-44,n=ds.length||1;"
  "var a0=n<=2?0:-Math.PI/2;"   // 2 nodes read better left/right than top/bottom
  "var pts=ds.map(function(d,i){var a=a0+i*2*Math.PI/n;return{x:cx+R*Math.cos(a),y:cy+R*Math.sin(a)};});"
  "var s='<svg viewBox=\"0 0 '+W+' '+H+'\"><defs><linearGradient id=\"nmg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\">'"
  "+'<stop offset=\"0\" stop-color=\"#22d3ee\"/><stop offset=\"1\" stop-color=\"#34d399\"/></linearGradient></defs>';"
  "ds.forEach(function(d,i){var p=pts[i];var c=!d.o?'#565f73':(d.l>60?'#34d399':(d.l>30?'#f5b94a':'#fb7185'));"
  "s+='<line x1=\"'+cx+'\" y1=\"'+cy+'\" x2=\"'+p.x+'\" y2=\"'+p.y+'\" stroke=\"'+c+'\" stroke-width=\"2\" opacity=\".65\"/>';"
  "s+='<text class=\"nm-lbl\" x=\"'+((cx+p.x)/2)+'\" y=\"'+((cy+p.y)/2-3)+'\" text-anchor=\"middle\">'+(d.o?d.l:'')+'</text>';});"
  "s+='<circle cx=\"'+cx+'\" cy=\"'+cy+'\" r=\"17\" fill=\"url(#nmg)\"/>'"
  "+'<text x=\"'+cx+'\" y=\"'+(cy+5)+'\" text-anchor=\"middle\" font-size=\"15\">\\u2605</text>';"
  "ds.forEach(function(d,i){var p=pts[i];"
  "s+='<g class=\"nm-node\"><title>'+d.n+(d.a?' ('+d.a+')':'')+' \\u2014 link '+d.l+(d.o?'':'  offline')+'</title>';"
  "s+='<circle class=\"nm-nd\" cx=\"'+p.x+'\" cy=\"'+p.y+'\" r=\"19\" stroke=\"'+(d.o?'#34d399':'#565f73')+'\" stroke-width=\"2\"/>';"
  "s+='<text x=\"'+p.x+'\" y=\"'+(p.y+6)+'\" text-anchor=\"middle\" font-size=\"17\">'+d.e+'</text>';"
  "s+='<text class=\"nm-name\" x=\"'+p.x+'\" y=\"'+(p.y+35)+'\" text-anchor=\"middle\">'+d.n+'</text></g>';});"
  "s+='</svg>';el.querySelector('.nm-canvas').innerHTML=s;}};";

class NetworkWidget : public Widget {
 public:
  NetworkWidget(const char* key, const char* title, String* data) : Widget(key, title), _data(data) {
    _size = RSIZE_L;   // needs vertical room
  }
  const char* typeId() const override { return "network"; }
  const char* css() const override { return RW_NETWORK_CSS; }
  const char* js() const override { return RW_NETWORK_JS; }
  void card(Print& out) override {
    cardOpen(out);
    out.print(F("<div class=\"netmap\"><div class=\"nm-canvas\"></div></div>"));
    cardClose(out);
  }
  bool hasState() const override { return true; }
  bool poll() override { return _trk.changed(_data ? *_data : String()); }
  void writeKV(String& out) override {
    out += '"'; out += _key; out += "\":\""; out += rwJsonEsc(_data ? *_data : String()); out += '"';
  }
 private:
  String* _data;
  RwTracked<String> _trk;
};
