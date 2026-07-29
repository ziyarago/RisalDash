// ESP32-CAM time-lapse & wildlife camera trap — one sketch, two trigger modes:
//   MODE_TIMELAPSE  — capture a JPEG every CAPTURE_EVERY_S seconds (construction sites, plants, sky)
//   MODE_MOTION     — capture when the PIR on PIR_PIN fires (wildlife / security camera trap)
// Photos go to the SD card (SD_MMC, the slot on the ESP32-CAM board) named /0001.jpg, /0002.jpg…;
// the RisalDash dashboard shows the shot counter, SD usage, last-capture time and a manual trigger,
// so you can watch the trap work from a phone on the same network. Time comes from NTP when Wi-Fi is
// up (a DS3231 RTC is optional if you need timestamps fully offline).
//
// Board: AI-Thinker ESP32-CAM (OV2640). Select "AI Thinker ESP32-CAM" in the IDE. PIR on GPIO13.
// ⚠ ESP32-CAM only — this sketch needs esp_camera + SD_MMC and does not build on other boards.
#include <RisalUI.h>
#include "esp_camera.h"
#include "SD_MMC.h"

#define MODE_TIMELAPSE 1        // 1 = interval capture · 0 = PIR-triggered camera trap
#define CAPTURE_EVERY_S 30      // time-lapse interval
#define PIR_PIN 13              // camera-trap motion input (HC-SR501 OUT)

// AI-Thinker ESP32-CAM pin map (the common green board).
#define PWDN 32
#define XCLK 0
#define SIOD 26
#define SIOC 27
#define Y9 35
#define Y8 34
#define Y7 39
#define Y6 36
#define Y5 21
#define Y4 19
#define Y3 18
#define Y2 5
#define VSYNC 25
#define HREF 23
#define PCLK 22

RisalUI dash("Cam Trap");

float shots = 0, sdUsedPct = 0;
bool  motion = false, sdOk = false;
LogWidget* evlog = nullptr;
uint32_t lastShot = 0, shotNo = 0;

bool camInit() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = Y2; c.pin_d1 = Y3; c.pin_d2 = Y4; c.pin_d3 = Y5;
  c.pin_d4 = Y6; c.pin_d5 = Y7; c.pin_d6 = Y8; c.pin_d7 = Y9;
  c.pin_xclk = XCLK; c.pin_pclk = PCLK; c.pin_vsync = VSYNC; c.pin_href = HREF;
  c.pin_sccb_sda = SIOD; c.pin_sccb_scl = SIOC; c.pin_pwdn = PWDN; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;         // compressed — small and fast to save
  c.frame_size = FRAMESIZE_SVGA;           // 800x600, the IoT sweet spot
  c.jpeg_quality = 10;                     // 0..63, lower = better
  c.fb_count = 1;
  return esp_camera_init(&c) == ESP_OK;
}

void capture(const char* why) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { if (evlog) evlog->print("Capture FAILED"); return; }
  char name[16];
  snprintf(name, sizeof(name), "/%04u.jpg", (unsigned)++shotNo);
  File f = SD_MMC.open(name, FILE_WRITE);
  if (f) { f.write(fb->buf, fb->len); f.close(); shots = shotNo; }
  esp_camera_fb_return(fb);
  if (evlog) evlog->print(String(why) + " -> " + name);
  sdUsedPct = SD_MMC.totalBytes() ? 100.0f * SD_MMC.usedBytes() / SD_MMC.totalBytes() : 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  dash.layout("Trap", RICON_MOTION);
  dash.stat("Photos", &shots, "");
  dash.gauge("SD used", &sdUsedPct, 0, 100, "%").variant("bar");
  dash.led("SD ready", &sdOk);
  dash.led("Motion", &motion);
  dash.button("Shoot", "Capture now", []() { capture("Manual"); });
  evlog = &dash.log("Captures", 6);

  dash.beginAP("RisalDash-CamTrap", "12345678");

  sdOk = SD_MMC.begin("/sdcard", true);    // 1-bit mode frees the flash-LED pin
  if (!camInit() && evlog) evlog->print("Camera init FAILED");
}

void loop() {
  dash.update();
#if MODE_TIMELAPSE
  if (millis() - lastShot > CAPTURE_EVERY_S * 1000UL) { lastShot = millis(); capture("Timer"); }
#else
  bool pir = digitalRead(PIR_PIN);
  if (pir && !motion && millis() - lastShot > 5000) {  // 5 s cool-down between triggers
    lastShot = millis();
    capture("Motion");
  }
  motion = pir;
#endif
}
