#pragma once
#include "base.h"

// Structure, not cards: group headers, layout() pages, separators, tabs.

static const char RW_GROUP_CSS[] PROGMEM =
  ".group{grid-column:1/-1;margin:8px 2px 0;font:700 11px/1 var(--font);letter-spacing:.16em;text-transform:uppercase;color:var(--ink3)}";

// ── Layout: group / section header (spans the grid; anchors the nav drawer) ──
class GroupWidget : public Widget {
 public:
  explicit GroupWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "group"; }
  const char* css() const override { return RW_GROUP_CSS; }
  void card(Print& out) override {
    out.print(F("<div class=\"group\" id=\"g-"));
    rwSlug(out, _title);  // slug/anchor stays the untranslated key (stable id)
    out.print(F("\">"));
    out.print(rI18n(_title));
    out.print(F("</div>"));
  }
};

// ── Layout: a full switchable page. With layouts declared, the dashboard shows one page at
// a time; a swipe-up sheet lists each as an icon tile. The marker itself renders nothing —
// RisalUI partitions the widgets that follow it into that page. Set the tile glyph via the
// second arg (a RICON_* path); falls back to a generic icon. ──
class LayoutWidget : public Widget {
 public:
  explicit LayoutWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "layout"; }
  void card(Print& out) override { (void)out; }  // a boundary marker, not a card
  const char* iconPath() const { return _icon; }
  // Conditional page: bound to a bool, the page and its switcher tile render only while it is true.
  // Flip the bool (its widget bumps the structure revision) so open clients reload and it appears/hides.
  LayoutWidget& visibleWhen(bool* b) { _vis = b; return *this; }
  bool visible() const { return !_vis || *_vis; }
 private:
  bool* _vis = nullptr;
};

static const char RW_SEP_CSS[] PROGMEM =
  ".sep{grid-column:1/-1;display:flex;align-items:center;gap:12px;margin:10px 2px 2px;color:var(--ink3);font:700 11px var(--font);letter-spacing:.14em;text-transform:uppercase}"
  ".sep::after{content:'';flex:1;height:1px;background:var(--line)}";
static const char RW_TAB_CSS[] PROGMEM =
  ".tabbar{display:flex;gap:8px;margin:14px 0 4px;flex-wrap:wrap}"
  ".tabbtn{padding:9px 16px;border-radius:999px;border:1px solid var(--line2);background:transparent;color:var(--ink2);font:600 13px var(--font);cursor:pointer}"
  ".tabbtn.on{background:var(--grad);color:var(--acc-ink);border-color:transparent}"
  ".tabpanel{display:none}.tabpanel.on{display:grid}";

static const char RW_TAB_JS[] PROGMEM =
  "R.W.tab={};document.addEventListener('DOMContentLoaded',function(){var b=document.querySelectorAll('.tabbtn');"
  "for(var i=0;i<b.length;i++)b[i].addEventListener('click',function(){var t=this.getAttribute('data-tab');"
  "var a=document.querySelectorAll('.tabbtn');for(var j=0;j<a.length;j++)a[j].classList.toggle('on',a[j].getAttribute('data-tab')===t);"
  "var p=document.querySelectorAll('.tabpanel');for(var j=0;j<p.length;j++)p[j].classList.toggle('on',p[j].getAttribute('data-panel')===t);});});";

// ── Layout: labelled divider (spans the grid) ──
class SeparatorWidget : public Widget {
 public:
  explicit SeparatorWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "separator"; }
  const char* css() const override { return RW_SEP_CSS; }
  void card(Print& out) override { out.print(F("<div class=\"sep\">")); out.print(_title); out.print(F("</div>")); }
};

// ── Layout: tab boundary (widgets after it, until the next tab, form a panel). Rendered by RisalUI. ──
class TabWidget : public Widget {
 public:
  explicit TabWidget(const char* title) : Widget(title, title) {}
  const char* typeId() const override { return "tab"; }
  const char* css() const override { return RW_TAB_CSS; }
  const char* js() const override { return RW_TAB_JS; }
  void card(Print&) override {}  // handled by RisalUI::_handleRoot
};
