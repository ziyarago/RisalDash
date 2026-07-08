// UI construction kit — every structural element you build an interface from, in one sketch.
// Widgets themselves live in AllWidgets; THIS is the skeleton around them:
//   pages        dash.layout("Name", RICON_*)   — swipeable pages + the top nav strip
//   groups       dash.group("Title")            — section headers inside a page
//   separators   dash.separator("Title")        — a labelled divider line
//   sizes        .size(RSIZE_S / M / L)         — the iOS-style cell grid footprint
//   icons        .icon(RICON_*)                 — a glyph in the card header
//   gear         .gear()                        — the control moves into the Settings modal
//   chrome       title, theme, accent, language, effects, timezone, status-bar radios
// Served over a plain access point — connect to "RisalDash-Demo", open http://192.168.4.1/
#include <RisalUI.h>
#include <math.h>

RisalUI dash("UI Kit");

float small_ = 24, medium_ = 48, large_ = 72, iconed = 36;
int pct = 40, bat = 76;
bool inGrid = false, inGear = true;

void setup() {
  // ── Chrome: everything AROUND the widget grid ──
  dash.timezone(300);         // status-bar clock, minutes from UTC (+05:00 Tashkent)
  dash.theme(RisalUI::AUTO);  // DARK / LIGHT / AUTO (follows the phone)
  dash.accent(2);             // default accent: 0 Aqua · 1 Blue · 2 Violet · 3 Amber · 4 Rose
  dash.lang("en");            // default language; Settings has the EN/RU/UZ/AR switcher
  dash.effects(true);         // glass blur + gradient wash (false = flat, lighter GPU)
  dash.battery(&bat);         // REAL battery % in the status bar (unset = a cosmetic one)
  dash.gsm();                 // extra status-bar radios — declare only what the board has
  dash.bluetooth();

  // ── Page 1: the cell grid — sizes, groups, separators, icons ──
  dash.layout("Grid", RICON_GAUGE);
  dash.group("Sizes: the same widget, three footprints");
  dash.metric("Small", &small_, "%");                   // RSIZE_S default: 1 cell
  dash.metric("Medium", &medium_, "%").size(RSIZE_M);   // full width
  dash.metric("Large", &large_, "%").size(RSIZE_L);     // full width + double height
  dash.separator("A separator between sections");
  dash.stat("With an icon", &iconed, "C").icon(RICON_THERMOMETER);  // glyph replaces the dot
  dash.stat("Accent dot", &iconed, "C");                             // the default header

  // ── Page 2: where controls can live ──
  dash.layout("Device", RICON_HOME);
  dash.group("In the grid");
  dash.toggle("A dashboard control", &inGrid);
  dash.group("In the Settings gear");
  // .gear() pulls a control out of the grid into the Settings modal (top-right gear icon) —
  // it reads as a device setting, but still lives in /api/state and the WebSocket.
  dash.toggle("Gear toggle", &inGear).gear();
  dash.slider("Gear slider", &pct, 0, 100).gear();

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Wandering values so every size preset visibly ticks.
  small_ = 50 + 40 * sinf(millis() * 0.0006f);
  medium_ = 50 + 40 * sinf(millis() * 0.0004f + 1);
  large_ = 50 + 40 * sinf(millis() * 0.0005f + 2);
  iconed = 30 + 8 * sinf(millis() * 0.0007f);
  dash.update();
}
