// Widget sizes. Every card sits on an iOS-style cell grid and has a sensible default
// footprint; `.size()` overrides it:
//   RSIZE_S — 1 cell, compact square (2 per row on a phone, 4 on a tablet)
//   RSIZE_M — full width, regular height
//   RSIZE_L — full width, double height (room for a gauge ring, chart, table…)
// This sketch shows the same reading rendered at all three sizes, side by side, plus a
// sensor preset resized as a whole. Served over a plain access point — connect to
// "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>
#include <math.h>

RisalUI dash("Sizes");

float temp = 24.3f, hum = 55, volts = 12.1f;

void setup() {
  dash.timezone(180);

  // ── Defaults: each type already picks a fitting footprint ──
  // metric/stat/badge/led/toggle… default to S; slider/select/text… to M;
  // gauge/chart/table/log… to L. You only reach for .size() to override.
  dash.group("Type defaults");
  dash.metric("Humidity", &hum, "%");            // S — compact single value
  dash.stat("Voltage", &volts, "V");             // S
  dash.gauge("Air temp", &temp, 0, 50, "C");     // L — needs room for the ring

  // ── The same metric at all three sizes ──
  dash.group("Same widget, three sizes");
  dash.metric("Temp S", &temp, "C").decimals(1);                 // default S: 1 cell
  dash.metric("Temp M", &temp, "C").decimals(1).size(RSIZE_M);   // full width
  dash.metric("Temp L", &temp, "C").decimals(1).size(RSIZE_L);   // full width + tall

  // ── A gauge shrunk to a cell — fine for a dense wall of dials ──
  dash.group("Compact gauges");
  dash.gauge("Temp", &temp, 0, 50, "C").size(RSIZE_S);
  dash.gauge("Volts", &volts, 0, 14, "V").size(RSIZE_S);

  // ── Sensor presets resize as a group: one .size() covers every readout ──
  dash.group("Preset resized");
  dash.sensor("dht22", &temp, &hum).size(RSIZE_L);

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Simulate movement so the dashboard looks alive.
  temp = 24.0f + 2.0f * sinf(millis() * 0.0005f);
  hum = 50 + (millis() / 400 % 12);
  dash.update();  // push changed values to the browser over WebSocket
}
