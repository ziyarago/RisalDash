// ESP32-S3 dual-core demo. The RisalDash web dashboard runs on one core while a HEAVY worker task
// runs on the OTHER core — real parallelism. On a dual-core part (S3 / classic ESP32) the worker does
// a big blocking computation every cycle, yet the dashboard stays perfectly smooth because it is on a
// different core. The dashboard shows each part's core id and the worker's throughput, so you can see
// "loop core" and "worker core" are different. (On the single-core C6 the task would only preempt.)
//
// No extra hardware — sensors are faked with RisalFake, so it runs on a bare S3/ESP32 dev board.
// This is the concrete "move heavy/blocking work off the UI loop" case, taken to a second core.
// Board: ESP32-S3 (also runs on any dual-core ESP32).
#include <RisalUI.h>
#include <RisalFake.h>
#include <WiFi.h>
#include <math.h>

RisalUI dash("S3 Dual-core");
RisalFakeEnv env;

float temp = 0, hum = 0;
float loopCore = 0, workerCore = -1, workerRate = 0;  // shown on the dashboard (stats take float*)
volatile uint32_t workerTicks = 0;                    // incremented by the worker task

// Heavy worker, pinned to core 0. loop() runs on core 1 (Arduino's APP_CPU) — so this blocking crunch
// runs truly in parallel and never stalls the web server.
void workerTask(void *) {
  workerCore = xPortGetCoreID();
  for (;;) {
    volatile double acc = 0;  // a deliberately heavy blocking computation
    for (int i = 1; i < 1500000; i++) acc += sqrt((double)i);
    workerTicks++;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200);
  env.begin();

  dash.group("Sensors");
  dash.gauge("Air temp", &temp, 0, 50, "C");
  dash.metric("Humidity", &hum, "%");
  dash.group("Cores");
  dash.stat("Loop core", &loopCore).decimals(0);       // 1 on a dual-core part
  dash.stat("Worker core", &workerCore).decimals(0);   // 0 — a different core → real parallelism
  dash.stat("Worker ops/s", &workerRate).decimals(0);  // keeps climbing while the crunch runs
  dash.beginAP("RisalDash-S3", "12345678");

  // Pin the worker to core 0 (PRO_CPU). Arduino's loop()/setup() run on core 1 (APP_CPU).
  xTaskCreatePinnedToCore(workerTask, "worker", 4096, nullptr, 1, nullptr, 0);
}

uint32_t lastSample = 0, lastRate = 0, lastHb = 0;
uint32_t prevTicks = 0;
void loop() {
  dash.update();
  loopCore = xPortGetCoreID();  // which core the dashboard loop runs on (1 on dual-core)

  if (millis() - lastSample > 250) {
    lastSample = millis();
    env.update();
    temp = env.temperature();
    hum = env.humidity();
  }
  if (millis() - lastRate > 1000) {  // worker throughput (proves it runs in parallel)
    lastRate = millis();
    workerRate = workerTicks - prevTicks;
    prevTicks = workerTicks;
  }
  if (millis() - lastHb > 3000) {
    lastHb = millis();
    Serial.printf("[hb] loop@core%d  worker@core%.0f  ops/s=%.0f  heap=%u\n",
                  xPortGetCoreID(), workerCore, workerRate, ESP.getFreeHeap());
  }
  delay(5);
}
