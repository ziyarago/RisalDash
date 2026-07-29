// Smart thermostat — a WiFi thermostat dashboard with NO hardware, now DUAL-MODE: a "heater" relay
// holds the setpoint from below and a "cooling fan" relay kicks in automatically when the room
// overheats past the setpoint; an overheat alert (buzzer/LED) trips a few degrees higher still.
// RisalFakeEnv drives a realistic day/night room temperature so both sides of the hysteresis fire.
// Swap the fake for a DHT22/DS18B20 read and drive real relay pins — the UI is unchanged.
// Served over a plain access point — connect to "RisalDash-Thermostat" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Thermostat");
RisalFakeEnv env;

float roomT = 21, roomH = 55;
int   setpoint = 22;
bool  heating = false, fan = false, hold = false, overheat = false;
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);
  dash.accent(0);  // users can change the accent live in Settings

  dash.layout("Climate", RICON_THERMOMETER);
  dash.gauge("Room temp", &roomT, 0, 40, "C");
  dash.gauge("Humidity", &roomH, 0, 100, "%").variant("semi");
  dash.chart("Trend", &roomT, "C");            // the history everyone wants on a thermostat

  dash.layout("Control", RICON_POWER);
  dash.number("Setpoint", &setpoint, 5, 35, 1, [](int v) { (void)v; /* new target */ });
  dash.led("Heating", &heating);               // relay 1
  dash.led("Cooling fan", &fan);               // relay 2 — auto-cooling
  dash.led("Overheat!", &overheat);            // buzzer + red LED in a real build
  dash.toggle("Hold", &hold, [](bool on) { (void)on; /* pause automation */ });
  evlog = &dash.log("Events", 5);

  env.begin();
  dash.beginAP("RisalDash-Thermostat", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    env.update();
    roomT = env.temperature();  // real: dht.readTemperature();
    roomH = env.humidity();
    if (!hold) {
      // Heating hysteresis: heat below setpoint-0.3, stop above setpoint+0.3.
      if (roomT < setpoint - 0.3f) heating = true;
      else if (roomT > setpoint + 0.3f) heating = false;
      // Auto-cooling: fan on when the room runs 1.5 C past the setpoint, off on recovery.
      if (roomT > setpoint + 1.5f) { if (!fan && evlog) evlog->print("Overshoot - fan ON"); fan = true; }
      else if (roomT < setpoint + 0.5f) fan = false;
    }
    // Overheat alert regardless of hold: something is wrong (sun on the sensor, heater stuck).
    bool hot = roomT > setpoint + 4.0f;
    if (hot && !overheat && evlog) evlog->print("OVERHEAT - check the heater");
    overheat = hot;
    // digitalWrite(HEAT_RELAY, heating);  digitalWrite(FAN_RELAY, fan || overheat);
  }
}
