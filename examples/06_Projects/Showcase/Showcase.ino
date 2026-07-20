// RisalDash Showcase — EVERY widget, layout device and input on ONE live page, driven entirely by
// fake sensors, so you can see the whole library at a glance with no hardware. Sensor presets,
// readouts, controls, HTML form inputs, data cards and the rich visual widgets (robot face, Leaflet
// map, 3D cube, thermal heatmap, terminal) all animate at once.
//
// Runs on ESP32 and ESP8266. Served over a plain access point — connect to "RisalDash-Demo"
// (password 12345678) and open http://192.168.4.1/. The map and image cards fetch remote tiles/JPEG,
// so those two want internet on the *client*; everything else is fully offline.
//
// The kitchen sink needs more than the default 48 widget slots, so bump the cap before the include:
#define RISAL_MAX_WIDGETS 96
#include <RisalUI.h>
#include <RisalFake.h>
#include <math.h>

RisalUI dash("RisalDash Showcase");

// ── Fake sensor bundles (no hardware) ──
RisalFakeEnv     env;
RisalFakeWeather wx;
RisalFakePower   pwr;
RisalFakeAir     air;
RisalFakeIMU     imu;
RisalFakeGPS     gps;

// ── Bound values ──
float temp = 24, hum = 55, pres = 1013, lux = 400;          // environment (BME280 preset)
float wind = 8, gust = 14, uv = 3;                           // weather
float volts = 12.1, amps = 1.4, watt = 17, tvoc = 120;      // power + air
float cpu = 42, rssi = -52;
int   ram = 60, status = 0;                                  // status: 0 ok · 1 warn · 2 fault
bool  linkUp = true;
// controls
int   bright = 128, fanSpeed = 2, mode = 1, lightMode = 0;
bool  pump = false;
// HTML inputs (String-bound)
String devName = "showcase-01", note = "All systems nominal.", wifiKey = "";
String boot = "08:30", svcDay = "2026-07-20", ledColor = "#22d3ee";
// visual
int   mood = 1;
float gpsLat = 41.311, gpsLon = 69.279, pitch = 0, roll = 0, yaw = 0;
// device cards
bool  plugOn = true, plugOnline = true;
String sumHead = "12 devices online", sumDetail = "Everything nominal · updated just now";
int   sumMood = 1;

LogWidget*      eventLog = nullptr;
TerminalWidget* term = nullptr;
HeatmapWidget*  heat = nullptr;
static const int TW = 16, TH = 12;
float thermal[TW * TH];

void setup() {
  dash.timezone(180);
  dash.brand("Risal<b>Dash</b>");

  // ── Sensors (presets expand into the right widgets automatically) ──
  dash.separator("Sensors");
  dash.sensor("bme280", &temp, &hum, &pres).chart();   // temp + humidity + pressure, with trend charts
  dash.gauge("Wind", &wind, 0, 40, "km/h").variant("semi");
  dash.gauge("UV index", &uv, 0, 12, "").variant("bar");
  dash.metric("Lux", &lux, "lx").decimals(0);

  // ── Readouts ──
  dash.separator("Readouts");
  dash.metric("CPU", &cpu, "%").decimals(0).zone(70, 90);
  dash.gauge("Voltage", &volts, 0, 14, "V");               // ring (default)
  dash.stat("RSSI", &rssi, "dBm").decimals(0);
  dash.progress("RAM", &ram, "%");
  dash.badge("Status", &status).labels("ok", "warn", "fault");
  dash.led("Link", &linkUp);
  dash.label("Device", &devName);

  // ── Controls ──
  dash.separator("Controls");
  dash.toggle("Pump", &pump, [](bool on) { status = on ? 0 : 1; if (eventLog) eventLog->print(String("Pump ") + (on ? "on" : "off")); });
  dash.slider("Brightness", &bright, 0, 255);
  dash.number("Fan speed", &fanSpeed, 0, 5, 1);
  dash.select("Mode", "Eco,Comfort,Boost", &mode);
  dash.radio("Light", "Off,Warm,Cool", &lightMode);
  dash.button("Reboot", "Restart", []() { ESP.restart(); });

  // ── HTML form inputs ──
  dash.separator("Inputs");
  dash.text("Hostname", &devName);
  dash.textarea("Notes", &note);
  dash.password("Wi-Fi key", &wifiKey);
  dash.time("Boot time", &boot);
  dash.date("Service day", &svcDay);
  dash.color("LED color", &ledColor);

  // ── Data cards ──
  dash.separator("Data");
  dash.summary("Fleet", &sumHead, &sumDetail, &sumMood);
  dash.deviceCard("Smart plug", "🔌", &plugOn, &plugOnline);
  dash.table("Live values")
      .row("Voltage", &volts, "V", 1)
      .row("Current", &amps, "A", 2)
      .row("Power", &watt, "W", 0)
      .row("TVOC", &tvoc, "ppb", 0);
  eventLog = &dash.log("Events", 6);
  dash.ai("Assistant", &note);

  // ── Rich visual widgets ──
  dash.separator("Visual");
  dash.face("Robot", &mood).size(RSIZE_L);
  dash.map("Track", &gpsLat, &gpsLon).size(RSIZE_L);        // client needs internet (Leaflet tiles)
  dash.cube("Orientation", &pitch, &roll, &yaw).size(RSIZE_L);
  heat = &dash.heatmap("Thermal cam", TW, TH);
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

uint32_t lastLog = 0, lastHeat = 0, lastSensor = 0;

void loop() {
  uint32_t now = millis();

  if (now - lastSensor > 200) {   // refresh every fake bundle
    lastSensor = now;
    env.update(); wx.update(); pwr.update(); air.update(); imu.update(); gps.update();
    temp = env.temperature(); hum = env.humidity(); pres = env.pressure(); lux = env.light();
    wind = wx.windSpeed(); gust = wx.gust(); uv = wx.uvIndex();
    volts = pwr.voltage(); amps = pwr.current(); watt = pwr.power(); tvoc = air.tvoc();
    pitch = imu.pitch(); roll = imu.roll(); yaw = imu.yaw();
    gpsLat = gps.lat(); gpsLon = gps.lon();
    cpu = 30 + (now / 200 % 55);
    ram = (int)(ESP.getFreeHeap() / 1024) % 100;
    rssi = -50 - (int)(10 * sinf(now * 0.0004f));
  }

  if (eventLog && now - lastLog > 4000) {
    lastLog = now;
    eventLog->print("tick " + String(now / 1000) + "s  heap=" + ESP.getFreeHeap());
  }

  if (heat && now - lastHeat > 500) {   // fake thermal frame — a moving hotspot
    lastHeat = now;
    float ph = now * 0.001f;
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
