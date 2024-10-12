#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <ESP8266WebServer.h>

#define WIFI_SSID         "azak0"
#define WIFI_PASS         "obrigado"
#define APP_KEY           "davif2530a67-b480-4886-b47a-3ddae8aa3bee"
#define APP_SECRET        "davib5cd3991-4e00-41ad-bf0b-a0ae80765bcc-67459ab0-4860-4f47-a818-8d01853112e2"

#define SWITCH_ID_1       "davi64c197152ac6a1822a925f58"
#define RELAYPIN_1        0

#define BAUD_RATE         115200

ESP8266WebServer server(80);

String getWebpage(bool relayState) {
  String buttonText = relayState ? "OFF" : "ON";
  String webpage = "<html><head><title>ESP Relay Control</title></head><body>";
  webpage += "<center><h2>Control Relay</h2>";
  webpage += "<form id='toggleForm' action='/toggle' method='get'>";
  webpage += "<button type='submit' onclick='reloadPage()'>Turn Relay " + buttonText + "</button>";
  webpage += "</form>";
  webpage += "<script>function reloadPage() { document.getElementById('toggleForm').submit(); setTimeout(() => { location.reload(); }, 2000); }</script>";
  webpage += "</center></body></html>";
  return webpage;
}

void handleRoot() {
  bool relayState = digitalRead(RELAYPIN_1) == LOW;
  server.send(200, "text/html", getWebpage(relayState));
}

void handleToggle() {
  digitalWrite(RELAYPIN_1, !digitalRead(RELAYPIN_1));
  server.sendHeader("Location", "/", true);  // Redirect to root after toggle
  server.send(302, "text/plain", "Toggle OK");
}

bool onPowerState1(const String &deviceId, bool &state) {
  digitalWrite(RELAYPIN_1, state ? LOW : HIGH );
  return true;
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
}

void setupSinricPro() {
  pinMode(RELAYPIN_1, OUTPUT);
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  
  setupWiFi();
  setupSinricPro();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);

  server.begin();
}

void loop() {
  server.handleClient();
  SinricPro.handle();
}
