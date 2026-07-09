// Voice assistant turn for the Hosyond board: record the mic -> POST a WAV to the voice-proxy
// (which runs Whisper -> Claude -> OpenAI TTS) -> play the returned MP3 reply. The whole turn runs
// on the audio task (core 0); the UI (core 1) only sets a request flag. Transcript + reply come back
// in X-Transcript / X-Reply headers (URL-encoded UTF-8) — shown on serial / the web, not the LCD
// (its font is Latin-only). Set VOICE_ENDPOINT to wherever the proxy listens.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "audio.h"

#ifndef VOICE_ENDPOINT
#define VOICE_ENDPOINT "http://192.168.8.130:8787/voice"   // PC on the Wi-Fi LAN (change as needed)
#endif

namespace voice {

static const int REC_SECONDS = 4;
static const uint32_t SAMPLE_RATE = 16000;
static const uint32_t REC_SAMPLES = SAMPLE_RATE * REC_SECONDS;   // 64000 mono samples
static const size_t WAV_CAP = 44 + REC_SAMPLES * 2;             // 44-byte header + PCM
static const size_t MP3_CAP = 320 * 1024;                       // reply MP3 budget

enum State { IDLE = 0, LISTENING, THINKING, SPEAKING, DONE, ERROR };
static volatile int state = IDLE;
static String transcript, reply, errMsg;

static uint8_t *_wav = nullptr;   // [WAV header][PCM] — PSRAM
static uint8_t *_mp3 = nullptr;   // reply MP3 — PSRAM
static size_t _mp3Len = 0;

inline void _u32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
inline void _u16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }

// Minimal 16-bit PCM mono WAV header at the front of the buffer.
inline void _wavHeader(uint8_t *p, uint32_t pcmBytes, uint32_t rate) {
  memcpy(p, "RIFF", 4);        _u32(p + 4, 36 + pcmBytes);  memcpy(p + 8, "WAVE", 4);
  memcpy(p + 12, "fmt ", 4);   _u32(p + 16, 16);            _u16(p + 20, 1);   // PCM
  _u16(p + 22, 1);             _u32(p + 24, rate);          _u32(p + 28, rate * 2);
  _u16(p + 32, 2);             _u16(p + 34, 16);
  memcpy(p + 36, "data", 4);   _u32(p + 40, pcmBytes);
}

inline String _urlDecode(const String &s) {
  String o; o.reserve(s.length());
  for (uint16_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '%' && i + 2 < s.length()) { o += (char)strtol(s.substring(i + 1, i + 3).c_str(), nullptr, 16); i += 2; }
    else if (c == '+') o += ' ';
    else o += c;
  }
  return o;
}

// Allocate the PSRAM buffers once (call from setup, core 1). Needs BOARD_HAS_PSRAM.
inline bool begin() {
  _wav = (uint8_t *)ps_malloc(WAV_CAP);
  _mp3 = (uint8_t *)ps_malloc(MP3_CAP);
  return _wav && _mp3;
}
inline bool ready() { return _wav && _mp3; }

// Run one full voice turn. MUST be called on the audio task (core 0). Blocks ~record + network + TTS.
inline void turn() {
  if (!ready()) { state = ERROR; errMsg = "no psram"; return; }
  transcript = ""; reply = ""; errMsg = "";

  // 1) record the mic into _wav+44
  state = LISTENING;
  audio::recordBegin();
  { int16_t junk[256]; for (int i = 0; i < 8; i++) audio::recordRead(junk, 256); }  // drop codec settle
  size_t got = 0;
  uint32_t t0 = millis();
  while (got < REC_SAMPLES && millis() - t0 < (uint32_t)(REC_SECONDS * 1000 + 800)) {
    size_t n = audio::recordRead((int16_t *)(_wav + 44) + got, REC_SAMPLES - got);
    if (n) got += n; else delay(2);
  }
  uint32_t pcmBytes = got * 2;
  _wavHeader(_wav, pcmBytes, SAMPLE_RATE);
  audio::recordEnd();  // codec back to playback

  // 2) POST the WAV, collect transcript/reply headers + the MP3 body
  state = THINKING;
  HTTPClient http;
  http.setTimeout(30000);
  if (!http.begin(VOICE_ENDPOINT)) { state = ERROR; errMsg = "begin failed"; return; }
  http.addHeader("Content-Type", "audio/wav");
  const char *H[] = {"X-Transcript", "X-Reply", "X-Error"};
  http.collectHeaders(H, 3);
  int code = http.POST(_wav, 44 + pcmBytes);
  if (code != 200) {
    errMsg = http.header("X-Error").length() ? _urlDecode(http.header("X-Error")) : (String("HTTP ") + code);
    http.end(); state = ERROR;
    Serial.printf("[voice] POST failed: %s\n", errMsg.c_str());
    return;
  }
  transcript = _urlDecode(http.header("X-Transcript"));
  reply = _urlDecode(http.header("X-Reply"));
  Serial.printf("[voice] heard: %s\n[voice] reply: %s\n", transcript.c_str(), reply.c_str());

  int len = http.getSize();
  WiFiClient *st = http.getStreamPtr();
  _mp3Len = 0;
  uint32_t rt = millis();
  while ((len < 0 || _mp3Len < (size_t)len) && _mp3Len < MP3_CAP && millis() - rt < 20000) {
    size_t avail = st->available();
    if (avail) {
      int r = st->readBytes(_mp3 + _mp3Len, min(avail, MP3_CAP - _mp3Len));
      _mp3Len += r; rt = millis();
    } else if (!http.connected()) break;
    else delay(1);
  }
  http.end();
  Serial.printf("[voice] mp3 %u bytes\n", (unsigned)_mp3Len);

  // 3) speak the reply
  state = SPEAKING;
  audio::playMemoryMp3(_mp3, _mp3Len);
  state = DONE;
}

}  // namespace voice
