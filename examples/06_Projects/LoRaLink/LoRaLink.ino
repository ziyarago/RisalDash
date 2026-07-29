// LoRa point-to-point link — two ESP32s talk over kilometres with NO internet, NO gateway, NO fees.
// Flash this sketch on both boards: set NODE_ID 'A' on one and 'B' on the other. Each node beacons a
// counter every SEND_EVERY_MS and shows what it hears from the peer — RSSI, SNR, packet counter and a
// live link log — on its own RisalDash access point. CRC is on, so corrupt packets are dropped.
//
// Library: sandeepmistry/LoRa (Library Manager: "LoRa"). Module: SX1278 (433 MHz) / SX1276 (868/915).
// Wiring (ESP32 VSPI): SCK 18 · MISO 19 · MOSI 23 · NSS 5 · RST 14 · DIO0 26 — antenna ALWAYS on!
// ⚠ Check your regional frequency (433E6 / 868E6 / 915E6) before transmitting.
#include <RisalUI.h>
#include <SPI.h>
#include <LoRa.h>

#define NODE_ID 'A'             // flash 'A' on one board, 'B' on the other
#define FREQ 433E6              // match your module + region!
#define SEND_EVERY_MS 2000

#define LORA_NSS 5
#define LORA_RST 14
#define LORA_DIO0 26

RisalUI dash("LoRa Link");

float rssi = 0, snr = 0, sent = 0, recv = 0;
bool  online = false;           // heard from the peer in the last 10 s
LogWidget* evlog = nullptr;
uint32_t lastSend = 0, lastHear = 0, txNo = 0;

void setup() {
  Serial.begin(115200);

  dash.layout("Link", RICON_SIGNAL);
  dash.stat("RSSI", &rssi, "dBm");
  dash.stat("SNR", &snr, "dB");
  dash.stat("Sent", &sent, "");
  dash.stat("Received", &recv, "");
  dash.led("Peer online", &online);
  evlog = &dash.log("Traffic", 6);
  dash.chart("RSSI trend", &rssi, "dBm");

  dash.beginAP(NODE_ID == 'A' ? "RisalDash-LoRa-A" : "RisalDash-LoRa-B", "12345678");

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(FREQ)) { if (evlog) evlog->print("LoRa init FAILED - check wiring"); return; }
  LoRa.enableCrc();                       // drop corrupt packets in hardware
  if (evlog) evlog->print(String("Node ") + (char)NODE_ID + " up @ " + String((long)(FREQ / 1E6)) + " MHz");
}

void loop() {
  dash.update();

  // Beacon: "A:123" every couple of seconds.
  if (millis() - lastSend > SEND_EVERY_MS) {
    lastSend = millis();
    LoRa.beginPacket();
    LoRa.print(String((char)NODE_ID) + ":" + String(++txNo));
    LoRa.endPacket();
    sent = txNo;
  }

  // Receive: anything not from ourselves counts as the peer.
  int sz = LoRa.parsePacket();
  if (sz) {
    String msg;
    while (LoRa.available()) msg += (char)LoRa.read();
    if (msg.length() && msg[0] != NODE_ID) {
      recv += 1;
      rssi = LoRa.packetRssi();
      snr = LoRa.packetSnr();
      lastHear = millis();
      if (evlog) evlog->print(msg + "  " + String((int)rssi) + " dBm");
    }
  }
  online = millis() - lastHear < 10000 && lastHear > 0;
}
