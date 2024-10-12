#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#define RELAYPIN_1        0
#define AP_SSID           "ESP-AP-Setup"
#define AP_PASS           "password"

// Variáveis globais para armazenar as configurações
String ssid, password, appKey, appSecret, switchId;
bool relayState = false;  // Variável para armazenar o estado do relé

ESP8266WebServer server(80);

IPAddress local_IP(192, 168, 40, 1);  // IP que deseja para o ESP
IPAddress gateway(192, 168, 40, 1);
IPAddress subnet(255, 255, 255, 0);

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Controle Relé</title></head><body>";
  html += "<h1>Bem-vindo ao Controle do Relé</h1>";
  html += "<p><a href='/setup'>Configurar WiFi e Tokens Sinric</a></p>";
  html += "<p><a href='/status'>Verificar Estado</a></p>";
  html += "<p><a href='/ligar'>Ligar Relé</a></p>";
  html += "<p><a href='/desligar'>Desligar Relé</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

<!DOCTYPE html><html><head><meta charset='UTF-8'>

void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  appKey = server.arg("appKey");
  appSecret = server.arg("appSecret");
  switchId = server.arg("switchId");

  EEPROM.begin(512);
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(i, i < ssid.length() ? ssid[i] : 0);
  }
  for (int i = 32; i < 96; ++i) {
    EEPROM.write(i, i - 32 < password.length() ? password[i - 32] : 0);
  }
  for (int i = 96; i < 160; ++i) {
    EEPROM.write(i, i - 96 < appKey.length() ? appKey[i - 96] : 0);
  }
  for (int i = 160; i < 224; ++i) {
    EEPROM.write(i, i - 160 < appSecret.length() ? appSecret[i - 160] : 0);
  }
  for (int i = 224; i < 288; ++i) {
    EEPROM.write(i, i - 224 < switchId.length() ? switchId[i - 224] : 0);
  }
  EEPROM.commit();

  String message = "Configurações salvas! Tentando conectar-se à rede WiFi " + ssid;
  server.send(200, "text/html", message);

  delay(2000);
  ESP.restart();
}

void handleStatus() {
  String state = relayState ? "Ligado" : "Desligado";
  String json = "{\"relayState\": \"" + state + "\"}";
  server.send(200, "application/json", json);
}

void handleLigar() {
  digitalWrite(RELAYPIN_1, LOW);  // Liga o relé
  relayState = true;
  server.send(200, "text/html", "Relé Ligado");
}

void handleDesligar() {
  digitalWrite(RELAYPIN_1, HIGH);  // Desliga o relé
  relayState = false;
  server.send(200, "text/html", "Relé Desligado");
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconectado!");
    } else {
      Serial.println("\nFalha ao reconectar WiFi");
    }
  }
}

void handleCaptivePortal() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='0; url=http://" + WiFi.softAPIP().toString() + "/setup' />";
  html += "</head></html>";
  server.send(200, "text/html", html);
}

void connectToWiFi() {
  EEPROM.begin(512);
  char storedSSID[32];
  char storedPassword[64];
  char storedAppKey[64];
  char storedAppSecret[64];
  char storedSwitchId[64];

  for (int i = 0; i < 32; ++i) {
    storedSSID[i] = char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i) {
    storedPassword[i - 32] = char(EEPROM.read(i));
  }
  for (int i = 96; i < 160; ++i) {
    storedAppKey[i - 96] = char(EEPROM.read(i));
  }
  for (int i = 160; i < 224; ++i) {
    storedAppSecret[i - 160] = char(EEPROM.read(i));
  }
  for (int i = 224; i < 288; ++i) {
    storedSwitchId[i - 224] = char(EEPROM.read(i));
  }

  storedSSID[31] = '\0';
  storedPassword[63] = '\0';
  storedAppKey[63] = '\0';
  storedAppSecret[63] = '\0';
  storedSwitchId[63] = '\0';

  ssid = String(storedSSID);
  password = String(storedPassword);
  appKey = String(storedAppKey);
  appSecret = String(storedAppSecret);
  switchId = String(storedSwitchId);

  if (ssid.length() > 0) {
    WiFi.begin(storedSSID, storedPassword);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
      delay(500);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado ao WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha na conexão ao WiFi, iniciando em modo AP");
  }

  // Inicia o AP mesmo quando conectado ao WiFi
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("Ponto de Acesso ativo");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAYPIN_1, OUTPUT);
  digitalWrite(RELAYPIN_1, HIGH);
  relayState = false;

  connectToWiFi();

  SinricProSwitch& mySwitch1 = SinricPro[switchId.c_str()];
  mySwitch1.onPowerState([](const String &deviceId, bool &state) -> bool {
    digitalWrite(RELAYPIN_1, state ? LOW : HIGH);
    relayState = state;
    return true;
  });
  SinricPro.begin(appKey.c_str(), appSecret.c_str());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/setup", HTTP_GET, handleSetup);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/ligar", HTTP_GET, handleLigar);
  server.on("/desligar", HTTP_GET, handleDesligar);
  server.onNotFound(handleCaptivePortal);
  server.begin();
}

void loop() {
  server.handleClient();
  SinricPro.handle();

  // Verifica o status do WiFi e tenta reconectar, se necessário
  reconnectWiFi();
}
