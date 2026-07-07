// Author-string translations for the served dashboard. The library localizes its own chrome
// (Settings, status bar, portal); the strings WE write — page names, widget titles, sensor
// labels, select options — need dash.translate(). One row per source string (English) with its
// ru / uz / ar form; a select's whole comma-list is one row (keep order/count).
#pragma once
#include <string.h>

struct Tr4 { const char *en, *ru, *uz, *ar; };
static const Tr4 I18N[] = {
  // pages (BLE / Zigbee stay as-is — proper nouns)
  {"Sensors", "Датчики", "Sensorlar", "المستشعرات"},
  {"Weather", "Погода", "Ob-havo", "الطقس"},
  {"Route", "Маршрут", "Marshrut", "المسار"},
  {"Energy", "Энергия", "Energiya", "الطاقة"},
  {"Motion", "Движение", "Harakat", "الحركة"},
  {"Console", "Консоль", "Konsol", "وحدة التحكم"},
  {"Thermal", "Тепло", "Termal", "حراري"},
  {"Robot", "Робот", "Robot", "روبوت"},
  {"Control", "Управление", "Boshqaruv", "التحكم"},
  // widget titles
  {"Air temp", "Темп. воздуха", "Havo harorati", "حرارة الهواء"},
  {"Humidity", "Влажность", "Namlik", "الرطوبة"},
  {"Soil moisture", "Влажн. почвы", "Tuproq namligi", "رطوبة التربة"},
  {"Light", "Свет", "Yorugʻlik", "الضوء"},
  {"Replay temp", "Повтор темп.", "Haroratni qayta", "إعادة الحرارة"},
  {"City", "Город", "Shahar", "المدينة"},
  {"Place", "Место", "Joy", "المكان"},
  {"Outside", "Снаружи", "Tashqarida", "بالخارج"},
  {"Sky", "Небо", "Osmon", "السماء"},
  {"Wind", "Ветер", "Shamol", "الرياح"},
  {"Track", "Трек", "Trek", "المسار"},
  {"Speed", "Скорость", "Tezlik", "السرعة"},
  {"Heading", "Курс", "Yoʻnalish", "الاتجاه"},
  {"Power", "Мощность", "Quvvat", "القدرة"},
  {"Today", "Сегодня", "Bugun", "اليوم"},
  {"Cost", "Стоимость", "Narx", "التكلفة"},
  {"Tariff c/kWh", "Тариф ¢/кВтч", "Tarif ¢/kVts", "التعرفة ¢/كوس"},
  {"Nearby", "Рядом", "Yaqinda", "القريبة"},
  {"Orientation", "Ориентация", "Orientatsiya", "الاتجاه"},
  {"Pitch", "Тангаж", "Nishab", "الميل"},
  {"Roll", "Крен", "Ogʻish", "الدوران"},
  {"Shell", "Терминал", "Terminal", "الطرفية"},
  {"Thermal cam", "Теплокамера", "Termokamera", "كاميرا حرارية"},
  {"Devices", "Устройства", "Qurilmalar", "الأجهزة"},
  {"Living light", "Свет в зале", "Zal chirogʻi", "ضوء الصالة"},
  {"Smart plug", "Умная розетка", "Aqlli rozetka", "مقبس ذكي"},
  {"Door", "Дверь", "Eshik", "الباب"},
  {"Emotion", "Эмоция", "Emotsiya", "المشاعر"},
  {"Auto emotion", "Авто-эмоция", "Avto emotsiya", "مشاعر تلقائية"},
  {"Auto-slide", "Автолистание", "Avto almashish", "تمرير تلقائي"},
  {"Show", "Показать", "Koʻrsatish", "إظهار"},
  {"Slide sec", "Секунды", "Soniya", "ثوانٍ"},
  {"Backlight", "Подсветка", "Orqa yoritish", "الإضاءة الخلفية"},
  {"LED mode", "Режим LED", "LED rejimi", "وضع LED"},
  {"LED colour", "Цвет LED", "LED rangi", "لون LED"},
  {"Pump", "Насос", "Nasos", "المضخة"},
  // sensor("ld2410") preset labels (come from the library's sensor table)
  {"Presence", "Присутствие", "Mavjudlik", "الوجود"},
  {"Distance", "Дистанция", "Masofa", "المسافة"},
  // select option lists — one row translates the whole comma-list (keep the order/count)
  {"Neutral,Happy,Sad,Angry,Surprised,Sleepy,Love,Wink,Dizzy,Look",
   "Спокойствие,Радость,Грусть,Злость,Удивление,Сонливость,Любовь,Подмигивание,Головокружение,Взгляд",
   "Xotirjam,Xursand,Gʻamgin,Jahl,Hayron,Uyquchan,Sevgi,Koʻz qisish,Boshaylanish,Qarash",
   "محايد,سعيد,حزين,غاضب,متفاجئ,ناعس,حب,غمزة,دوار,نظرة"},
  {"Off,Manual,Per-widget,Gradient",
   "Выкл,Ручной,По виджету,Градиент",
   "Oʻchiq,Qoʻlda,Vidjet boʻyicha,Gradient",
   "إيقاف,يدوي,لكل أداة,تدرّج"},
  {"Address,Air temp,Humidity,Soil,Pressure,Air quality,Trend,Robot,Weather,Pump,Backlight,Overview,Thermal",
   "Адрес,Темп. воздуха,Влажность,Почва,Давление,Качество,Тренд,Робот,Погода,Насос,Подсветка,Обзор,Тепло",
   "Manzil,Havo harorati,Namlik,Tuproq,Bosim,Sifat,Trend,Robot,Ob-havo,Nasos,Yorugʻlik,Umumiy,Termal",
   "العنوان,حرارة الهواء,الرطوبة,التربة,الضغط,الجودة,الاتجاه,روبوت,الطقس,المضخة,الإضاءة,نظرة عامة,حراري"},
};

// Registered with dash.translate(); called at render time for every author string.
inline const char *trWeb(const char *s, const char *lang) {
  int c = lang[0] == 'r' ? 1 : (lang[0] == 'u' && lang[1] == 'z') ? 2 : lang[0] == 'a' ? 3 : 0;
  if (c == 0) return s;  // English: nothing to translate
  for (auto &t : I18N)
    if (strcmp(t.en, s) == 0) return c == 1 ? t.ru : c == 2 ? t.uz : t.ar;
  return s;  // unmapped -> leave as written
}
