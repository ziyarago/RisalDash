// Fully multilingual dashboard. The library already localizes its OWN chrome (Settings, status bar,
// captive portal) in en/ru/uz/ar. But the strings YOU write — page names, widget titles, button
// labels, select options — are yours, so the library can't translate them. dash.translate() closes
// that gap: register a lookup and the whole UI switches language together.
//
// Switch language from the appbar Settings (gear) → the built-in EN/RU/UZ/AR buttons. No extra
// control needed: the picker reloads with ?lang=xx, and the translator is called with that language.
//
// Served over a plain access point — connect to "RisalDash-Demo" and open http://192.168.4.1/
#include <RisalUI.h>
#include <string.h>

RisalUI dash("RisalDash");

float cpu = 42, temp = 24.4f, hum = 55;
int   ram = 61, target = 24, mode = 0;
bool  heater = true, fan = false;

// Author-string dictionary: one row per source string with its ru/uz/ar forms. English is the
// source, returned as-is. A select's options are one comma-list translated as a unit (same order).
struct Row { const char *en, *ru, *uz, *ar; };
static const Row DICT[] = {
  {"Overview",       "Обзор",         "Umumiy",               "نظرة عامة"},
  {"Climate",        "Климат",        "Iqlim",                "المناخ"},
  {"System",         "Система",        "Tizim",                "النظام"},
  {"CPU",            "Процессор",     "Protsessor",           "المعالج"},
  {"RAM",            "Память",        "Xotira",               "الذاكرة"},
  {"Temperature",    "Температура",   "Harorat",              "الحرارة"},
  {"Humidity",       "Влажность",     "Namlik",               "الرطوبة"},
  {"Target",         "Уставка",       "Nishon",               "الهدف"},
  {"Heater",         "Обогрев",       "Isitgich",             "المدفأة"},
  {"Fan",            "Вентилятор",    "Ventilyator",          "المروحة"},
  {"Mode",           "Режим",         "Rejim",                "الوضع"},
  {"Auto,Eco,Boost", "Авто,Эко,Буст", "Avto,Tejamkor,Kuchli", "تلقائي,اقتصادي,معزّز"},
};

// Called by the library at render time for every author string. Return the localized form, or the
// original when the language is English or the string isn't mapped (mix-and-match is fine).
const char *tr(const char *s, const char *lang) {
  int col = lang[0] == 'r' ? 1 : (lang[0] == 'u' && lang[1] == 'z') ? 2 : lang[0] == 'a' ? 3 : 0;
  if (col == 0) return s;  // English: nothing to do
  for (auto &r : DICT)
    if (strcmp(r.en, s) == 0) return col == 1 ? r.ru : col == 2 ? r.uz : r.ar;
  return s;  // not in the dictionary → leave as written
}

void setup() {
  dash.timezone(180);
  dash.translate(tr);  // register the author-string translator (off until this is called)

  // ── Page 1: Overview ──
  dash.layout("Overview", RICON_HOME);
  dash.group("System");
  dash.metric("CPU", &cpu, "%").zone(70, 90);
  dash.progress("RAM", &ram, "%");
  dash.gauge("Temperature", &temp, 0, 50, "C").icon(RICON_THERMOMETER);

  // ── Page 2: Climate ──
  dash.layout("Climate", RICON_THERMOMETER);
  dash.chart("Humidity", &hum, "%").span(2);
  dash.slider("Target", &target, 16, 30, [](int v) { (void)v; /* setSetpoint(v) */ });
  dash.select("Mode", "Auto,Eco,Boost", &mode, [](int i) { (void)i; });
  dash.toggle("Heater", &heater, [](bool on) { (void)on; });
  dash.toggle("Fan", &fan, [](bool on) { (void)on; });

  dash.beginAP("RisalDash-Demo", "12345678");
}

void loop() {
  // Simulate movement so the dashboard looks alive.
  cpu = 30 + (millis() / 200 % 55);
  temp += 0.03f * ((millis() / 1000 % 2) ? 1 : -1);
  dash.update();  // push changed values to the browser over WebSocket
}
