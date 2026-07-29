// Room monitor — an all-in-one safety & comfort panel for a single room with NO hardware: climate
// (DHT22), motion (PIR), gas/smoke (MQ-2), a 2-channel relay (light + fan) and a buzzer. The MQ-2
// occasionally "smells smoke" and the PIR fires at random so you can watch the alarm chain: badge ->
// buzzer -> event log. Remote control lives on the dashboard, and every widget is an MCP tool, so an
// AI agent can read the room and flip the fan too. Swap the fakes for the real sensors later.
// Served over a plain access point — connect to "RisalDash-Room" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Room");

RisalFakeEnv env;                        // DHT22 stand-in (temp + humidity day cycle)
RisalFake gasFake(180, 120, 8, 0.9f);    // MQ-2 ppm-ish reading, drifts and spikes

float temp = 24, hum = 55, gas = 180;
bool  motion = false, light = false, fan = false, alarmOut = false;
int   roomState = 0;                     // 0 all clear · 1 motion · 2 GAS  -> badge
LogWidget* evlog = nullptr;

void setup() {
  dash.timezone(180);

  dash.layout("Room", RICON_HOME);
  dash.sensor("dht22", &temp, &hum);            // temp gauge + humidity metric
  dash.sensor("mq2", &gas);                     // smoke/LPG gauge with the right range
  dash.badge("Status", &roomState).labels("All clear", "Motion", "GAS!");
  dash.led("Motion", &motion);                  // PIR state, live
  evlog = &dash.log("Events", 6);

  dash.layout("Control", RICON_BULB);
  dash.toggle("Light", &light, [](bool on) { (void)on; /* digitalWrite(RELAY1, on) */ });
  dash.toggle("Fan", &fan, [](bool on) { (void)on; /* digitalWrite(RELAY2, on) */ });
  dash.led("Alarm", &alarmOut);                 // buzzer output

  dash.beginAP("RisalDash-Room", "12345678");
  env.begin();
}

uint32_t last = 0, motionUntil = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {
    last = millis();
    env.update();
    temp = env.temperature(); hum = env.humidity();
    gas = gasFake.read() + (random(1000) < 4 ? 260 : 0);   // rare smoke spikes

    // PIR: random visits, each held ~3 s like a real sensor's retrigger window.
    if (random(1000) < 6) { motionUntil = millis() + 3000; if (!motion && evlog) evlog->print("Motion detected"); }
    motion = millis() < motionUntil;

    // Alarm chain: gas beats motion; the buzzer follows the gas state, fan auto-vents.
    bool gasBad = gas > 350;                               // real: calibrate to your MQ-2
    if (gasBad && roomState != 2 && evlog) evlog->print("GAS/SMOKE - venting");
    roomState = gasBad ? 2 : (motion ? 1 : 0);
    alarmOut = gasBad;
    if (gasBad) fan = true;                                // auto-vent on gas
  }
}
