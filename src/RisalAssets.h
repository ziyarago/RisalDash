#pragma once
#include <Arduino.h>

// Core UI assets in PROGMEM — the Risal brand shell (tokens + base + orbs + appbar +
// glass card grid). Ported from the dev/ mock. Per-widget CSS/JS get appended to this
// core in P2 (Zero-Waste aggregation); the WebSocket runtime arrives in P3.

// The <!DOCTYPE><html lang dir> open is printed by RisalUI (language/RTL aware).
static const char RISAL_HEAD[] PROGMEM =
  "<head><meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<meta name=\"theme-color\" content=\"#0F1115\"><title>RisalDash</title><style>";

static const char RISAL_CSS[] PROGMEM =
  "*{box-sizing:border-box;margin:0;padding:0}"
  "html{color-scheme:dark}html.light{color-scheme:light}"
  ":root{--bg0:oklch(0.16 0.022 255);--bg2:oklch(0.22 0.025 255);--bg3:oklch(0.28 0.025 255);--field:oklch(0.24 0.02 255 / .7);"
  "--ink1:oklch(0.97 0.01 255);--ink2:oklch(0.78 0.02 255);--ink3:oklch(0.62 0.022 255);"
  "--acc:oklch(0.82 0.14 200);--acc2:oklch(0.84 0.16 152);--acc-ink:oklch(0.26 0.06 220);"
  "--grad:linear-gradient(135deg,oklch(0.82 0.15 200),oklch(0.85 0.16 152));"
  "--glass:oklch(0.32 0.03 255 / .42);--glass2:oklch(0.40 0.03 255 / .30);--track:oklch(0.34 0.02 255 / .6);"
  "--line:oklch(0.92 0.01 255 / .14);--line2:oklch(0.92 0.01 255 / .26);--radius:22px;"
  "--font:-apple-system,\"SF Pro Display\",\"Segoe UI\",system-ui,Roboto,sans-serif;"
  "--mono:ui-monospace,SFMono-Regular,Consolas,monospace}"
  "html.light{--bg0:oklch(0.95 0.008 255);--bg2:oklch(1 0 0 / .7);--bg3:oklch(0.9 0.01 255);--field:oklch(1 0 0 / .6);"
  "--ink1:oklch(0.22 0.02 255);--ink2:oklch(0.42 0.02 255);--ink3:oklch(0.56 0.015 255);"
  "--glass:oklch(1 0 0 / .62);--glass2:oklch(1 0 0 / .5);--track:oklch(0.5 0.02 255 / .2);"
  "--line:oklch(0.25 0.02 255 / .12);--line2:oklch(0.25 0.02 255 / .2)}"
  "body{background:var(--bg0);color:var(--ink1);font-family:var(--font);min-height:100vh;letter-spacing:-.012em;"
  "padding:56px clamp(14px,3vw,26px) 52px;background-attachment:fixed;background-image:"
  "radial-gradient(58% 48% at 18% -4%,oklch(0.80 0.16 200 / .22),transparent 70%),"
  "radial-gradient(54% 50% at 92% 104%,oklch(0.84 0.17 152 / .20),transparent 70%),"
  "radial-gradient(48% 42% at 72% 26%,oklch(0.70 0.19 300 / .14),transparent 72%)}"
  "html.light body{background-image:radial-gradient(58% 48% at 18% -4%,oklch(0.82 0.13 200 / .2),transparent 70%),"
  "radial-gradient(54% 50% at 92% 104%,oklch(0.84 0.14 152 / .18),transparent 70%),"
  "radial-gradient(48% 42% at 72% 26%,oklch(0.72 0.16 300 / .12),transparent 72%)}"
  // effects(false) → .flat: drop orbs + appbar blur, keep the palette.
  ".flat .orbs{display:none}.flat body,html.flat body{background-image:none}"
  ".flat .appbar{backdrop-filter:none;-webkit-backdrop-filter:none;background:var(--bg2)}"
  ".orbs{display:none}"  /* vibrant gradients now live on body; orbs div kept harmless */
  ".appbar{position:sticky;top:56px;z-index:50;display:flex;align-items:center;gap:12px;margin-bottom:18px;"
  "padding:11px 15px;border-radius:20px;background:var(--glass);"
  "backdrop-filter:blur(26px) saturate(190%);-webkit-backdrop-filter:blur(26px) saturate(190%);"
  "border:1px solid var(--line2);box-shadow:0 10px 30px oklch(0 0 0 / .35),inset 0 1px 0 oklch(1 0 0 / .12)}"
  ".brand{font-weight:800;font-size:18px;letter-spacing:-.02em}"
  ".brand b{background:var(--grad);-webkit-background-clip:text;background-clip:text;color:transparent}"
  ".sp{flex:1}.pill{font:600 12.5px var(--font);padding:7px 12px;border-radius:999px;color:var(--ink2);"
  "border:1px solid var(--line2);cursor:pointer;background:transparent}"
  ".lng{display:flex;gap:3px}.lng a{font:600 11px var(--font);padding:6px 9px;border-radius:8px;color:var(--ink3);"
  "text-decoration:none;border:1px solid transparent}.lng a.on{color:var(--acc-ink);background:var(--grad)}"
  // z-index scale: orbs -1 · appbar(sticky) 30 · scrim 40 · drawer 50.
  ".burg{display:none;align-items:center;justify-content:center;width:34px;height:34px;border-radius:9px;"
  "border:1px solid var(--line2);background:transparent;color:var(--ink2);cursor:pointer}.burg svg{width:18px;height:18px}"
  ".gear{display:inline-flex;align-items:center;justify-content:center;width:38px;height:38px;border-radius:12px;"
  "border:1px solid var(--line2);background:var(--glass2);color:var(--ink2);cursor:pointer}.gear svg{width:19px;height:19px}"
  ".gear:hover{color:var(--ink1)}"
  ".scrim{position:fixed;inset:0;z-index:60;background:rgba(0,0,0,.5);opacity:0;pointer-events:none;transition:opacity .2s}"
  ".scrim.open{opacity:1;pointer-events:auto}"
  ".drawer{position:fixed;top:0;inset-inline-start:0;z-index:70;height:100%;width:260px;max-width:80vw;background:var(--bg2);"
  "border-inline-end:1px solid var(--line);padding:18px;transform:translateX(-100%);transition:transform .25s;overflow:auto}"
  "[dir=rtl] .drawer{transform:translateX(100%)}.drawer.open{transform:none}"
  ".drawer h4{font:700 11px var(--font);letter-spacing:.14em;text-transform:uppercase;color:var(--ink3);margin-bottom:10px}"
  ".drawer a{display:block;padding:10px 12px;border-radius:10px;color:var(--ink1);text-decoration:none;font:600 14px var(--font)}"
  ".drawer a:hover{background:var(--bg3)}@media(max-width:960px){.burg{display:inline-flex}}"
  // iOS-style cell grid: 2 columns on phone (4 on tablet+). Widgets pick a size preset —
  // s = 1 cell (square), m = full width, l = full width + tall. Height comes from per-size
  // min-height (not grid rows) so group headers stay compact; order is preserved (no dense).
  ".grid{display:grid;grid-template-columns:repeat(2,1fr);gap:13px}"
  ".card.s{grid-column:span 1;min-height:112px}.card.m{grid-column:span 2;min-height:92px}.card.l{grid-column:span 2;min-height:236px}"
  "@media(min-width:720px){.grid{grid-template-columns:repeat(4,1fr)}.card.m,.card.l{grid-column:span 2}}"
  ".card{background:var(--glass);border:1px solid var(--line);border-radius:var(--radius);padding:18px;display:flex;flex-direction:column;gap:12px;"
  "backdrop-filter:blur(22px) saturate(180%);-webkit-backdrop-filter:blur(22px) saturate(180%);"
  "box-shadow:0 8px 28px oklch(0 0 0 / .34),inset 0 1px 0 oklch(1 0 0 / .10)}"
  ".card h3{font:700 11px/1 var(--font);letter-spacing:.13em;text-transform:uppercase;color:var(--ink3);display:flex;align-items:center}"
  ".eb{display:inline-block;width:5px;height:5px;border-radius:50%;background:var(--acc);margin-inline-end:6px}"
  ".card h3 .ic{width:14px;height:14px;margin-inline-end:6px;fill:var(--acc);flex:none}"
  ".row{display:flex;align-items:baseline;gap:7px}"
  ".big{font:800 34px/1 var(--font);font-variant-numeric:tabular-nums;letter-spacing:-.03em}"
  ".unit{font-size:13px;color:var(--ink3);font-weight:500}"
  ".bar{height:7px;border-radius:99px;background:var(--track);overflow:hidden}"
  ".bar>i{display:block;height:100%;border-radius:99px;background:var(--grad)}"
  ".empty{grid-column:1/-1;text-align:center;color:var(--ink3);padding:48px 16px;font-size:14px}"
  // Multi-page layouts: switchable grids + bottom swipe-up sheet of icon tiles.
  // Horizontal pager: layout pages sit side-by-side and snap on swipe (left/right).
  ".lays{display:flex;overflow-x:auto;scroll-snap-type:x mandatory;-webkit-overflow-scrolling:touch;scrollbar-width:none;align-items:start;scroll-behavior:smooth}"
  ".lays::-webkit-scrollbar{display:none}.lay{flex:0 0 100%;min-width:0;scroll-snap-align:center}"
  // Page navigation strip under the appbar: [‹]  CURRENT PAGE  [›]. Sticky so it's always reachable.
  ".lnav{position:sticky;top:118px;z-index:49;display:flex;align-items:center;gap:9px;margin:-6px 2px 16px}"
  ".lnav-a{width:42px;height:42px;flex:none;border:1px solid var(--line2);border-radius:13px;background:var(--glass);"
  "backdrop-filter:blur(20px) saturate(180%);-webkit-backdrop-filter:blur(20px) saturate(180%);color:var(--ink1);"
  "display:grid;place-items:center;cursor:pointer}.lnav-a svg{width:20px;height:20px}.lnav-a:disabled{opacity:.25;pointer-events:none}"
  ".lnav-name{flex:1;height:42px;border:1px solid var(--line2);border-radius:13px;background:var(--glass);"
  "backdrop-filter:blur(20px) saturate(180%);-webkit-backdrop-filter:blur(20px) saturate(180%);color:var(--ink1);cursor:pointer;"
  "font:800 13px var(--font);letter-spacing:.17em;text-transform:uppercase}"
  ".lhandle{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);z-index:55;display:flex;align-items:center;gap:7px;"
  "padding:9px 16px 9px 14px;border-radius:999px;background:var(--glass);backdrop-filter:blur(20px) saturate(180%);"
  "-webkit-backdrop-filter:blur(20px) saturate(180%);border:1px solid var(--line2);color:var(--ink1);font:600 13px var(--font);"
  "cursor:pointer;box-shadow:0 10px 28px oklch(0 0 0 / .35)}.lhandle svg{width:16px;height:16px}"
  ".lscrim{position:fixed;inset:0;z-index:80;background:oklch(0 0 0 / .45);opacity:0;pointer-events:none;transition:opacity .25s}"
  ".lscrim.on{opacity:1;pointer-events:auto}"
  ".lsheet{position:fixed;left:0;right:0;bottom:0;z-index:90;background:var(--bg2);border-top:1px solid var(--line2);"
  "border-radius:26px 26px 0 0;padding:10px 18px 26px;transform:translateY(105%);transition:transform .3s cubic-bezier(.32,.72,0,1);"
  "box-shadow:0 -18px 50px oklch(0 0 0 / .45)}.lsheet.on{transform:none}"
  ".lgrip{width:42px;height:5px;border-radius:99px;background:var(--line2);margin:2px auto 16px}"
  ".ltiles{display:grid;grid-template-columns:repeat(auto-fill,minmax(92px,1fr));gap:10px}"
  ".ltile{display:flex;flex-direction:column;align-items:center;gap:9px;padding:16px 8px;border-radius:16px;background:var(--glass2);"
  "border:1px solid var(--line);color:var(--ink2);cursor:pointer;font:inherit}"
  ".ltile svg{width:26px;height:26px;fill:var(--ink2)}.ltile span{font:600 12px var(--font)}"
  ".ltile.on{border-color:transparent;background:linear-gradient(135deg,oklch(0.82 0.15 200 / .2),oklch(0.85 0.16 152 / .16));color:var(--ink1)}"
  ".ltile.on svg{fill:var(--acc2)}"
  // Always pinned to the bottom of the screen (fixed) so it shows on every page/layout.
  ".foot{position:fixed;left:0;right:0;bottom:0;z-index:44;text-align:center;color:var(--ink3);"
  "font:12px ui-monospace,Consolas,monospace;padding:9px 12px 11px;pointer-events:none;"
  "background:linear-gradient(to top,var(--bg0) 45%,transparent)}"
  ".foot b{background:var(--grad);-webkit-background-clip:text;background-clip:text;color:transparent}";

// In-page OS-style status bar (time + wifi/gsm/bt + fake battery). Self-injects CSS+HTML and ticks.
// Config: window.RSB_TZ (minutes), window.RSB_GSM, window.RSB_BT, window.RSB_BAT.
static const char RISAL_STATUSBAR_JS[] PROGMEM = R"js((function(){
var CSS=".rsb{position:fixed;top:0;left:0;right:0;height:46px;display:flex;align-items:center;justify-content:space-between;padding:0 22px 0 26px;color:#fff;font:600 15px var(--font);z-index:40;pointer-events:none;text-shadow:0 1px 2px rgba(0,0,0,.3)}.rsb::before{content:'';position:absolute;top:0;left:0;right:0;height:64px;z-index:-1;background:linear-gradient(to bottom,oklch(0.15 0.02 255 / .6),oklch(0.15 0.02 255 / .12) 60%,transparent);backdrop-filter:blur(15px) saturate(160%);-webkit-backdrop-filter:blur(15px) saturate(160%);-webkit-mask-image:linear-gradient(to bottom,#000 72%,transparent);mask-image:linear-gradient(to bottom,#000 72%,transparent)}.rsb .ind{display:flex;align-items:center;gap:7px}";
var HTML='<span class="rsb-time">9:41</span><span class="ind"><svg class="rsb-gsm" width="18" height="12" viewBox="0 0 18 12" fill="#fff" style="display:none"><rect x="0" y="8" width="3" height="4" rx="1"/><rect x="5" y="5" width="3" height="7" rx="1"/><rect x="10" y="2.5" width="3" height="9.5" rx="1"/><rect x="15" y="0" width="3" height="12" rx="1"/></svg><span class="rsb-rssi" style="display:none;font:600 12.5px var(--font);opacity:.9;margin-inline-end:2px">-</span><svg width="17" height="13" viewBox="0 0 17 13" fill="#fff"><path d="M8.5 0C5.3 0 2.4 1.2.2 3.2l1.5 1.6C3.5 3.1 5.9 2.1 8.5 2.1s5 1 6.8 2.7l1.5-1.6C14.6 1.2 11.7 0 8.5 0z"/><path d="M8.5 4.2c-2 0-3.8.8-5.1 2.1l1.6 1.6c.9-.9 2.1-1.4 3.5-1.4s2.6.5 3.5 1.4l1.6-1.6C12.3 5 10.5 4.2 8.5 4.2z"/><path d="M8.5 8.4c-.9 0-1.7.4-2.3 1l2.3 2.3 2.3-2.3c-.6-.6-1.4-1-2.3-1z"/></svg><svg class="rsb-bt" width="14" height="16" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round" style="display:none"><path d="M7 9l10 6-5 4V5l5 4-10 6"/></svg><svg class="rsb-batsvg" width="38" height="17" viewBox="0 0 38 17"><rect x="1" y="1.5" width="31" height="14" rx="4.5" fill="none" stroke="#fff" stroke-opacity=".5" stroke-width="1.4"/><rect class="rsb-fill" x="2.6" y="3" width="22" height="11" rx="3" fill="#34d399"/><rect x="33.5" y="5.5" width="2.6" height="6" rx="1.3" fill="#fff" fill-opacity=".5"/><text class="rsb-pct" x="16.5" y="12" text-anchor="middle" font-size="9.5" font-weight="800" fill="#fff" paint-order="stroke" stroke="#0a2a20" stroke-width="0.5">82</text></svg></span>';
var s=document.createElement("style");s.textContent=CSS;document.head.appendChild(s);
var bar=document.createElement("div");bar.className="rsb";bar.innerHTML=HTML;document.body.appendChild(bar);
if(window.RSB_GSM)bar.querySelector(".rsb-gsm").style.display="";
if(window.RSB_BT)bar.querySelector(".rsb-bt").style.display="";
var off=+(window.RSB_TZ||180);
function tick(){var n=new Date(),d=new Date(n.getTime()+n.getTimezoneOffset()*60000+off*60000);bar.querySelector(".rsb-time").textContent=d.getHours()+":"+String(d.getMinutes()).padStart(2,"0");}
tick();setInterval(tick,1000);
var rssiEl=bar.querySelector(".rsb-rssi");
function applyRssi(){var v=window.RSB_RSSI,show=localStorage.getItem("rd-rssi")!=="0";if(!show||v==null||v===0){rssiEl.style.display="none";return;}rssiEl.textContent=v+" dBm";rssiEl.style.display="";}
applyRssi();
window.RSB={setTz:function(m){off=+m;tick();},setRssi:function(v){window.RSB_RSSI=v;applyRssi();},applyRssi:applyRssi};
var real=!!window.RSB_BAT_REAL,bat=+(window.RSB_BAT||82),batSvg=bar.querySelector(".rsb-batsvg");
function drawBat(){var show=localStorage.getItem("rd-bat")!=="0";batSvg.style.display=show?"":"none";if(!show)return;var b=Math.max(0,Math.min(100,bat)),low=b<=20;bar.querySelector(".rsb-pct").textContent=Math.round(b);bar.querySelector(".rsb-fill").setAttribute("width",(27*b/100).toFixed(1));bar.querySelector(".rsb-fill").setAttribute("fill",low?"#ff453a":"#34d399");bar.querySelector(".rsb-pct").setAttribute("stroke",low?"#3a0a0a":"#0a2a20");}
drawBat();
if(!real)setInterval(function(){bat+=(Math.random()<0.72?-1:1);if(bat>100)bat=100;if(bat<6)bat=6;drawBat();},9000);
window.RSB.setBat=function(v){bat=+v;real=true;drawBat();};window.RSB.applyBat=drawBat;
})();)js";

// Global settings modal (appbar gear): language / theme / accent. Theme+accent persist to
// localStorage and apply live; language reloads (?lang=) so the server re-renders RTL+chrome.
// Localized labels via window.RSL; default accent index via window.RSACC.
static const char RISAL_SETTINGS_JS[] PROGMEM = R"js((function(){
var ACC=[['Aqua',200,152],['Blue',255,232],['Violet',300,278],['Amber',80,55],['Rose',18,350]];
var CSS=".sscrim{position:fixed;inset:0;z-index:100;background:oklch(0 0 0 / .5);backdrop-filter:blur(5px);-webkit-backdrop-filter:blur(5px);display:none;place-items:center;padding:20px}.sscrim.open{display:grid}.smodal{width:100%;max-width:360px;background:oklch(0.2 0.025 255 / .98);border:1px solid oklch(0.92 0.01 255 / .2);border-radius:22px;padding:20px;box-shadow:0 30px 80px oklch(0 0 0 / .6);color:oklch(0.95 0.01 255);font-family:var(--font);transform:scale(.96);transition:transform .2s}.sscrim.open .smodal{transform:none}.smodal .top{display:flex;align-items:center;justify-content:space-between;margin-bottom:6px}.smodal h3{font:800 18px var(--font)}.sx{width:30px;height:30px;border:none;border-radius:9px;background:oklch(0.32 0.02 255 / .7);color:oklch(0.8 0.02 255);font-size:16px;cursor:pointer}.srow{margin-top:16px}.srow .l{font:700 10.5px var(--font);letter-spacing:.13em;text-transform:uppercase;color:oklch(0.62 0.02 255);margin:0 2px 8px}.sseg{display:flex;gap:3px;background:oklch(0.14 0.02 255 / .7);border-radius:12px;padding:3px}.sseg button{flex:1;border:none;background:transparent;color:oklch(0.74 0.02 255);font:600 13px var(--font);padding:9px 0;border-radius:9px;cursor:pointer}.sseg button.on{background:linear-gradient(135deg,var(--acc),var(--acc2));color:var(--acc-ink)}.sacc{display:flex;gap:12px;padding:2px}.sacc button{width:30px;height:30px;border-radius:50%;border:2px solid transparent;cursor:pointer;padding:0}.sacc button.on{box-shadow:0 0 0 2px oklch(0.16 0.02 255),0 0 0 4px oklch(0.95 0.01 255)}.snote{margin-top:18px;font-size:11.5px;color:oklch(0.55 0.02 255);text-align:center}";
var D={set:'Settings',lang:'Language',theme:'Theme',accent:'Accent',note:'Saved · shown on every screen',dark:'Dark',light:'Light',auto:'Auto',signal:'Signal (dBm)',on:'On',off:'Off',battery:'Battery'};
var L=window.RSL||{};for(var k in D)if(L[k]==null)L[k]=D[k];
var accBtns=ACC.map(function(a,i){return '<button data-i="'+i+'" style="background:linear-gradient(135deg,oklch(0.82 0.15 '+a[1]+'),oklch(0.85 0.16 '+a[2]+'))"></button>';}).join('');
var html='<div class="smodal"><div class="top"><h3>'+L.set+'</h3><button class="sx" id="sx">✕</button></div>'+
'<div class="srow"><div class="l">'+L.lang+'</div><div class="sseg" id="slang"><button data-v="en">English</button><button data-v="ru">Русский</button><button data-v="ar">العربية</button></div></div>'+
'<div class="srow"><div class="l">'+L.theme+'</div><div class="sseg" id="stheme"><button data-v="dark">'+L.dark+'</button><button data-v="light">'+L.light+'</button><button data-v="auto">'+L.auto+'</button></div></div>'+
'<div class="srow"><div class="l">'+L.accent+'</div><div class="sacc" id="sacc">'+accBtns+'</div></div>'+
'<div class="srow"><div class="l">'+L.signal+'</div><div class="sseg" id="ssig"><button data-v="1">'+L.on+'</button><button data-v="0">'+L.off+'</button></div></div>'+
'<div class="srow"><div class="l">'+L.battery+'</div><div class="sseg" id="sbat"><button data-v="1">'+L.on+'</button><button data-v="0">'+L.off+'</button></div></div>'+
'<div class="snote">'+L.note+'</div></div>';
var st=document.createElement('style');st.textContent=CSS;document.head.appendChild(st);
var scrim=document.createElement('div');scrim.className='sscrim';scrim.innerHTML=html;document.body.appendChild(scrim);
var root=document.documentElement;
function mark(id,v){scrim.querySelectorAll('#'+id+' button').forEach(function(b){b.classList.toggle('on',b.dataset.v===v);});}
scrim.querySelectorAll('#slang button').forEach(function(b){b.addEventListener('click',function(){if(b.dataset.v!==root.getAttribute('lang'))location.search='?lang='+b.dataset.v;});});
mark('slang',root.getAttribute('lang'));
function applyTheme(v){var d=v==='auto'?matchMedia('(prefers-color-scheme:dark)').matches:v!=='light';root.classList.toggle('light',!d);root.classList.toggle('dark',d);}
scrim.querySelectorAll('#stheme button').forEach(function(b){b.addEventListener('click',function(){mark('stheme',b.dataset.v);localStorage.setItem('rd-th',b.dataset.v);applyTheme(b.dataset.v);});});
mark('stheme',localStorage.getItem('rd-th')||'dark');
function applyAcc(i){var a=ACC[i];root.style.setProperty('--acc','oklch(0.82 0.14 '+a[1]+')');root.style.setProperty('--acc2','oklch(0.84 0.16 '+a[2]+')');root.style.setProperty('--acc-ink','oklch(0.26 0.06 '+a[1]+')');root.style.setProperty('--grad','linear-gradient(135deg,oklch(0.82 0.15 '+a[1]+'),oklch(0.85 0.16 '+a[2]+'))');scrim.querySelectorAll('#sacc button').forEach(function(x){x.classList.toggle('on',+x.dataset.i===i);});}
scrim.querySelectorAll('#sacc button').forEach(function(b){b.addEventListener('click',function(){var i=+b.dataset.i;localStorage.setItem('rd-acc',i);applyAcc(i);});});
var ai=localStorage.getItem('rd-acc');applyAcc(ai!=null?+ai:(window.RSACC||0));
scrim.querySelectorAll('#ssig button').forEach(function(b){b.addEventListener('click',function(){mark('ssig',b.dataset.v);localStorage.setItem('rd-rssi',b.dataset.v);if(window.RSB&&window.RSB.applyRssi)window.RSB.applyRssi();});});
mark('ssig',localStorage.getItem('rd-rssi')==='0'?'0':'1');
scrim.querySelectorAll('#sbat button').forEach(function(b){b.addEventListener('click',function(){mark('sbat',b.dataset.v);localStorage.setItem('rd-bat',b.dataset.v);if(window.RSB&&window.RSB.applyBat)window.RSB.applyBat();});});
mark('sbat',localStorage.getItem('rd-bat')==='0'?'0':'1');
function open(o){scrim.classList.toggle('open',o);}
scrim.querySelector('#sx').addEventListener('click',function(){open(false);});
scrim.addEventListener('click',function(e){if(e.target===scrim)open(false);});
window.RSettings={open:function(){open(true);}};
})();)js";

// Multi-page layout switcher: swipe up from the bottom edge (or tap the handle) to open the
// sheet; a tile click shows that page client-side. Emitted only when layout()s are declared.
static const char RISAL_LAYOUTS_JS[] PROGMEM = R"js((function(){
var sheet=document.querySelector('.lsheet'),scrim=document.querySelector('.lscrim'),lays=document.querySelector('.lays');
var nameEl=document.getElementById('lnavN'),aL=document.getElementById('lnavL'),aR=document.getElementById('lnavR');
if(!sheet||!lays)return;
var N=lays.querySelectorAll('.lay').length;
function set(o){sheet.classList.toggle('on',o);scrim.classList.toggle('on',o);}
function nm(i){var s=document.querySelector('.ltile[data-lay="'+i+'"] span');return s?s.textContent:'';}
function cur(){return Math.max(0,Math.min(N-1,Math.round(lays.scrollLeft/lays.clientWidth)));}
function mark(i){document.querySelectorAll('.ltile').forEach(function(t){t.classList.toggle('on',+t.dataset.lay===i);});
  if(nameEl)nameEl.textContent=nm(i);if(aL)aL.disabled=i<=0;if(aR)aR.disabled=i>=N-1;}
function go(i){i=Math.max(0,Math.min(N-1,i));lays.scrollTo({left:i*lays.clientWidth,behavior:'smooth'});mark(i);set(false);window.scrollTo(0,0);}
document.querySelectorAll('.ltile').forEach(function(t){t.addEventListener('click',function(){go(+t.dataset.lay);});});
var tmr;lays.addEventListener('scroll',function(){clearTimeout(tmr);tmr=setTimeout(function(){mark(cur());},90);},{passive:true});
window.RL={open:function(o){set(!!o);},go:go,cur:cur};
var a=+(lays.dataset.active||0);requestAnimationFrame(function(){lays.scrollLeft=a*lays.clientWidth;mark(a);});
})();)js";

// </style></head><body>; the theme-resolve script is printed by RisalUI between
// OPEN and CHROME (first body child → applies the saved/auto theme before paint).
static const char RISAL_BODY_OPEN[] PROGMEM = "</style></head><body>";
static const char RISAL_BODY_CHROME[] PROGMEM =
  "<div class=\"orbs\"><i class=\"orb a\"></i><i class=\"orb b\"></i></div>"
  "<header class=\"appbar\"><div class=\"brand\"><b>RisalDash</b> &middot; ";

// title is printed between OPEN and MID. The language switcher + theme toggle
// (RISAL_APPBAR_END) are printed by RisalUI after MID, so the active language can be marked.
static const char RISAL_BODY_MID[] PROGMEM =
  "</div><span class=\"sp\"></span>";

static const char RISAL_APPBAR_END[] PROGMEM =
  "<button class=\"gear\" onclick=\"RSettings.open()\" aria-label=\"Settings\">"
  "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
  "<circle cx=\"12\" cy=\"12\" r=\"3\"/>"
  "<path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z\"/></svg></button></header>";

// Global SVG gradient defs (shared by gauge arc + chart line/fill). Printed once, hidden.
static const char RISAL_DEFS[] PROGMEM =
  "<svg width=\"0\" height=\"0\" style=\"position:absolute\"><defs>"
  "<linearGradient id=\"rg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\"><stop offset=\"0\" style=\"stop-color:var(--acc)\"/><stop offset=\"1\" style=\"stop-color:var(--acc2)\"/></linearGradient>"
  "<linearGradient id=\"rgf\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\"><stop offset=\"0\" style=\"stop-color:var(--acc);stop-opacity:.25\"/><stop offset=\"1\" style=\"stop-color:var(--acc2);stop-opacity:0\"/></linearGradient>"
  "</defs></svg>";

static const char RISAL_EMPTY[] PROGMEM =
  "<div class=\"empty\">No widgets declared yet &mdash; add dash.metric(&hellip;), dash.toggle(&hellip;) in setup().</div>";

// Footer: "served by ESP" tail is localized by RisalUI between these two.
static const char RISAL_BODY_FOOT[] PROGMEM =
  "<footer class=\"foot\">RisalDash v" RISALDASH_VERSION " &middot; ";
static const char RISAL_FOOT_END[] PROGMEM = "</footer>";

static const char RISAL_SCRIPT_OPEN[] PROGMEM = "<script>";

// Client runtime: WebSocket connect + apply diffs to widgets + send commands.
static const char RISAL_RUNTIME_JS[] PROGMEM =
  "window.R={W:{},ws:null,"
  "send:function(k,v){var o={};o[k]=v;if(this.ws&&this.ws.readyState===1)this.ws.send(JSON.stringify(o));},"
  "apply:function(st){for(var k in st){var e=document.querySelectorAll('[data-key=\"'+k+'\"]');"
  "for(var i=0;i<e.length;i++){var w=R.W[e[i].dataset.type];if(w&&w.update)w.update(e[i],st[k]);}}},"
  "connect:function(){var s=this,p=location.protocol==='https:'?'wss':'ws';"
  "var ws=new WebSocket(p+'://'+location.host+'/ws');this.ws=ws;"
  "ws.onmessage=function(ev){try{R.apply(JSON.parse(ev.data));}catch(_){}};"
  "ws.onclose=function(){setTimeout(function(){s.connect();},800);};}};";

static const char RISAL_INIT_JS[] PROGMEM =
  "document.addEventListener('DOMContentLoaded',function(){"
  "var e=document.querySelectorAll('[data-key]');"
  "for(var i=0;i<e.length;i++){var w=R.W[e[i].dataset.type];if(w&&w.init)w.init(e[i]);}"
  "R.connect();});</script>";

static const char RISAL_HTML_END[] PROGMEM = "</body></html>";

// ── First-boot Wi-Fi provisioning portal (OKLCH glass + signal levels + custom tz select) ──
static const char RISAL_PORTAL_CSS[] PROGMEM =
  ".wrap{max-width:400px;margin:0 auto;padding:4vh 0}.pc{padding:22px}"
  ".pc h2{font:800 21px var(--font);margin-bottom:2px}.pc h2 b{background:var(--grad);-webkit-background-clip:text;background-clip:text;color:transparent}"
  ".pc p.s{color:var(--ink3);font-size:13px;margin:6px 0 4px}"
  ".l{font:700 10.5px var(--font);letter-spacing:.14em;text-transform:uppercase;color:var(--ink3);margin:14px 4px 8px}"
  ".nets{display:flex;flex-direction:column;gap:7px}"
  ".net{display:flex;align-items:center;gap:11px;padding:12px 13px;border-radius:14px;cursor:pointer;width:100%;text-align:start;"
  "background:var(--glass2);border:1px solid var(--line);color:var(--ink1);font:inherit}"
  ".net.on{border-color:transparent;background:linear-gradient(135deg,oklch(0.82 0.15 200 / .22),oklch(0.85 0.16 152 / .18));box-shadow:inset 0 0 0 1.5px oklch(0.84 0.16 175 / .55)}"
  ".net .nm{flex:1;font-size:15px;font-weight:600}"
  ".net svg{width:18px;height:18px;fill:none;stroke:var(--ink2);stroke-width:2;stroke-linecap:round}"
  ".net.on>svg:first-of-type{stroke:var(--acc2)}.net .lock{width:14px;height:14px;stroke:var(--ink3)}"
  ".check{width:22px;height:22px;border-radius:50%;background:var(--grad);display:none;place-items:center}"
  ".net.on .check{display:grid}.net.on .check svg{width:13px;height:13px;stroke:#06281f;stroke-width:3.4}"
  ".inp{width:100%;height:48px;border-radius:14px;margin-top:14px;padding:0 14px;background:var(--glass2);border:1px solid var(--line);color:var(--ink1);font:15px var(--font)}"
  ".inp::placeholder{color:var(--ink3)}"
  ".pwrap{position:relative;margin-top:14px}.pwrap .inp{margin-top:0;padding-inline-end:48px}"
  ".eye{position:absolute;inset-inline-end:5px;top:50%;transform:translateY(-50%);width:40px;height:40px;display:grid;"
  "place-items:center;background:none;border:none;cursor:pointer;color:var(--ink3);border-radius:11px}"
  ".eye:active{color:var(--ink1)}.eye svg{width:21px;height:21px}.eye.on{color:var(--acc2)}"
  ".tzc{position:relative;margin-top:14px}"
  ".tzsel{width:100%;height:48px;border-radius:14px;padding:0 14px;display:flex;align-items:center;justify-content:space-between;background:var(--glass2);border:1px solid var(--line);color:var(--ink1);font:15px var(--font);cursor:pointer}"
  ".tzsel svg{width:15px;height:15px;fill:none;stroke:var(--ink3);stroke-width:2;transition:transform .2s}.tzc.open .tzsel svg{transform:rotate(180deg)}"
  ".tzpop{position:absolute;left:0;right:0;top:56px;z-index:120;display:none;flex-direction:column;gap:2px;padding:6px;background:var(--bg2);border:1px solid var(--line2);border-radius:14px;box-shadow:0 18px 44px rgba(0,0,0,.5);max-height:232px;overflow:auto}"
  ".tzc.open .tzpop{display:flex}"
  ".tzpop button{text-align:start;padding:11px 12px;border:none;background:transparent;color:var(--ink1);font:15px var(--font);border-radius:10px;cursor:pointer}"
  ".tzpop button.on{background:linear-gradient(135deg,oklch(0.82 0.15 200 / .24),oklch(0.85 0.16 152 / .2));color:var(--acc2);font-weight:600}"
  ".act{width:100%;height:50px;border:none;border-radius:15px;margin-top:16px;cursor:pointer;background:var(--grad);color:var(--acc-ink);font:800 15px var(--font);box-shadow:0 10px 26px oklch(0.82 0.15 195 / .35)}";

// Portal client JS: network row selection + custom timezone dropdown (full zone list).
static const char RISAL_PORTAL_JS[] PROGMEM = R"js((function(){
var ssid=document.getElementById('ssid'),pw=document.getElementById('pw');
document.querySelectorAll('.net').forEach(function(b){b.addEventListener('click',function(){
  document.querySelectorAll('.net').forEach(function(x){x.classList.remove('on');});b.classList.add('on');ssid.value=b.dataset.s;
  if(pw){pw.focus();pw.scrollIntoView({block:'center'});}});});
var eye=document.getElementById('eye');
if(eye&&pw){eye.addEventListener('click',function(){var show=pw.type==='password';pw.type=show?'text':'password';eye.classList.toggle('on',show);var sl=eye.querySelector('.sl');if(sl)sl.style.display=show?'':'none';pw.focus();});}
var TZ=[[-720,'-12:00 Baker I.'],[-660,'-11:00 Niue'],[-600,'-10:00 Honolulu'],[-570,'-09:30 Marquesas'],[-540,'-09:00 Anchorage'],[-480,'-08:00 Los Angeles'],[-420,'-07:00 Denver'],[-360,'-06:00 Mexico City'],[-300,'-05:00 New York'],[-240,'-04:00 Santiago'],[-210,'-03:30 St. John’s'],[-180,'-03:00 Sao Paulo'],[-120,'-02:00 S. Georgia'],[-60,'-01:00 Azores'],[0,'+00:00 London / UTC'],[60,'+01:00 Berlin'],[120,'+02:00 Cairo'],[180,'+03:00 Riyadh'],[210,'+03:30 Tehran'],[240,'+04:00 Dubai'],[270,'+04:30 Kabul'],[300,'+05:00 Tashkent'],[330,'+05:30 Mumbai'],[345,'+05:45 Kathmandu'],[360,'+06:00 Dhaka'],[390,'+06:30 Yangon'],[420,'+07:00 Bangkok'],[480,'+08:00 Singapore'],[525,'+08:45 Eucla'],[540,'+09:00 Tokyo'],[570,'+09:30 Adelaide'],[600,'+10:00 Sydney'],[630,'+10:30 Lord Howe'],[660,'+11:00 Solomon Is.'],[720,'+12:00 Auckland'],[765,'+12:45 Chatham'],[780,'+13:00 Samoa'],[840,'+14:00 Kiribati']];
var c=document.getElementById('tzc'),btn=c.querySelector('.tzsel'),val=document.getElementById('tzval'),pop=document.getElementById('tzpop'),hid=document.getElementById('tzv');
var DEF=+hid.value;
TZ.forEach(function(z){var b=document.createElement('button');b.type='button';b.textContent=z[1];if(z[0]===DEF){b.className='on';val.textContent=z[1];}
  b.addEventListener('click',function(e){e.stopPropagation();val.textContent=z[1];hid.value=z[0];pop.querySelectorAll('button').forEach(function(x){x.classList.remove('on');});b.classList.add('on');c.classList.remove('open');if(window.RSB)RSB.setTz(z[0]);});
  pop.appendChild(b);});
btn.addEventListener('click',function(e){e.stopPropagation();var o=c.classList.toggle('open');if(o){var s=pop.querySelector('.on');if(s)s.scrollIntoView({block:'center'});}});
document.addEventListener('click',function(){c.classList.remove('open');});
})();)js";
