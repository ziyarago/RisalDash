#pragma once
#include <Arduino.h>

// Declarations only — the PROGMEM payloads live in RisalAssets.cpp, so several
// translation units can share one flash copy of each asset.

// Core UI assets in PROGMEM — the Risal brand shell (tokens + base + orbs + appbar +
// glass card grid). Ported from the dev/ mock. Per-widget CSS/JS get appended to this
// core in P2 (Zero-Waste aggregation); the WebSocket runtime arrives in P3.

// The <!DOCTYPE><html lang dir> open is printed by RisalUI (language/RTL aware).
extern const char RISAL_HEAD[] PROGMEM;
extern const char RISAL_HEAD_END[] PROGMEM;

extern const char RISAL_CSS[] PROGMEM;

// In-page OS-style status bar (time + wifi/gsm/bt + fake battery). Self-injects CSS+HTML and ticks.
// Config: window.RSB_TZ (minutes), window.RSB_GSM, window.RSB_BT, window.RSB_BAT.
extern const char RISAL_STATUSBAR_JS[] PROGMEM;

// Global settings modal (appbar gear): language / theme / accent. Theme+accent persist to
// localStorage and apply live; language reloads (?lang=) so the server re-renders RTL+chrome.
// Localized labels via window.RSL; default accent index via window.RSACC.
extern const char RISAL_SETTINGS_JS[] PROGMEM;

// Multi-page layout switcher: swipe up from the bottom edge (or tap the handle) to open the
// sheet; a tile click shows that page client-side. Emitted only when layout()s are declared.
extern const char RISAL_LAYOUTS_JS[] PROGMEM;

// </style></head><body>; the theme-resolve script is printed by RisalUI between
// OPEN and CHROME (first body child → applies the saved/auto theme before paint).
extern const char RISAL_BODY_OPEN[] PROGMEM;
extern const char RISAL_BODY_CHROME[] PROGMEM;

// title is printed between OPEN and MID. The language switcher + theme toggle
// (RISAL_APPBAR_END) are printed by RisalUI after MID, so the active language can be marked.
extern const char RISAL_BODY_MID[] PROGMEM;

extern const char RISAL_APPBAR_END[] PROGMEM;

// Global SVG gradient defs (shared by gauge arc + chart line/fill). Printed once, hidden.
extern const char RISAL_DEFS[] PROGMEM;

extern const char RISAL_EMPTY[] PROGMEM;

// Footer: "served by ESP" tail is localized by RisalUI between these two.
extern const char RISAL_BODY_FOOT[] PROGMEM;
extern const char RISAL_FOOT_END[] PROGMEM;

extern const char RISAL_SCRIPT_OPEN[] PROGMEM;

// Client runtime: WebSocket connect + apply diffs to widgets + send commands.
extern const char RISAL_RUNTIME_JS[] PROGMEM;

extern const char RISAL_INIT_JS[] PROGMEM;

extern const char RISAL_HTML_END[] PROGMEM;

// ── First-boot Wi-Fi provisioning portal (OKLCH glass + signal levels + custom tz select) ──
extern const char RISAL_PORTAL_CSS[] PROGMEM;

// Portal client JS: network row selection + custom timezone dropdown (full zone list).
extern const char RISAL_PORTAL_JS[] PROGMEM;
