// Reference of EVERY RisalDash widget, split into swipeable pages by purpose.
// Display readouts, controls, text inputs, data cards and the rich visual widgets
// (robot face, Leaflet map, 3D cube, terminal, thermal heatmap) — all live at once.
// The map and the image load remote content, so those two cards want internet on the
// *client*; everything else is fully offline. Served over a plain access point:
// connect to "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>
#include <math.h>

RisalUI dash("Showcase");

// Display values
float cpu = 42, volts = 12.1f, temp = 24.3f, rssi = -52, hum = 55, pres = 1013;
int   ram = 60, pumpState = 0;            // pumpState: 0 ok · 1 warn · 2 bad
bool  linkUp = true;
// Control values
int   bright = 128, fanSpeed = 2, mode = 1, lightMode = 0;
bool  pump = false;
// Input values (controls bound to String state)
String devName = "greenhouse-01", wifiKey = "", boot = "08:30", svcDay = "2026-06-27", led = "#22d3ee";
String note = "All systems nominal — no action needed.";
String imgUrl = "https://picsum.photos/480/300";  // any URL the *client* can reach
// Visual values
int mood = 1;                              // robot face: 0..9
float gpsLat = 41.311f, gpsLon = 69.279f;  // map marker (fake circle below)
float pitch = 0, roll = 0, yaw = 0;        // 3D cube orientation
LogWidget* eventLog = nullptr;
TerminalWidget* term = nullptr;
HeatmapWidget* heat = nullptr;
static const int TW = 16, TH = 12;
float thermal[TW * TH];

void setup() {
  dash.timezone(180);

  // ── Page 1: display readouts ──
  dash.layout("Display", RICON_GAUGE);
  dash.metric("CPU", &cpu, "%").decimals(0).zone(70, 90);
  dash.gauge("Voltage", &volts, 0, 14, "V");                       // ring (default)
  dash.gauge("Humidity", &hum, 0, 100, "%").variant("semi");       // speedometer half
  dash.gauge("Pressure", &pres, 950, 1050, "hPa").variant("bar");  // linear bar
  dash.chart("Temperature", &temp, "C");
  dash.separator("Small stats");
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.progress("RAM", &ram, "%");
  dash.badge("Pump state", &pumpState).labels("running", "idle", "fault");
  dash.led("Link", &linkUp);
  dash.label("Device", &devName);

  // ── Page 2: controls ──
  dash.layout("Controls", RICON_POWER);
  dash.toggle("Pump", &pump, [](bool on) { pumpState = on ? 0 : 1; });
  dash.slider("Brightness", &bright, 0, 255, [](int v) { (void)v; });
  dash.number("Fan speed", &fanSpeed, 0, 5, 1, [](int v) { (void)v; });
  dash.select("Mode", "Eco,Comfort,Boost", &mode, [](int i) { (void)i; });
  dash.radio("Light", "Off,Warm,Cool", &lightMode, [](int i) { (void)i; });
  dash.button("Reboot", "Restart", []() { ESP.restart(); });

  // ── Page 3: text & value inputs ──
  dash.layout("Inputs", RICON_HOME);
  dash.text("Hostname", &devName, [](const String& v) { (void)v; });
  dash.textarea("Notes", &note, [](const String& v) { (void)v; });
  dash.password("Wi-Fi key", &wifiKey, [](const String& v) { (void)v; });
  dash.time("Boot time", &boot);
  dash.date("Service day", &svcDay);
  dash.color("LED color", &led);

  // ── Page 4: data cards ──
  dash.layout("Data", RICON_CLOCK);
  dash.table("Sensors")
      .row("Temperature", &temp, "C", 1)
      .row("Humidity", &hum, "%", 0)
      .row("Pressure", &pres, "hPa", 0);
  eventLog = &dash.log("Events", 5);
  dash.image("Camera", &imgUrl);  // any live JPEG/PNG URL (IP cam snapshot…)
  dash.ai("Assistant", &note);

  // ── Page 5: rich visual widgets ──
  dash.layout("Visual", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_L);
  dash.select("Emotion", "Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look", &mood);
  dash.map("Track", &gpsLat, &gpsLon).size(RSIZE_L);  // client needs internet (Leaflet tiles)
  dash.cube("Orientation", &pitch, &roll, &yaw).size(RSIZE_L);
  heat = &dash.heatmap("Thermal cam", TW, TH);        // MLX90640-style, fake frames below
  term = &dash.terminal("Shell", [](const String& cmd) {
    if (!term) return;
    if (cmd == "help") term->print("cmds: help, heap, uptime");
    else if (cmd == "heap") term->print(String(ESP.getFreeHeap()) + " bytes free");
    else if (cmd == "uptime") term->print(String(millis() / 1000) + " s");
    else term->print("unknown: " + cmd + " (try 'help')");
  });
  term->print("RisalDash console - type 'help'");

  dash.beginAP("RisalDash-Demo", "12345678");
}

uint32_t lastLog = 0, lastHeat = 0;

void loop() {
  // Simulate movement so every page looks alive.
  cpu = 30 + (millis() / 200 % 55);
  temp = 24.0f + 2.5f * sinf(millis() * 0.0005f);  // bounded wander, ~22..27 C
  gpsLat = 41.311f + 0.004f * sinf(millis() * 0.0003f);  // circle for the map marker
  gpsLon = 69.279f + 0.004f * cosf(millis() * 0.0003f);
  pitch = 30.0f * sinf(millis() * 0.0009f);              // wobble for the 3D cube
  roll = 25.0f * sinf(millis() * 0.0013f);
  yaw = fmodf(millis() * 0.03f, 360.0f);
  if (eventLog && millis() - lastLog > 4000) {
    lastLog = millis();
    eventLog->print("tick " + String(millis() / 1000) + "s  heap=" + ESP.getFreeHeap());
  }
  if (heat && millis() - lastHeat > 500) {               // fake thermal frame — moving hotspot
    lastHeat = millis();
    float ph = millis() * 0.001f;
    float hx = TW / 2.0f + TW / 3.0f * sinf(ph), hy = TH / 2.0f + TH / 3.0f * cosf(ph * 0.7f);
    for (int y = 0; y < TH; y++)
      for (int x = 0; x < TW; x++) {
        float dx = x - hx, dy = y - hy;
        thermal[y * TW + x] = 22.0f + 15.0f * expf(-(dx * dx + dy * dy) / 10.0f);
      }
    heat->frame(thermal);
  }
  dash.update();
}
