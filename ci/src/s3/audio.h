// Internet-radio audio for the Hosyond 2.8" board via arduino-audio-tools + arduino-audio-driver.
// The audio-driver AudioBoard configures the ES8311 codec through esp_codec_dev (the same driver
// ESPHome/ESP-ADF use), coordinating the codec + I2S + MCLK as one unit. audio-tools streams the
// URL, libhelix decodes MP3; the PCM passes through a software VolumeStream (runtime loudness, no
// I2C) and out to the I2SCodecStream. Runs on core 0 (audio::loop -> copier.copy), off the UI loop.
//
// Pins (verified vs the working ESPHome config celer/es3c28p.yaml for this exact board):
//   PA_EN(8002E amp)=1 · MCLK=4 · BCLK=5 · LRCK/WS=7 · DOUT(->codec)=8 · DIN(mic)=6 · ES8311@0x18
// ES8311 shares the touch I2C bus (SDA=16, SCL=15); Wire is already begun by touch::begin().
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>       // network for URLStream
#define USE_WIFI        // gate that enables URLStream (HTTP streaming) in audio-tools
#include "AudioTools.h"
#include "AudioTools/AudioLibs/I2SCodecStream.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Communication/HTTP/URLStream.h"  // HTTP radio stream (not pulled in by default)

namespace audio {

enum { PA_EN = 1, I2S_MCLK = 4, I2S_BCLK = 5, I2S_LRCK = 7, I2S_DOUT = 8, I2S_DIN = 6, ES_ADDR = 0x18 };

static DriverPins _pins;
static AudioBoard _board(AudioDriverES8311, _pins);
static I2SCodecStream _i2s(_board);
static VolumeStream _vol(_i2s);                          // software gain -> i2s (thread-safe, no I2C)
static EncodedAudioStream _dec(&_vol, new MP3DecoderHelix());
static URLStream _url("", "");
static StreamCopy _copier(_dec, _url);
static bool _ok = false, _playing = false;

// Call once from setup() (core 1) after touch::begin brought the Wire bus up. Configures the ES8311
// board + I2S and leaves the amp muted until play().
inline bool begin() {
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);  // quiet per-copy spam; warnings only
  // This board's 8002E amp is ACTIVE-LOW (LOW=on, HIGH=mute) — verified on hardware. Keep it muted
  // (HIGH) until play(). We do NOT register PA with the driver: setPAPower() hard-codes HIGH=enable,
  // which would drive the amp into permanent mute. We own GPIO1 by hand instead.
  pinMode(PA_EN, OUTPUT);
  digitalWrite(PA_EN, HIGH);  // amp off (active-LOW) until playback starts
  Wire.beginTransmission(ES_ADDR);
  if (Wire.endTransmission() != 0) { _ok = false; return false; }  // codec must answer first
  // Reuse the already-running Wire bus (touch/BMP280 are on it), plus the I2S pins.
  _pins.addI2C(PinFunction::CODEC, 15 /*SCL*/, 16 /*SDA*/, ES_ADDR, 400000, Wire);
  _pins.addI2S(PinFunction::CODEC, I2S_MCLK, I2S_BCLK, I2S_LRCK, I2S_DOUT, I2S_DIN);

  auto cfg = _i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  // Explicitly pin the I2S signals so MCLK/BCLK/LRCK/data actually reach the ES8311.
  cfg.pin_mck = I2S_MCLK;   // 4
  cfg.pin_bck = I2S_BCLK;   // 5
  cfg.pin_ws = I2S_LRCK;    // 7
  cfg.pin_data = I2S_DOUT;  // 8 (ESP -> codec, speaker)
  if (!_i2s.begin(cfg)) { _ok = false; return false; }
  _i2s.setVolume(0.9f);  // codec DAC at a fixed level — one-time I2C write, safe here on core 1
  _vol.begin(cfg);       // software volume stage (same PCM format); runtime loudness lives here
  _vol.setVolume(0.6f);
  _dec.begin();
  _ok = true;
  return true;
}

inline void loop() { if (_ok && _playing) _copier.copy(); }  // pump URL -> MP3 decode -> vol -> I2S
inline bool play(const char *url) {
  if (!_ok) return false;
  _url.end();
  bool r = _url.begin(url, "audio/mpeg");
  _playing = r;
  if (r) digitalWrite(PA_EN, LOW);  // un-mute the active-LOW 8002E amp once the stream is up
  return r;
}
inline void stop() { if (_ok) { digitalWrite(PA_EN, HIGH); _url.end(); _playing = false; } }  // mute amp
inline void volume(int pct) { if (_ok) _vol.setVolume(constrain(pct, 0, 100) / 100.0f); }  // software, any core
inline bool running() { return _playing; }
inline bool ok() { return _ok; }

// ── Voice: record from the mic, then play a reply MP3 from memory. All of this must run on the ONE
// audio task (core 0). The single I2S peripheral is reconfigured between capture (RX, 16 kHz mono —
// what STT wants) and playback (TX); the codec/pins stay the same, only the direction/rate change.

// Switch the codec to capture and start reading the mic. Stops any radio first.
inline bool recordBegin() {
  if (!_ok) return false;
  stop();                       // drop the radio stream + mute the amp
  _i2s.end();
  auto cfg = _i2s.defaultConfig(RX_MODE);
  cfg.sample_rate = 16000; cfg.channels = 1; cfg.bits_per_sample = 16;
  cfg.pin_mck = I2S_MCLK; cfg.pin_bck = I2S_BCLK; cfg.pin_ws = I2S_LRCK; cfg.pin_data = I2S_DIN;
  bool r = _i2s.begin(cfg);
  _board.setInputVolume(90);    // ES8311 ADC/PGA gain
  return r;
}
// Read up to maxSamples 16-bit mono samples; returns the count actually captured.
inline size_t recordRead(int16_t *buf, size_t maxSamples) {
  if (!_ok) return 0;
  return _i2s.readBytes((uint8_t *)buf, maxSamples * 2) / 2;
}
// (Re)configure the I2S/codec for playback at a specific rate + channel count. The MP3 decoder's
// auto-notify doesn't reliably re-clock the codec here, so we set the rate EXPLICITLY per source:
// radio streams are 44.1 kHz stereo, OpenAI TTS is 24 kHz mono — mismatch = wrong pitch/speed.
inline void _txConfig(uint32_t rate, int channels) {
  _i2s.end();
  auto cfg = _i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = rate; cfg.channels = channels; cfg.bits_per_sample = 16;
  cfg.pin_mck = I2S_MCLK; cfg.pin_bck = I2S_BCLK; cfg.pin_ws = I2S_LRCK; cfg.pin_data = I2S_DOUT;
  _i2s.begin(cfg);
  _i2s.setVolume(0.9f);
  _vol.begin(cfg);
}
// Return the codec to the radio playback baseline (44.1 kHz stereo) after capture.
inline void recordEnd() { if (_ok) _txConfig(44100, 2); }

// Play an in-memory MP3 (the TTS reply) to the speaker, blocking until it finishes. OpenAI tts-1 is
// 24 kHz mono, so clock the codec to that; restore the 44.1 kHz stereo baseline afterwards so radio
// still plays at the right speed.
inline void playMemoryMp3(const uint8_t *data, size_t len) {
  if (!_ok || !data || !len) return;
  _txConfig(24000, 1);          // match OpenAI TTS output rate/channels
  digitalWrite(PA_EN, LOW);     // un-mute the active-LOW amp for the reply
  MemoryStream mem(data, len);
  _dec.begin();
  StreamCopy c(_dec, mem);
  c.copyAll();
  digitalWrite(PA_EN, HIGH);    // mute again when done
  _txConfig(44100, 2);          // back to the radio baseline
}

}  // namespace audio
