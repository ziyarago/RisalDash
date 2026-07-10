// Smart thermostat — a WiFi thermostat dashboard with NO hardware. RisalFakeEnv drives a realistic
// day/night room temperature; the sketch runs simple hysteresis around a live setpoint and toggles a
// "heater" relay. Swap the fake for a DS18B20/DHT read and drive a real relay pin — the UI is unchanged.
// Served over a plain access point — connect to "RisalDash-Thermostat" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Thermostat");
RisalFakeEnv env;

float roomT = 21, roomH = 55;
int   setpoint = 22;
bool  heating = false, hold = false;

void setup() {
  dash.timezone(180);
  dash.accent(0);  // users can change the accent live in Settings

  dash.layout("Climate", RICON_THERMOMETER);
  dash.gauge("Room temp", &roomT, 0, 40, "C");
  dash.gauge("Humidity", &roomH, 0, 100, "%").variant("semi");
  dash.chart("Trend", &roomT, "C");

  dash.layout("Control", RICON_POWER);
  dash.number("Setpoint", &setpoint, 5, 35, 1, [](int v) { (void)v; /* new target */ });
  dash.led("Heating", &heating);
  dash.toggle("Hold", &hold, [](bool on) { (void)on; /* pause automation */ });

  env.begin();
  dash.beginAP("RisalDash-Thermostat", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    env.update();
    roomT = env.temperature();  // real: roomT = ds18b20.getTempC();
    roomH = env.humidity();
    // Hysteresis: heat below setpoint-0.3, stop above setpoint+0.3 (skipped while "held").
    if (!hold) {
      if (roomT < setpoint - 0.3f) heating = true;
      else if (roomT > setpoint + 0.3f) heating = false;
    }
    // digitalWrite(RELAY_PIN, heating);
  }
}
