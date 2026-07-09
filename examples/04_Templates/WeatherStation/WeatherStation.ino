// Weather station — wind / rain / UV with NO hardware. dash.sensor("weather", …) expands the
// anemometer + vane + tipping-bucket + UV preset; RisalFakeWeather drives it with gusty wind, a
// slowly veering direction, passing rain showers that accumulate, and a UV index that follows a day
// cycle. A BME280 page adds temperature / humidity / pressure. Swap the fakes for real drivers later.
// Served over a plain access point — connect to "RisalDash-Weather" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Weather");

RisalFakeWeather wx;
RisalFakeEnv env;   // reuse the greenhouse bundle for the ambient page
float wind = 8, gust = 8, dir = 180, rain = 0, uv = 0;
float temp = 22, hum = 55, pres = 1013;

void setup() {
  dash.timezone(180);

  dash.layout("Weather", RICON_WATER);
  dash.sensor("weather", &wind, &gust, &dir, &rain);  // km/h gauge · gust · direction · rain total
  dash.gauge("UV index", &uv, 0, 12);

  dash.layout("Ambient", RICON_THERMOMETER);
  dash.sensor("bme280", &temp, &hum, &pres).chart();

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
  }
}
