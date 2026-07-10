// RFID access control — a smart-lock dashboard with NO hardware. The sketch fakes card scans (mostly
// known cards, some unknown), keeps an access log with grant/deny counters, and auto-relocks. Swap the
// fake scan for an MFRC522/PN532 read and drive a solenoid/relay lock — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-Access" and open http://192.168.4.1/
#include <RisalUI.h>

RisalUI dash("Access Control");

LogWidget* aclog = nullptr;
bool   locked = true;
int    grants = 0, denials = 0;
String lastCard = "-";

const char* known[] = {"A1:B2:C3:D4", "11:22:33:44", "DE:AD:BE:EF"};  // your whitelist

void setup() {
  dash.timezone(180);
  dash.accent(2);

  dash.layout("Door", RICON_HOME);
  dash.led("Locked", &locked);
  dash.badge("Grants today", &grants);
  dash.badge("Denied", &denials);
  dash.label("Last card", &lastCard);
  dash.button("Unlock", "Momentary unlock", []() {
    locked = false;
    if (aclog) aclog->print("Remote unlock");
  });

  dash.layout("Log", RICON_CLOCK);
  aclog = &dash.log("Access log", 6);

  dash.beginAP("RisalDash-Access", "12345678");
}

uint32_t last = 0, lastScan = 0, nextGap = 7000;
void loop() {
  dash.update();
  uint32_t now = millis();
  if (now - last > 200) {
    last = now;
    if (now - lastScan > nextGap) {  // real: if (rfid.PICC_IsNewCardPresent()) { ... }
      lastScan = now;
      nextGap = random(5000, 12000);
      bool auth = random(0, 10) < 7;  // 70 % of scans are whitelisted cards
      if (auth) {
        lastCard = String(known[random(0, 3)]);
        grants++;
        locked = false;
        if (aclog) aclog->print("Granted " + lastCard);
      } else {
        char buf[18];
        sprintf(buf, "%02X:%02X:%02X:%02X", (int)random(256), (int)random(256), (int)random(256), (int)random(256));
        lastCard = String(buf);
        denials++;
        if (aclog) aclog->print("DENIED " + lastCard);
      }
    }
    if (!locked && now - lastScan > 2000) locked = true;  // auto re-lock 2 s after a grant
  }
}
