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
  ":root{--bg0:#0F1115;--bg2:#1B1F27;--bg3:#232834;--field:#20242E;--ink1:#F2F4F8;--ink2:#AEB6C2;--ink3:#6B7280;"
  "--acc:#22D3EE;--acc2:#34D399;--acc-ink:#04262C;--grad:linear-gradient(135deg,#22D3EE,#34D399);"
  "--line:rgba(255,255,255,.08);--line2:rgba(255,255,255,.16);--radius:18px;"
  "--font:-apple-system,\"Segoe UI\",system-ui,Roboto,sans-serif;"
  "--mono:ui-monospace,SFMono-Regular,Consolas,monospace}"
  "html.light{--bg0:#E7EBF3;--bg2:#fff;--bg3:#E2E7F0;--field:#fff;--ink1:#0E1420;--ink2:#4A5462;--ink3:#8A93A3;"
  "--line:rgba(14,20,32,.09);--line2:rgba(14,20,32,.16)}"
  "body{background:var(--bg0);color:var(--ink1);font-family:var(--font);min-height:100vh;letter-spacing:-.01em;"
  "padding:clamp(14px,3vw,30px);background-image:radial-gradient(1100px 600px at 50% -10%,rgba(34,211,238,.06),transparent),"
  "radial-gradient(900px 500px at 80% 110%,rgba(52,211,153,.05),transparent);background-attachment:fixed}"
  // effects(false) → .flat: drop orbs + appbar blur, keep the palette.
  ".flat .orbs{display:none}.flat body,html.flat body{background-image:none}"
  ".flat .appbar{backdrop-filter:none;-webkit-backdrop-filter:none;background:var(--bg2)}"
  ".orbs{position:fixed;inset:0;z-index:-1;overflow:hidden;background:var(--bg0)}"
  ".orb{position:absolute;border-radius:50%;filter:blur(70px)}"
  ".orb.a{width:460px;height:460px;top:-120px;right:-90px;background:rgba(34,211,238,.22)}"
  ".orb.b{width:420px;height:420px;bottom:-120px;left:-110px;background:rgba(52,211,153,.18)}"
  ".appbar{position:sticky;top:8px;z-index:30;display:flex;align-items:center;gap:12px;margin-bottom:18px;"
  "padding:9px 14px;border-radius:16px;background:linear-gradient(135deg,rgba(255,255,255,.10),rgba(255,255,255,.04));"
  "backdrop-filter:blur(18px) saturate(160%);-webkit-backdrop-filter:blur(18px) saturate(160%);"
  "border:1px solid var(--line2);box-shadow:0 8px 24px rgba(0,0,0,.28)}"
  ".brand{font-weight:800;font-size:18px;letter-spacing:-.02em}"
  ".brand b{background:var(--grad);-webkit-background-clip:text;background-clip:text;color:transparent}"
  ".sp{flex:1}.pill{font:600 12.5px var(--font);padding:7px 12px;border-radius:999px;color:var(--ink2);"
  "border:1px solid var(--line2);cursor:pointer;background:transparent}"
  ".lng{display:flex;gap:3px}.lng a{font:600 11px var(--font);padding:6px 9px;border-radius:8px;color:var(--ink3);"
  "text-decoration:none;border:1px solid transparent}.lng a.on{color:var(--acc-ink);background:var(--grad)}"
  // z-index scale: orbs -1 · appbar(sticky) 30 · scrim 40 · drawer 50.
  ".burg{display:none;align-items:center;justify-content:center;width:34px;height:34px;border-radius:9px;"
  "border:1px solid var(--line2);background:transparent;color:var(--ink2);cursor:pointer}.burg svg{width:18px;height:18px}"
  ".scrim{position:fixed;inset:0;z-index:40;background:rgba(0,0,0,.5);opacity:0;pointer-events:none;transition:opacity .2s}"
  ".scrim.open{opacity:1;pointer-events:auto}"
  ".drawer{position:fixed;top:0;inset-inline-start:0;z-index:50;height:100%;width:260px;max-width:80vw;background:var(--bg2);"
  "border-inline-end:1px solid var(--line);padding:18px;transform:translateX(-100%);transition:transform .25s;overflow:auto}"
  "[dir=rtl] .drawer{transform:translateX(100%)}.drawer.open{transform:none}"
  ".drawer h4{font:700 11px var(--font);letter-spacing:.14em;text-transform:uppercase;color:var(--ink3);margin-bottom:10px}"
  ".drawer a{display:block;padding:10px 12px;border-radius:10px;color:var(--ink1);text-decoration:none;font:600 14px var(--font)}"
  ".drawer a:hover{background:var(--bg3)}@media(max-width:960px){.burg{display:inline-flex}}"
  ".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(228px,1fr));gap:15px;align-items:start}"
  ".sp2{grid-column:span 2}.sp3{grid-column:span 3}@media(max-width:560px){.sp2,.sp3{grid-column:auto}}"
  ".card{background:var(--bg2);border:1px solid var(--line);border-radius:var(--radius);padding:17px;display:flex;flex-direction:column;gap:12px}"
  ".card h3{font:700 11px/1 var(--font);letter-spacing:.13em;text-transform:uppercase;color:var(--ink3);display:flex;align-items:center}"
  ".eb{display:inline-block;width:5px;height:5px;border-radius:50%;background:var(--acc);margin-inline-end:6px}"
  ".row{display:flex;align-items:baseline;gap:7px}"
  ".big{font:800 34px/1 var(--font);font-variant-numeric:tabular-nums;letter-spacing:-.03em}"
  ".unit{font-size:13px;color:var(--ink3);font-weight:500}"
  ".bar{height:6px;border-radius:99px;background:var(--bg3);overflow:hidden}"
  ".bar>i{display:block;height:100%;border-radius:99px;background:var(--grad)}"
  ".empty{grid-column:1/-1;text-align:center;color:var(--ink3);padding:48px 16px;font-size:14px}"
  ".foot{margin-top:22px;text-align:center;color:var(--ink3);font:12px ui-monospace,Consolas,monospace}"
  ".foot b{background:var(--grad);-webkit-background-clip:text;background-clip:text;color:transparent}";

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
  "<button class=\"pill\" onclick=\"R.theme()\">Theme</button></header>";

// Global SVG gradient defs (shared by gauge arc + chart line/fill). Printed once, hidden.
static const char RISAL_DEFS[] PROGMEM =
  "<svg width=\"0\" height=\"0\" style=\"position:absolute\"><defs>"
  "<linearGradient id=\"rg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\"><stop offset=\"0\" stop-color=\"#22D3EE\"/><stop offset=\"1\" stop-color=\"#34D399\"/></linearGradient>"
  "<linearGradient id=\"rgf\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\"><stop offset=\"0\" stop-color=\"#22D3EE\" stop-opacity=\".25\"/><stop offset=\"1\" stop-color=\"#34D399\" stop-opacity=\"0\"/></linearGradient>"
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

// ── First-boot Wi-Fi provisioning portal ──
static const char RISAL_PORTAL_CSS[] PROGMEM =
  ".wrap{max-width:420px;margin:8vh auto}.pc h2{font:800 22px var(--font);margin-bottom:6px}"
  ".pc p.s{color:var(--ink3);font-size:13px;margin-bottom:14px}"
  ".pc form{display:flex;flex-direction:column;gap:12px}"
  ".pc select,.pc input{height:46px;border-radius:12px;border:1px solid var(--line2);background:var(--field);color:var(--ink1);font:15px var(--font);padding:0 12px}"
  ".pc .act{height:48px;border:none;border-radius:13px;background:var(--grad);color:var(--acc-ink);font:700 15px var(--font);cursor:pointer}";

// Portal chrome strings (subtitle / password placeholder / connect) are localized by
// RisalUI between these segments.
static const char RISAL_PORTAL_OPEN[] PROGMEM =
  "</style></head><body><div class=\"orbs\"><i class=\"orb a\"></i><i class=\"orb b\"></i></div>"
  "<div class=\"wrap\"><div class=\"card pc\"><h2><span class=\"brand\"><b>Risal</b></span>Dash</h2>"
  "<p class=\"s\">";
static const char RISAL_PORTAL_FORM[] PROGMEM =
  "</p><form action=\"/connect\" method=\"get\"><select name=\"ssid\">";
static const char RISAL_PORTAL_PASS[] PROGMEM =
  "</select><input type=\"password\" name=\"pass\" placeholder=\"";
static const char RISAL_PORTAL_BTN[] PROGMEM =
  "\" autocomplete=\"off\"><button class=\"act\" type=\"submit\">";
static const char RISAL_PORTAL_END[] PROGMEM =
  "</button></form></div></div></body></html>";
