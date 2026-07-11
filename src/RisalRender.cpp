// Dashboard page render: streams the whole page through a windowing Print sink
// (chunked response) — widget CSS/JS emitted once per type (Zero-Waste).
#include "RisalUI.h"
#include "RisalAssets.h"
#include "RisalInternal.h"

// Stream the page in segments (no giant String in RAM). CSS for each widget type is
// emitted once — Zero-Waste, since unused widget classes are dropped by the linker.
void RisalUI::_handleRoot(AsyncWebServerRequest* req) {
  // Effective language: ?lang= overrides the configured default (live appbar switcher).
  String want = req->hasParam("lang") ? req->getParam("lang")->value() : String(_langCode);
  const char* eff = "en";
  bool rtl = false;
  if (want == "ru") eff = "ru";
  else if (want == "uz") eff = "uz";
  else if (want == "ar") { eff = "ar"; rtl = true; }
  int active = req->hasParam("layout") ? req->getParam("layout")->value().toInt() : 0;
  req->sendChunked("text/html", [this, eff, rtl, active](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
    WindowPrint w(buf, maxLen, index);
    _renderRoot(w, eff, rtl, active);
    return w.produced();
  });
}

void RisalUI::_renderRoot(Print& out, const char* eff, bool rtl, int active) {
  g_rActiveLang = eff;  // author-string translator (if any) resolves against this request's language
  out.print(F("<!DOCTYPE html><html class=\""));
  out.print(_effects ? "dark" : "dark flat");
  out.print(F("\" lang=\""));
  out.print(eff);
  out.print(F("\" dir=\""));
  out.print(rtl ? "rtl" : "ltr");
  out.print(F("\">"));
  out.print(FPSTR(RISAL_HEAD));
  out.print(_title);  // browser tab = the device, not the brand
  out.print(FPSTR(RISAL_HEAD_END));
  out.print(FPSTR(RISAL_CSS));

  const char* seen[RISAL_MAX_WIDGETS];
  uint8_t sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* c = _widgets[i]->css();
    if (!c) continue;
    bool dup = false;
    for (uint8_t j = 0; j < sc; j++)
      if (seen[j] == c) { dup = true; break; }
    if (!dup) { seen[sc++] = c; out.print(FPSTR(c)); }
  }

  uint8_t groups = 0, layCount = 0;
  bool hasTabs = false;
  for (uint8_t i = 0; i < _count; i++) {
    if (strcmp(_widgets[i]->typeId(), "group") == 0) groups++;
    if (strcmp(_widgets[i]->typeId(), "tab") == 0) hasTabs = true;
    if (strcmp(_widgets[i]->typeId(), "layout") == 0) layCount++;
  }

  out.print(FPSTR(RISAL_BODY_OPEN));
  // Resolve theme before paint: saved choice → configured mode → prefers-color-scheme (AUTO).
  out.print(F("<script>(function(){var m='"));
  out.print(_theme == LIGHT ? "light" : _theme == AUTO ? "auto" : "dark");
  out.print(F("';var s=localStorage.getItem('rd-th')||m;"
               "var d=s==='dark'?true:s==='light'?false:matchMedia('(prefers-color-scheme:dark)').matches;"
               "var c=document.documentElement.classList;c.toggle('light',!d);c.toggle('dark',d);})();</script>"));
  out.print(FPSTR(RISAL_BODY_CHROME));
  // Appbar brand: a custom wordmark (dash.brand("Risal<b>Hub</b>")) or the default "RisalDash · <title>".
  if (_brand) out.print(_brand);
  else { out.print(F("<b>RisalDash</b> &middot; ")); out.print(_title); }
  out.print(FPSTR(RISAL_BODY_MID));
  // Hamburger → the nav drawer (mobile only). Only for single-page dashboards with groups:
  // with layout() pages the swipe pager + nav strip own navigation, a drawer would double it.
  if (groups && !layCount)
    out.print(F("<button class=\"burg\" onclick=\"R.openNav(true)\"><svg viewBox=\"0 0 24 24\" fill=\"none\" "
                "stroke=\"currentColor\" stroke-width=\"2\"><path d=\"M4 6h16M4 12h16M4 18h16\"/></svg></button>"));
  // Language / theme / accent now live in the Settings modal (appbar gear → RISAL_APPBAR_END).
  out.print(FPSTR(RISAL_APPBAR_END));
  // Nav drawer: scrim + off-canvas list of groups (anchors to each section header).
  if (groups && !layCount) {
    out.print(F("<div class=\"scrim\" onclick=\"R.openNav(false)\"></div><aside class=\"drawer\"><h4>"));
    out.print(_title);
    out.print(F("</h4>"));
    for (uint8_t i = 0; i < _count; i++) {
      if (strcmp(_widgets[i]->typeId(), "group") != 0) continue;
      out.print(F("<a href=\"#g-"));
      rwSlug(out, _widgets[i]->key());
      out.print(F("\" onclick=\"R.openNav(false)\">"));
      out.print(rI18n(_widgets[i]->key()));
      out.print(F("</a>"));
    }
    out.print(F("</aside>"));
  }
  out.print(FPSTR(RISAL_DEFS));

  if (_count == 0) {
    out.print(F("<main class=\"grid\">"));
    out.print(FPSTR(RISAL_EMPTY));
    out.print(F("</main>"));
  } else if (layCount > 0) {
    // Multi-page mode: swipe/arrow between pages. `active` (the selected page) is passed in — the
    // request object isn't available during chunked rendering.
    // Nav strip under the appbar: [‹]  PAGE NAME  [›]  (name taps open the tile sheet).
    // Skipped when a bottom tab bar is chosen (dash.bottomNav()).
    if (!_bottomNav)
      out.print(F("<nav class=\"lnav\"><button class=\"lnav-a\" id=\"lnavL\" aria-label=\"prev\" onclick=\"RL.go(RL.cur()-1)\">"
                 "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2.4\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M15 18l-6-6 6-6\"/></svg></button>"
                 "<button class=\"lnav-name\" id=\"lnavN\" onclick=\"RL.open(1)\"></button>"
                 "<button class=\"lnav-a\" id=\"lnavR\" aria-label=\"next\" onclick=\"RL.go(RL.cur()+1)\">"
                 "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2.4\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M9 6l6 6-6 6\"/></svg></button></nav>"));
    uint8_t i = 0;
    // Cards before the first layout() are pinned (visible on every page).
    if (strcmp(_widgets[0]->typeId(), "layout") != 0) {
      out.print(F("<main class=\"grid\">"));
      for (; i < _count && strcmp(_widgets[i]->typeId(), "layout") != 0; i++)
        if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
      out.print(F("</main>"));
    }
    out.print(F("<div class=\"lays\" data-active=\""));
    out.print(active);
    out.print(F("\">"));
    int li = -1;
    bool open = false, skip = false;
    for (; i < _count; i++) {
      if (strcmp(_widgets[i]->typeId(), "layout") == 0) {
        if (open) { out.print(F("</main>")); open = false; }
        // A page hidden by visibleWhen(): emit no <main>, skip its widgets, don't consume an index.
        if (!static_cast<LayoutWidget*>(_widgets[i])->visible()) { skip = true; continue; }
        skip = false;
        li++;
        out.print(F("<main class=\"grid lay\" data-lay=\""));
        out.print(li);
        out.print(F("\">"));
        open = true;
      } else if (!skip && !_widgets[i]->isSetting()) {
        _widgets[i]->card(out);
      }
    }
    if (open) out.print(F("</main>"));
    out.print(F("</div>"));
    // Tile sheet (opened by tapping the page name in the nav strip) + its scrim.
    out.print(F("<div class=\"lscrim\" onclick=\"RL.open(0)\"></div><div class=\"lsheet\"><div class=\"lgrip\"></div>"
                 "<div class=\"ltiles\">"));
    li = -1;
    for (uint8_t k = 0; k < _count; k++) {
      if (strcmp(_widgets[k]->typeId(), "layout") != 0) continue;
      LayoutWidget* lw = static_cast<LayoutWidget*>(_widgets[k]);
      if (!lw->visible()) continue;   // hidden page: no switcher tile, no index
      li++;
      out.print(F("<button class=\"ltile"));
      if (li == active) out.print(F(" on"));
      out.print(F("\" data-lay=\""));
      out.print(li);
      out.print(F("\"><svg viewBox=\"0 0 24 24\"><path d=\""));
      out.print(FPSTR(lw->iconPath() ? lw->iconPath() : RICON_HOME));
      out.print(F("\"/></svg><span>"));
      out.print(rI18n(_widgets[k]->key()));
      out.print(F("</span></button>"));
    }
    out.print(F("</div></div>"));
    // Bottom tab bar (dash.bottomNav()) — one tab per visible page, icon + name, syncs with the pager.
    if (_bottomNav) {
      out.print(F("<nav class=\"botnav\">"));
      int bi = -1;
      for (uint8_t k = 0; k < _count; k++) {
        if (strcmp(_widgets[k]->typeId(), "layout") != 0) continue;
        LayoutWidget* blw = static_cast<LayoutWidget*>(_widgets[k]);
        if (!blw->visible()) continue;
        bi++;
        out.print(F("<button class=\"bn-item\" data-lay=\""));
        out.print(bi);
        out.print(F("\" onclick=\"RL.go("));
        out.print(bi);
        out.print(F(")\"><svg viewBox=\"0 0 24 24\"><path d=\""));
        out.print(FPSTR(blw->iconPath() ? blw->iconPath() : RICON_HOME));
        out.print(F("\"/></svg><span>"));
        out.print(rI18n(_widgets[k]->key()));
        out.print(F("</span></button>"));
      }
      out.print(F("</nav>"));
    }
  } else if (!hasTabs) {
    out.print(F("<main class=\"grid\">"));
    for (uint8_t i = 0; i < _count; i++)
      if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
    out.print(F("</main>"));
  } else {
    // Cards before the first tab are always visible.
    uint8_t i = 0;
    out.print(F("<main class=\"grid\">"));
    for (; i < _count && strcmp(_widgets[i]->typeId(), "tab") != 0; i++)
      if (!_widgets[i]->isSetting()) _widgets[i]->card(out);
    out.print(F("</main>"));
    // Tab bar.
    out.print(F("<div class=\"tabbar\">"));
    uint8_t ti = 0;
    for (uint8_t j = i; j < _count; j++)
      if (strcmp(_widgets[j]->typeId(), "tab") == 0) {
        out.print(F("<button class=\"tabbtn"));
        if (ti == 0) out.print(F(" on"));
        out.print(F("\" data-tab=\""));
        out.print(ti);
        out.print(F("\">"));
        out.print(rI18n(_widgets[j]->key()));
        out.print(F("</button>"));
        ti++;
      }
    out.print(F("</div>"));
    // One switchable panel per tab.
    ti = 0;
    uint8_t j = i;
    while (j < _count) {
      out.print(F("<main class=\"grid tabpanel"));
      if (ti == 0) out.print(F(" on"));
      out.print(F("\" data-panel=\""));
      out.print(ti);
      out.print(F("\">"));
      j++;  // skip the tab marker
      for (; j < _count && strcmp(_widgets[j]->typeId(), "tab") != 0; j++)
        if (!_widgets[j]->isSetting()) _widgets[j]->card(out);
      out.print(F("</main>"));
      ti++;
    }
  }
  out.print(FPSTR(RISAL_BODY_FOOT));
  out.print(rloc(L_SERVED, eff));
  out.print(FPSTR(RISAL_FOOT_END));

  // Client runtime + each widget type's JS (once) + init.
  out.print(FPSTR(RISAL_SCRIPT_OPEN));
  out.print(FPSTR(RISAL_RUNTIME_JS));
  out.print(F("R.L={on:'On',off:'Off'};"));
  if (strcmp(eff, "ru") == 0) out.print(F("R.L.on='Вкл';R.L.off='Выкл';"));
  else if (strcmp(eff, "uz") == 0) out.print(F("R.L.on='Yoniq';R.L.off='O\xCA\xBB" "chiq';"));
  else if (strcmp(eff, "ar") == 0) out.print(F("R.L.on='تشغيل';R.L.off='إيقاف';"));
  out.print(F("R.openNav=function(o){var s=document.querySelector('.scrim'),d=document.querySelector('.drawer');"
              "if(s)s.classList.toggle('open',o);if(d)d.classList.toggle('open',o);};"));
  out.print(F("window.RSB_TZ="));
  out.print(_tz);
  out.print(F(";"));
  if (_gsm) out.print(F("window.RSB_GSM=1;"));  // cellular bars, only if the board declares a modem
  if (_bt) out.print(F("window.RSB_BT=1;"));    // Bluetooth glyph, only when BT is active
  if (WiFi.isConnected()) {  // real link RSSI (dBm), shown next to the Wi-Fi icon; only in STA mode
    out.print(F("window.RSB_RSSI="));
    out.print(WiFi.RSSI());
    out.print(F(";"));
  }
  if (_battery) {  // real battery % (dash.battery) -> status bar shows it, no cosmetic drift
    out.print(F("window.RSB_BAT="));
    out.print(*_battery);
    out.print(F(";window.RSB_BAT_REAL=1;"));
  }
  out.print(FPSTR(RISAL_STATUSBAR_JS));
  // Settings modal: localized labels (window.RSL) + default accent (window.RSACC) + the JS.
  if (strcmp(eff, "ru") == 0)
    out.print(F("window.RSL={set:'Настройки',lang:'Язык',theme:'Тема',accent:'Акцент',"
                 "note:'Сохранено \\u00b7 на всех экранах',dark:'Тёмная',light:'Светлая',auto:'Авто',"
                 "signal:'Сигнал (dBm)',on:'Вкл',off:'Выкл',battery:'Батарея'};"));
  else if (strcmp(eff, "uz") == 0)
    out.print(F("window.RSL={set:'Sozlamalar',lang:'Til',theme:'Mavzu',accent:'Urg\xCA\xBBu',"
                 "note:'Saqlandi \\u00b7 barcha ekranlarda',dark:'Qorong\xCA\xBBi',light:'Yorug\xCA\xBB',auto:'Avto',"
                 "signal:'Signal (dBm)',on:'Yoniq',off:'O\xCA\xBB" "chiq',battery:'Batareya'};"));
  else if (strcmp(eff, "ar") == 0)
    out.print(F("window.RSL={set:'الإعدادات',lang:'اللغة',theme:'السمة',accent:'اللون',"
                 "note:'محفوظ \\u00b7 في كل الشاشات',dark:'داكن',light:'فاتح',auto:'تلقائي',"
                 "signal:'الإشارة (dBm)',on:'تشغيل',off:'إيقاف',battery:'البطارية'};"));
  out.print(F("window.RSACC="));
  out.print(_accent);
  out.print(F(";"));
  // Device settings: .gear() widgets are pulled out of the grid and listed in the Settings modal,
  // which reads their value from /api/state and writes via /api/set. (Toggles for now.)
  out.print(F("window.RSDEV=["));
  {
    bool first = true;
    for (uint8_t i = 0; i < _count; i++) {
      if (!_widgets[i]->isSetting()) continue;
      if (!first) out.print(',');
      if (_widgets[i]->writeGearMeta(out)) first = false;  // each control emits its own descriptor
    }
  }
  out.print(F("];"));
  out.print(FPSTR(RISAL_SETTINGS_JS));
  if (layCount > 0) out.print(FPSTR(RISAL_LAYOUTS_JS));
  sc = 0;
  for (uint8_t i = 0; i < _count; i++) {
    const char* j = _widgets[i]->js();
    if (!j) continue;
    bool dup = false;
    for (uint8_t k = 0; k < sc; k++)
      if (seen[k] == j) { dup = true; break; }
    if (!dup) { seen[sc++] = j; out.print(FPSTR(j)); }
  }
  out.print(FPSTR(RISAL_INIT_JS));
  out.print(FPSTR(RISAL_HTML_END));
}

