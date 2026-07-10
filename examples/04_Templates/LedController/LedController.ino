// LED strip controller — a WLED-style dashboard for a WS2812/SK6812 strip, NO hardware needed. A color
// picker, brightness, and effect selector drive the UI; the sketch estimates the strip's current draw so
// you see the load in real time. Wire FastLED/NeoPixel where marked and the same variables drive real
// pixels. Served over a plain access point — connect to "RisalDash-LED" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("LED Controller");

String color = "#22D3EE";
int brightness = 160, effect = 0, ledCount = 60;
bool  power = true;
float drawMa = 0;  // estimated current draw

void applyStrip() {
  // FastLED: fill_solid(leds, ledCount, CRGB(r, g, b));
  //          FastLED.setBrightness(power ? brightness : 0); FastLED.show();
}

void setup() {
  dash.timezone(180);
  dash.accent(3);  // purple — matches the LED theme; changeable in Settings

  dash.layout("Strip", RICON_BULB);
  dash.toggle("Power", &power, [](bool on) { (void)on; applyStrip(); });
  dash.color("Color", &color, [](const String& c) { (void)c; applyStrip(); });
  dash.slider("Brightness", &brightness, 0, 255, [](int v) { (void)v; applyStrip(); });
  dash.select("Effect", "Solid,Rainbow,Breathe,Chase,Sparkle", &effect, [](int v) { (void)v; });
  dash.number("LED count", &ledCount, 1, 300, 1, [](int v) { (void)v; });

  dash.layout("Power", RICON_FLASH);
  dash.gauge("Current draw", &drawMa, 0, 6000, "mA").variant("bar");

  dash.beginAP("RisalDash-LED", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 200) {
    last = millis();
    // Rough model: ~60 mA per WS2812 at full white, scaled by brightness and a colour-luminance factor.
    long r = strtol(color.substring(1, 3).c_str(), nullptr, 16);
    long g = strtol(color.substring(3, 5).c_str(), nullptr, 16);
    long b = strtol(color.substring(5, 7).c_str(), nullptr, 16);
    float lum = (r + g + b) / 765.0f;  // 0..1
    drawMa = power ? ledCount * 60.0f * (brightness / 255.0f) * (0.15f + 0.85f * lum) : 1.0f;
  }
}
