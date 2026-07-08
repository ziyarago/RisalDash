// Wi-Fi setup flows, end to end. dash.begin() is the whole provisioning story:
//   1st boot  -> the device raises a captive-portal AP ("WifiSetup-Demo" here): connect to it,
//                the setup page opens by itself, pick a network + password + timezone, save.
//   after     -> credentials live in NVS/EEPROM; begin() joins your Wi-Fi and serves the
//                dashboard there. This sketch shows the live connection info and a
//                "Forget Wi-Fi" button that erases the credentials and reboots back into
//                the portal — so you can walk the full circle without erasing flash.
//
// The other two bring-up modes (uncomment to try):
//   dash.begin("ssid", "password");          // fixed credentials; falls back to the portal
//   dash.beginAP("Demo", "12345678");        // no portal at all: dashboard over its own AP
#include <RisalUI.h>
#if defined(ESP32)
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

RisalUI dash("Wi-Fi Setup");

String mode = "-", ssid = "-", ip = "-";
float rssi = 0;

void setup() {
  Serial.begin(115200);

  dash.group("Connection");
  dash.label("Mode", &mode);
  dash.label("Network", &ssid);
  dash.label("IP", &ip);
  dash.stat("Signal", &rssi, "dBm").decimals(0);

  dash.group("Actions");
  // Erases the saved credentials and reboots ~1 s later — next boot raises the setup portal.
  dash.button("Forget", "Forget Wi-Fi & re-setup", []() { dash.forgetWiFi(); });

  dash.apName("WifiSetup-Demo");  // the captive-portal AP name (default: RisalDash-Setup)
  dash.begin();                   // saved Wi-Fi -> STA; nothing saved -> captive portal

  // While the portal is up, the dashboard isn't served yet — the setup page catches every
  // request. These labels are what you'll see once the device has joined your network.
  bool sta = WiFi.status() == WL_CONNECTED;
  mode = sta ? "STA — on your Wi-Fi" : "AP — setup portal";
  ssid = sta ? WiFi.SSID() : String("WifiSetup-Demo");
  ip = sta ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  Serial.printf("[net] %s  ip=%s\n", mode.c_str(), ip.c_str());
}

uint32_t lastSample = 0;

void loop() {
  dash.update();  // serves the dashboard — or the captive portal's DNS while provisioning
  if (millis() - lastSample > 1000) {
    lastSample = millis();
    rssi = WiFi.RSSI();
  }
}
