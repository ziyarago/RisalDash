// GPS tracker — a live-map vehicle tracker dashboard with NO hardware. RisalFakeGPS replays a fixed
// route on a loop (interpolating between waypoints), driving the map marker plus speed/heading stats,
// exactly like a NEO-6M feed. Swap in TinyGPS++ reads and the same lat/lon/speed variables drive the UI.
// Served over a plain access point — connect to "RisalDash-GPS" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("GPS Tracker");
RisalFakeGPS gps;

float lat = 41.311, lon = 69.240, speed = 0, heading = 0;

// A short city loop (Tashkent) as a flat {lat, lon, lat, lon, ...} array.
const float route[] = {
  41.3110, 69.2401,  41.3155, 69.2490,  41.3120, 69.2585,
  41.3050, 69.2560,  41.3030, 69.2450,  41.3075, 69.2390,
};

void setup() {
  dash.timezone(300);
  dash.accent(0);

  dash.layout("Live", RICON_SIGNAL);
  dash.map("Location", &lat, &lon);
  dash.stat("Speed", &speed, "km/h");
  dash.stat("Heading", &heading, "deg");
  dash.chart("Speed trend", &speed, "km/h");

  gps.begin(route, sizeof(route) / sizeof(float) / 2);
  dash.beginAP("RisalDash-GPS", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 200) {
    last = millis();
    gps.update();  // real: while (Serial1.available()) gps.encode(Serial1.read());
    lat = gps.lat();
    lon = gps.lon();
    speed = gps.speed();
    heading = gps.heading();
  }
}
