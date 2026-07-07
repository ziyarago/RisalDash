#pragma once
#include <Arduino.h>

// Curated IoT icon set (MDI-style 24×24 path data). Each is its own PROGMEM symbol, so
// --gc-sections keeps only the ones a widget actually references (Zero-Waste). Pass any
// other path string to .icon() to use a different glyph.
static const char RICON_THERMOMETER[] PROGMEM = "M12 3a3 3 0 0 0-3 3v7.1a4 4 0 1 0 6 0V6a3 3 0 0 0-3-3zm0 16a2 2 0 0 1-1-3.7V6a1 1 0 0 1 2 0v9.3A2 2 0 0 1 12 19z";
static const char RICON_WATER[] PROGMEM = "M12 3c-3.2 4.2-6 7.3-6 10.2A6 6 0 0 0 18 13.2C18 10.3 15.2 7.2 12 3z";
static const char RICON_FLASH[] PROGMEM = "M13 2L4 14h6l-1 8 9-12h-6l1-8z";
static const char RICON_BULB[] PROGMEM = "M9 21h6v-1H9v1zm3-19a7 7 0 0 0-4 12.7V17h8v-2.3A7 7 0 0 0 12 2z";
static const char RICON_POWER[] PROGMEM = "M11 3h2v10h-2V3zm5.6 3.6 1.4 1.4a8 8 0 1 1-12 0L7.4 6.6a6 6 0 1 0 9.2 0z";
static const char RICON_GAUGE[] PROGMEM = "M12 16A2 2 0 0 1 10 14C10 13 14 8 14 8C14 8 14 13 14 14A2 2 0 0 1 12 16M12 3A9 9 0 0 0 3 12A9 9 0 0 0 12 21A9 9 0 0 0 21 12A9 9 0 0 0 12 3M12 5A7 7 0 0 1 19 12A7 7 0 0 1 12 19A7 7 0 0 1 5 12A7 7 0 0 1 12 5Z";
static const char RICON_HOME[] PROGMEM = "M12 3 3 11h3v9h5v-6h2v6h5v-9h3L12 3z";
static const char RICON_WIFI[] PROGMEM = "M12 18l3-3a4.2 4.2 0 0 0-6 0l3 3zM5 11a10 10 0 0 1 14 0l-2 2a7 7 0 0 0-10 0l-2-2z";
static const char RICON_CLOCK[] PROGMEM = "M12 20A8 8 0 0 1 4 12A8 8 0 0 1 12 4A8 8 0 0 1 20 12A8 8 0 0 1 12 20M12 2A10 10 0 0 0 2 12A10 10 0 0 0 12 22A10 10 0 0 0 22 12A10 10 0 0 0 12 2M12.5 7H11V13L16.25 16.15L17 14.92L12.5 12.25V7Z";
static const char RICON_SIGNAL[] PROGMEM = "M4 20h3v-7H4v7zm6.5 0h3V8h-3v12zm6.5 0h3V4h-3v16z";
static const char RICON_LEAF[] PROGMEM = "M17 8C8 10 6 16 5 21c0 0 8.5 1 12.5-6 1.6-2.8 1-7-.5-7z";
static const char RICON_MOTION[] PROGMEM = "M13.5 5.5A2 2 0 0 0 15.5 3.5A2 2 0 0 0 13.5 1.5A2 2 0 0 0 11.5 3.5A2 2 0 0 0 13.5 5.5M9.89 19.38L10.89 15L13 17V23H15V15.5L12.89 13.5L13.5 10.5C14.79 12 16.79 13 19 13V11C17.09 11 15.5 10 14.69 8.58L13.69 7C13.29 6.38 12.69 6 12 6C11.69 6 11.5 6.08 11.19 6.08L6 8.28V13H8V9.58L9.79 8.88L8.19 17L3.29 16L2.89 18L9.89 19.38Z";
