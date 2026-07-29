// Weather station — wind / rain / UV / light with NO hardware, plus a live "conditions" readout and
// storm & heat alerts. dash.sensor("weather", …) expands the anemometer + vane + tipping-bucket + UV
// preset; RisalFakeWeather drives it with gusty wind, a slowly veering direction, passing rain
// showers that accumulate, and a UV index that follows a day cycle. A BME280 page adds temperature /
// humidity / pressure, an LDR-style light reading follows the sun. Swap the fakes for real drivers
// (DHT22/BME280 + rain gauge + anemometer + GY-ML8511 + LDR) later — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Weather" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Weather");

RisalFakeWeather wx;
RisalFakeEnv env;   // reuse the greenhouse bundle for the ambient page
float wind = 8, gust = 8, dir = 180, rain = 0, uv = 0;
float temp = 22, hum = 55, pres = 1013, lux = 0;
int   sky = 0;               // 0 clear · 1 cloudy/dim · 2 rain  -> conditions badge
bool  alarmOut = false;      // storm / heat alert -> LED + buzzer in a real build
LogWidget* evlog = nullptr;
float rainPrev = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Weather", RICON_WATER);
  dash.sensor("weather", &wind, &gust, &dir, &rain);  // km/h gauge · gust · direction · rain total
  dash.gauge("UV index", &uv, 0, 12);
  dash.badge("Conditions", &sky).labels("Clear", "Cloudy", "Rain");
  dash.led("Weather alert", &alarmOut);               // real build: red LED + buzzer
  evlog = &dash.log("Alerts", 5);

  dash.layout("Ambient", RICON_THERMOMETER);
  dash.sensor("bme280", &temp, &hum, &pres).chart();
  dash.metric("Light", &lux, "lx");                   // LDR / BH1750 reading

  dash.beginAP("RisalDash-Weather", "12345678");
  env.begin();
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 250) {   // 4 Hz
    last = millis();
    wx.update();
    wind = wx.windSpeed(); gust = wx.gust(); dir = wx.direction();
    rain = wx.rain(); uv = wx.uvIndex();
    env.update();
    temp = env.temperature(); hum = env.humidity(); pres = env.pressure();
    lux = env.light();

    // Conditions: raining if the bucket is ticking, else clear vs dim by daylight.
    bool raining = rain - rainPrev > 0.01f;
    rainPrev = rain;
    sky = raining ? 2 : (lux < 8000 ? 1 : 0);

    // Alerts: a storm gust or extreme heat trips the LED/buzzer + a log line.
    bool trip = gust > 60 || temp > 40;
    if (trip && !alarmOut && evlog)
      evlog->print(gust > 60 ? "STORM - gusts over 60 km/h" : "HEAT - over 40 C");
    alarmOut = trip;
  }
}
