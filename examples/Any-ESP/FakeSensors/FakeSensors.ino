// Fake-sensor tour: build and debug a full dashboard with NO hardware attached.
// <RisalFake.h> ships five stand-ins, all shown here on their own pages:
//   RisalFake     — one realistic drifting value (drift + wobble + noise)
//   RisalFakeEnv  — a greenhouse bundle with a day/night cycle (temp/hum/pressure/soil/light/air)
//   RisalFakeGPS  — plays a waypoint route on a loop (drives the map widget)
//   RisalFakeBLE  — a plausible nearby-device scan, incl. a decoded Xiaomi beacon
//   RisalRecorder — record a live signal, then replay it forever (great for demos)
// When real hardware arrives, swap each read for your driver's — variable names stay,
// the dashboard code doesn't change. Served over a plain access point — connect to
// "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Fake lab");

// ── RisalFake: one wandering value per instance ──
RisalFake voltsFake(12.4f, 0.3f, 0.05f);  // 12.4 V ± 0.3 drift + a little jitter
RisalFake wattsFake(850, 650, 40, 1.3f);  // a restless power draw
float volts = 12.4f, watts = 0;

// ── RisalFakeEnv: the whole greenhouse, one virtual day ≈ 3 minutes ──
RisalFakeEnv env;
float temp = 22, hum = 55, pres = 1013, lux = 0, vday = 8;
int soil = 60, airq = 0;

// ── RisalFakeGPS: a loop around Tashkent for the map ──
RisalFakeGPS gps;
const float ROUTE[] = {41.311f, 69.279f, 41.318f, 69.286f, 41.315f, 69.298f,
                       41.306f, 69.296f, 41.302f, 69.283f, 41.311f, 69.279f};
float lat = ROUTE[0], lon = ROUTE[1], speed = 0, heading = 0;

// ── RisalFakeBLE: fake scan results + a decoded temp/humidity beacon ──
RisalFakeBLE ble;
LogWidget* bleLog = nullptr;
float beaconTemp = 23.5f;

// ── RisalRecorder: capture the live temperature, then loop the recording ──
RisalRecorder rec;
bool replay = false;

void setup() {
  dash.timezone(180);

  dash.layout("Environment", RICON_LEAF);
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.progress("Soil", &soil, "%");
  dash.stat("Light", &lux, "lux");
  dash.stat("Pressure", &pres, "hPa");
  dash.badge("Air quality", &airq).labels("good", "fair", "poor");
  dash.stat("Virtual hour", &vday).decimals(1);  // watch the fake day tick by
  dash.toggle("Replay temp", &replay);           // RisalRecorder: loop the captured trace

  dash.layout("Power", RICON_FLASH);
  dash.gauge("Voltage", &volts, 0, 14, "V");
  dash.chart("Power", &watts, "W");

  dash.layout("Route", RICON_MOTION);
  dash.map("Track", &lat, &lon).size(RSIZE_L);   // needs internet on the client (Leaflet tiles)
  dash.stat("Speed", &speed, "km/h").decimals(0);
  dash.stat("Heading", &heading, "deg").decimals(0);

  dash.layout("BLE", RICON_SIGNAL);
  bleLog = &dash.log("Nearby", 5);
  dash.stat("Beacon temp", &beaconTemp, "C").decimals(1);  // the Xiaomi-style sensor, decoded

  dash.beginAP("RisalDash-Demo", "12345678");
  env.begin();
  gps.begin(ROUTE, 6);
}

uint32_t lastTick = 0, lastBle = 0;

void loop() {
  dash.update();

  if (millis() - lastTick > 250) {  // 4 Hz: advance every fake model
    lastTick = millis();
    env.update();
    if (replay && rec.size() > 8) {
      temp = rec.replay();          // play the captured trace on a loop
    } else {
      temp = env.temperature();
      rec.record(temp);             // keep capturing the live signal
    }
    hum = env.humidity();
    pres = env.pressure();
    lux = env.light();
    soil = env.soil();
    airq = env.airQuality();
    vday = env.hourOfDay();

    volts = voltsFake.read();
    watts = wattsFake.read();
    if (watts < 0) watts = 0;

    gps.update();
    lat = gps.lat(); lon = gps.lon(); speed = gps.speed(); heading = gps.heading();
  }

  if (bleLog && millis() - lastBle > 2000) {  // refresh the fake scan list
    lastBle = millis();
    ble.update();
    beaconTemp = ble.sensorTemp();
    for (int i = 0; i < ble.count(); i++)
      bleLog->print(String(ble.name(i)) + "  " + ble.rssi(i) + "dBm");
  }
}
