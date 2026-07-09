#include <Arduino.h>
// Hosyond 2.8" ESP32-S3 touch module showcase — a phone-style app launcher on the LCD, in the
// RisalDash design language, alongside the served web dashboard. Tap a tile to open a mini-app;
// the ‹ chevron returns home. Apps: Monitor (sensor list), Radio (stations — audio is Stage B),
// Robot (animated companion), Settings (brightness · LED · Forget Wi-Fi). Real battery, real
// touch coordinates (FT6336), a BMP280 on the I2C connector auto-detected (fake env until then).
// Split by purpose: display.h (lcd:: draw + screens), touch.h (tap x/y), led.h, sensors.h.
//
// Board: "ESP32-S3 2.8 inch 240x320 capacitive touch" (Hosyond/lcdwiki), 8 MB OPI PSRAM.
// PlatformIO: board_build.arduino.memory_type = qio_opi + -D BOARD_HAS_PSRAM (see ci/platformio.ini).
// Requires: "GFX Library for Arduino" + "QRCode" (ricmoo).
#include "display.h"
#include "touch.h"
#include "led.h"
#include "sensors.h"
#include "audio.h"
#define VOICE_ENDPOINT "http://192.168.8.130:8787/voice"  // voice-proxy on the PC (change to your host)
#include "voice.h"
#include <RisalUI.h>
#include <WiFi.h>
#include <Preferences.h>

Preferences prefs;

RisalUI dash("Touch 2.8");

// Web-controlled state.
int backlight = 100;   // no heat limit on this panel — full brightness for vivid colour
int ledMode = 3;       // 0 Off · 1 Manual · 2 Per-widget · 3 Gradient
String ledColor = "#22d3ee";
int mood = 1;          // robot emotion (eyes() mood 0..9)
bool autoEmo = false;

// Real battery (GPIO9 reads ~half the cell voltage).
int batPct = 100;
float batV = 4.2f;

// System stats surfaced on the web "System" page.
float heapKb = 0, psramKb = 0, rssi = -60;
bool touchOk = false;

void bgTask(void *);  // core-0 background worker (defined after setup)

// ── LCD app shell ──
enum Screen { HOME = 0, MONITOR = 1, RADIO = 2, ROBOT = 3, SETTINGS = 4 };
int screen = HOME, lastScreen = -1;

// Internet radio — an EDITABLE station store. Seeded with Saudi/Makkah MP3 streams verified live via
// radio-browser.info; the user can rewrite the whole list from the web "Edit stations" textarea
// ("Name | URL | Meta" per line) and it persists in NVS. HTTP and HTTPS both play (audio-tools
// URLStream auto-uses WiFiClientSecure.setInsecure for https). Latin names only — the LCD font has
// no Arabic glyphs. LCD taps and the web select/textarea all drive one audio task via reqPlay/etc.
#define STN_CAP 14
String stnName[STN_CAP], stnUrl[STN_CAP], stnMeta[STN_CAP];  // parsed store (source of truth)
int stnN = 0;
char stationCsv[380];     // stable buffer for the web <select> — rebuilt in place so the pointer the
                          // SelectWidget captured stays valid and its options refresh live on edit.
String stationsText;      // the textarea mirror / NVS payload ("Name | URL | Meta" lines)

// Default list (used until the user saves their own). Makkah 88.4 = the Holy Quran Radio from Makkah.
const char DEFAULT_STATIONS[] PROGMEM =
  "Rotana FM | http://philae.shoutca.st:8250/stream | Saudi pop - 128k\n"
  "UFM 95.5 | https://stream.ufmradio.com:8003/ | Jeddah music\n"
  "AlifAlif FM | https://alifalifjobs.com/radio/8000/AlifAlifLive.mp3 | Saudi hits - 128k\n"
  "Makkah 88.4 | https://edge.mixlr.com/channel/rwumx | Holy Quran - Makkah\n"
  "Quran Mix | https://qurango.net/radio/mix | Holy Quran - 128k\n"
  "Quran Tarateel | https://qurango.net/radio/tarateel | Recitations - 128k\n"
  "Akhbar News | https://replaynewsar.ice.infomaniak.ch/replaynewsar-128.mp3 | Arabic news\n";

int station = 0, volPct = 60, radioScroll = 0;
bool playing = false;

// Rebuild the CSV the web <select> reads (in place — same buffer address). Commas in names are
// swapped for spaces so they don't split the CSV.
void rebuildStationCsv() {
  size_t n = 0;
  stationCsv[0] = 0;
  for (int i = 0; i < stnN && n < sizeof(stationCsv) - 1; i++) {
    if (i) n += snprintf(stationCsv + n, sizeof(stationCsv) - n, ",");
    String nm = stnName[i]; nm.replace(",", " ");
    n += snprintf(stationCsv + n, sizeof(stationCsv) - n, "%s", nm.c_str());
  }
}

// Parse "Name | URL | Meta" lines into the store (Meta optional). Falls back to one placeholder row.
void parseStations(const String &text) {
  stnN = 0;
  int start = 0;
  while (start < (int)text.length() && stnN < STN_CAP) {
    int nl = text.indexOf('\n', start);
    String line = (nl < 0) ? text.substring(start) : text.substring(start, nl);
    start = (nl < 0) ? text.length() : nl + 1;
    line.trim();
    if (line.length() == 0) continue;
    int p1 = line.indexOf('|');
    if (p1 < 0) continue;                       // require at least "Name | URL"
    int p2 = line.indexOf('|', p1 + 1);
    String nm = line.substring(0, p1); nm.trim();
    String url = (p2 < 0 ? line.substring(p1 + 1) : line.substring(p1 + 1, p2)); url.trim();
    String meta = (p2 < 0 ? String("") : line.substring(p2 + 1)); meta.trim();
    if (nm.length() == 0 || url.length() == 0) continue;
    stnName[stnN] = nm; stnUrl[stnN] = url; stnMeta[stnN] = meta; stnN++;
  }
  if (stnN == 0) { stnName[0] = "(empty)"; stnUrl[0] = ""; stnMeta[0] = ""; stnN = 1; }
  rebuildStationCsv();
}
// The Audio lib is NOT thread-safe: every call must come from ONE task. The UI (core 1) only
// posts requests here; bgTask (core 0, where audio::loop runs) executes them.
volatile int reqPlay = -1;  // station index to connect, or -1
volatile int reqVol = -1;   // 0..100 volume to set, or -1
volatile bool reqStop = false;  // UI (core 1) asks core 0 to stop the stream + mute the amp
volatile bool reqVoice = false; // UI asks core 0 to run one voice turn (record -> proxy -> speak)
String voiceStatus = "Готов";   // shown on the web (Russian) — LCD font is Latin-only

// Stream status from the Audio lib -> serial (helps tell "connecting" from "silent").
void audio_info(const char *info) { Serial.printf("[audio] %s\n", info); }

// Live-value caches so we repaint only what changed.
int lastTemp10 = -9999, lastPres = -1, lastHum = -1, lastBat = -1;
int lastMood = -1;
bool lastBlink = false;
uint32_t lastEyes = 0;

// One accent per screen: aqua on the hero (temp), the rest soft-white, battery as a status colour.
lcd::MonRow MONROWS[4] = {
  {"AIR TEMP", "BMP280 / fake env", lcd::C_TEAL},
  {"PRESSURE", "barometric", lcd::C_INK3},
  {"HUMIDITY", "AHT20 / fake", lcd::C_INK3},
  {"BATTERY", "GPIO9 ADC", lcd::C_GREEN},
};

void drawScreenStatic() {
  int r = (int)rssi;
  switch (screen) {
    case HOME:     lcd::launcher(r, batPct, "RisalDash v" RISALDASH_VERSION); break;
    case MONITOR:  lcd::monitorStatic(r, batPct, MONROWS); lastTemp10 = lastPres = lastHum = lastBat = -9999; break;
    case RADIO: {
      static const char *np[STN_CAP];
      for (int i = 0; i < stnN; i++) np[i] = stnName[i].c_str();
      lcd::radioStatic(r, batPct, stnName[station].c_str(), stnMeta[station].c_str(), np, stnN, station, volPct, playing, radioScroll);
    } break;
    case ROBOT:    lcd::robotStatic(r, batPct, mood); lastMood = -1; break;
    case SETTINGS: lcd::settingsStatic(r, batPct, backlight, ledMode); break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(120);

  lcd::begin();
  lcd::backlight(backlight);
  touchOk = touch::begin();  // brings up the shared I2C bus (touch + BMP280 + ES8311)
  sensorsBegin();
  bool audioOk = audio::begin();  // enable amp + init the ES8311 codec + wire I2S
  audio::volume(volPct);
  bool voiceOk = voice::begin();  // PSRAM buffers for record + reply MP3

  prefs.begin("rdisp", false);
  backlight = prefs.getInt("bl", backlight);
  ledMode = prefs.getInt("led", ledMode);
  ledColor = prefs.getString("col", ledColor);
  mood = prefs.getInt("mood", mood);
  // Radio station list: user's saved list from NVS, else the baked-in defaults.
  stationsText = prefs.getString("stns", "");
  if (stationsText.length() == 0) stationsText = FPSTR(DEFAULT_STATIONS);
  parseStations(stationsText);
  station = prefs.getInt("stn", station);   // last-picked radio station, persisted
  volPct = prefs.getInt("vol", volPct);     // last radio volume, persisted
  if (station < 0 || station >= stnN) station = 0;
  lcd::backlight(backlight);
  audio::volume(volPct);  // apply the saved volume (software gain — safe on this core)

  dash.battery(&batPct);  // real battery % in the web status bar

  // Radio control mirrored to the web dashboard — the same audio task the LCD drives. Web callbacks
  // run on core 0 (the audio core), so they post the same reqPlay/reqStop/reqVol handshake volatiles.
  dash.layout("Radio", RICON_SIGNAL);
  dash.select("Station", stationCsv, &station, [](int i) { station = i; prefs.putInt("stn", station); reqPlay = station; playing = true; });
  dash.toggle("Play", &playing, [](bool on) { if (on) { reqPlay = station; playing = true; } else { reqStop = true; playing = false; } });
  dash.slider("Volume", &volPct, 0, 100, [](int v) { volPct = v; reqVol = v; prefs.putInt("vol", volPct); });
  // Edit the list with a proper editor: drag to reorder, edit name/URL inline, add/remove. Saved to
  // NVS; the dropdown + LCD refresh live. MP3 streams only (the decoder is MP3). Latin names (no Arabic).
  dash.radioBrowser("Edit stations", &stationsText, [](const String &v) {
    parseStations(v);
    prefs.putString("stns", v);
    if (station >= stnN) station = 0;
    radioScroll = 0;
  });

  // Voice assistant — tap Talk, speak ~4 s near the board; it records, asks Claude via the proxy,
  // and speaks the reply. Transcript/reply show here (Russian renders on the web, not the LCD).
  dash.layout("Assistant", RICON_MOTION);
  dash.button("Voice", "🎤 Говорить (4 сек)", []() { if (!reqVoice) reqVoice = true; });
  dash.label("Статус", &voiceStatus);
  dash.label("Услышал", &voice::transcript);
  dash.label("Ответ", &voice::reply);

  dash.layout("Sensors", RICON_LEAF);
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.stat("Pressure", &pres, "hPa").decimals(1);
  dash.chart("Temp trend", &temp, "C");

  dash.layout("Robot", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_L);
  dash.select("Emotion", "Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look", &mood, [](int i) { (void)i; prefs.putInt("mood", mood); });
  dash.toggle("Auto emotion", &autoEmo);

  dash.layout("System", RICON_GAUGE);
  dash.stat("Battery", &batV, "V").decimals(2);
  dash.stat("Free heap", &heapKb, "KB").decimals(0);
  dash.stat("PSRAM free", &psramKb, "KB").decimals(0);
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.led("Touch OK", &touchOk);

  dash.slider("Backlight", &backlight, 5, 100, [](int i) { (void)i; prefs.putInt("bl", backlight); lcd::backlight(backlight); }).gear();
  dash.select("LED mode", "Off,Manual,Per-widget,Gradient", &ledMode, [](int i) { (void)i; prefs.putInt("led", ledMode); led::apply(ledMode, ledColor, screen); }).gear();
  dash.color("LED colour", &ledColor, [](const String &v) { (void)v; prefs.putString("col", ledColor); led::apply(ledMode, ledColor, screen); }).gear();

  dash.begin();
  Serial.printf("[net] IP=%s touch=%d audio=%d bmp280=%s\n", WiFi.localIP().toString().c_str(),
                (int)touchOk, (int)audioOk, bmpReal ? "REAL" : "absent (fake env)");
  led::apply(ledMode, ledColor, screen);

  // Dual-core split: the web server, sensor sampling and battery ADC live on core 0 so loop()
  // on core 1 stays a tight UI loop (touch + LCD). Without this, a full-screen SPI repaint or a
  // busy web request stalls loop() and the touch feels laggy. (Audio will join this core later.)
  xTaskCreatePinnedToCore(bgTask, "bg", 24576, nullptr, 2, nullptr, 0);  // big stack: MP3 decode runs here
}

// Core 0: the web server + battery/stats. NO I2C here — touch (FT6336) and the BMP280 share one
// I2C bus, so all I2C stays on core 1 to avoid a cross-core bus race. Battery is an ADC read.
void bgTask(void *) {
  uint32_t lastSysT = 0, lastWebT = 0;
  for (;;) {
    // Audio commands posted by the UI (core 1) are executed here on core 0, where audio::loop runs.
    if (reqStop) { reqStop = false; audio::stop(); }
    if (reqPlay >= 0) { if (reqPlay < stnN) audio::play(stnUrl[reqPlay].c_str()); reqPlay = -1; }
    if (reqVol >= 0) { audio::volume(reqVol); reqVol = -1; }
    if (reqVoice) {                        // one blocking voice turn (~record + network + speak)
      reqVoice = false; playing = false;
      voiceStatus = "Слушаю 4 сек…"; dash.update();   // push the status before we block
      voice::turn();
      voiceStatus = (voice::state == voice::ERROR) ? ("Ошибка: " + voice::errMsg) : "Готово ✓";
    }
    audio::loop();  // feed the MP3 stream -> I2S -> ES8311 (must run often, so it leads here)
    if (millis() - lastWebT > 20) { lastWebT = millis(); dash.update(); }  // web, lighter cadence
    if (millis() - lastSysT > 1000) {
      lastSysT = millis();
      batV = analogReadMilliVolts(9) * 2 / 1000.0f;  // ADC, not I2C — safe on this core
      int pc = (int)((batV - 3.30f) / (4.20f - 3.30f) * 100.0f);
      batPct = pc < 0 ? 0 : pc > 100 ? 100 : pc;
      heapKb = ESP.getFreeHeap() / 1024.0f;
      psramKb = ESP.getFreePsram() / 1024.0f;
      rssi = WiFi.RSSI();
    }
    vTaskDelay(1);
  }
}

// Handle a tap at (x,y) on the current screen.
void onTap(int x, int y) {
  if (screen != HOME && lcd::backHit(x, y)) {
    if (screen == RADIO) {
      prefs.putInt("vol", volPct);                          // save volume on the way out (not per-drag)
      if (playing) { reqStop = true; playing = false; }      // leaving Radio stops the stream
    }
    screen = HOME; return;
  }
  switch (screen) {
    case HOME: {
      int t = lcd::launcherHit(x, y);
      if (t >= 0) screen = t + 1;  // tiles 0..3 -> MONITOR..SETTINGS
    } break;
    case RADIO: {
      int sc = lcd::radioScrollHit(x, y);
      if (stnN > lcd::STN_MAX && sc >= 0) {                    // ▲/▼ pager (only when the list overflows)
        int maxOff = stnN - lcd::STN_MAX;
        radioScroll += (sc == 0 ? -1 : 1);
        if (radioScroll < 0) radioScroll = 0;
        if (radioScroll > maxOff) radioScroll = maxOff;
        drawScreenStatic();
        break;
      }
      int slot = lcd::radioStationHit(x, y);
      int s = (slot >= 0) ? radioScroll + slot : -1;           // map the visible row to a global index
      if (s >= 0 && s < stnN) {
        if (playing && s == station) { reqStop = true; playing = false; }   // tap the live station = stop
        else { station = s; reqPlay = s; playing = true; prefs.putInt("stn", station); }  // else connect + save
        drawScreenStatic();
      } else if (lcd::radioVolHit(x, y)) { volPct = lcd::radioVolPct(x); reqVol = volPct; drawScreenStatic(); }
    } break;
    case ROBOT: {
      int c = lcd::robotChipHit(x, y);
      if (c >= 0) { mood = lcd::ROBO_MOOD[c]; prefs.putInt("mood", mood); drawScreenStatic(); }
      else if (y >= 40 && y < 208 && !reqVoice && voice::state != voice::LISTENING &&
               voice::state != voice::THINKING && voice::state != voice::SPEAKING) {
        reqVoice = true;   // tap the robot's face = start a voice turn
      }
    } break;
    case SETTINGS: {
      int l = lcd::setLedHit(x, y);
      if (lcd::setForgetHit(x, y)) { dash.forgetWiFi(); lcd::centerText("Rebooting...", 150, 2, lcd::C_INK); }
      else if (l >= 0) { ledMode = l; prefs.putInt("led", ledMode); led::apply(ledMode, ledColor, screen); drawScreenStatic(); }
    } break;
    default: break;
  }
}

// Live-drag on a slider (fires while held, not just on tap-down).
void onHold(int x, int y) {
  if (screen == SETTINGS && lcd::setBrightHit(x, y)) {
    int p = lcd::setBrightPct(x);
    if (p != backlight) { backlight = p; lcd::backlight(backlight); prefs.putInt("bl", backlight); drawScreenStatic(); }
  } else if (screen == RADIO && lcd::radioVolHit(x, y)) {
    int p = lcd::radioVolPct(x);
    if (p != volPct) { volPct = p; reqVol = volPct; drawScreenStatic(); }
  }
}

uint32_t lastSys = 0, lastMon = 0, lastEmo = 0, lastLed = 0, lastHb = 0;

void loop() {
  // Core 1: UI + all I2C (touch + BMP280 + ES8311 config). The web server + audio stream run on
  // core 0. sampleSensors() is quick (4 Hz, throttled) and keeps I2C single-core.
  sampleSensors();

  // ── input ── release-based: after a tap opens an app, ignore the still-held finger until it
  // lifts (no fixed dead time that eats quick taps). Same-screen taps (sliders, chips) drag freely.
  static bool navLock = false;
  touch::Point p = touch::read();
  if (!p.held) navLock = false;                 // finger up -> ready for the next tap immediately
  else if (!navLock) {
    if (p.tap) { int before = screen; onTap(p.x, p.y); if (screen != before) navLock = true; }
    else onHold(p.x, p.y);                       // drag a slider
  }

  if (autoEmo && millis() - lastEmo > 1500) { lastEmo = millis(); mood = (mood + 1) % 10; }

  // ── render ──
  if (screen != lastScreen) { drawScreenStatic(); lastScreen = screen; }

  if (screen == HOME) {
    if (millis() - lastSys > 1000) { lastSys = millis(); lcd::statusBar((int)rssi, batPct); }  // live bar
  } else if (screen == MONITOR && millis() - lastMon > 700) {
    lastMon = millis();
    char b[10];
    int t10 = (int)(temp * 10);
    if (t10 != lastTemp10) { snprintf(b, sizeof(b), "%.1f", temp); lcd::monitorValue(0, b, "C", lcd::C_TEAL); lastTemp10 = t10; }   // hero: accent
    if ((int)pres != lastPres) { snprintf(b, sizeof(b), "%.0f", pres); lcd::monitorValue(1, b, "hPa", lcd::C_INK); lastPres = (int)pres; }
    if ((int)hum != lastHum) { snprintf(b, sizeof(b), "%d", (int)hum); lcd::monitorValue(2, b, "%", lcd::C_INK); lastHum = (int)hum; }
    if (batPct != lastBat) { snprintf(b, sizeof(b), "%d", batPct); lcd::monitorValue(3, b, "%", batPct <= 20 ? lcd::C_RED : lcd::C_GREEN); lastBat = batPct; }
  } else if (screen == ROBOT) {
    // The robot's eyes mirror the conversation: wide & attentive while listening, glancing around
    // while thinking, cheerful while speaking, dizzy on error — otherwise the user's chosen mood.
    int effMood = mood;
    switch (voice::state) {
      case voice::LISTENING: effMood = 4; break;          // surprised = wide, attentive
      case voice::THINKING:  effMood = 9; break;          // eyes dart side to side
      case voice::SPEAKING:  effMood = voice::mood; break; // Claude's mood for THIS reply
      case voice::ERROR:     effMood = 8; break;          // dizzy
      default: break;
    }
    bool blink = (millis() % 3800) < 130;
    bool anim = (effMood == 9 || effMood == 8);    // looking / dizzy animate continuously
    if (effMood != lastMood || blink != lastBlink || (anim && millis() - lastEyes > 120)) {
      lcd::eyes(effMood, blink);
      lastMood = effMood; lastBlink = blink; lastEyes = millis();
    }
    // Status line above the eyes, colour-coded to the phase.
    static int lastVs = -1;
    if (voice::state != lastVs) {
      lastVs = voice::state;
      switch (voice::state) {
        case voice::LISTENING: lcd::robotStatus("Listening...", lcd::C_TEAL); break;
        case voice::THINKING:  lcd::robotStatus("Thinking...", lcd::C_AMBER); break;
        case voice::SPEAKING:  lcd::robotStatus("Speaking...", lcd::C_GREEN); break;
        case voice::ERROR:     lcd::robotStatus("Error - see web", lcd::C_RED); break;
        default:               lcd::robotStatus("Tap face to talk", lcd::C_INK3); break;
      }
    }
  } else if (screen == RADIO) {
    // Reflect web-driven changes (station select / play toggle / edited list) on the LCD without a
    // tap, and scroll the list so the current station stays visible.
    static int lastStn = -1, lastN = -1; static bool lastPlay = false;
    if (station != lastStn || playing != lastPlay || stnN != lastN) {
      if (station < radioScroll) radioScroll = station;
      else if (station >= radioScroll + lcd::STN_MAX) radioScroll = station - lcd::STN_MAX + 1;
      lastStn = station; lastPlay = playing; lastN = stnN; drawScreenStatic();
    }
  }

  if (ledMode == led::GRADIENT && millis() - lastLed > 60) { lastLed = millis(); led::apply(ledMode, ledColor, screen); }
  if (millis() - lastHb > 3000) {
    lastHb = millis();
    Serial.printf("[hb] screen=%d bat=%d%% heap=%u psram=%u\n", screen, batPct, ESP.getFreeHeap(), ESP.getFreePsram());
  }
  delay(2);
}
