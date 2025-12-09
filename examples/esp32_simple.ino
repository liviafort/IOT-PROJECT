/*
 * ESP32/ESP8266 - MQTT IoT Gateway
 *
 * Envia dados de sensores para o Raspberry Pi via MQTT
 *
 * Biblioteca necessária:
 * - PubSubClient: https://github.com/knolleary/pubsubclient
 *
 * CONFIGURAÇÃO:
 * 1. Ajuste DEVICE_ID para um ID único (ex: esp32_sala_01)
 * 2. Faça upload no ESP32/ESP8266
 * 3. O ESP conectará automaticamente no WiFi do Raspberry Pi
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================================
// CONFIGURAÇÃO - AJUSTE AQUI!
// ============================================================================

// WiFi do Raspberry Pi (Access Point)
const char* WIFI_SSID = "RPi-IoT-Gateway";        // WiFi criado pelo Raspberry Pi
const char* WIFI_PASSWORD = "iotgateway2024";     // Senha padrão (mude se alterou no setup)

// MQTT Broker (Raspberry Pi)
const char* MQTT_SERVER = "192.168.50.1";         // IP fixo do Raspberry Pi
const int MQTT_PORT = 1883;

// Identificação única deste dispositivo
const char* DEVICE_ID = "esp32_sala_01";          // ← MUDE PARA CADA ESP32!
                                                   // Exemplos: esp32_quarto_01, esp32_cozinha_01

// ============================================================================
// NÃO ALTERE DAQUI PARA BAIXO
// ============================================================================

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSend = 0;
const long SEND_INTERVAL = 5000;  // Enviar a cada 5 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n================================");
  Serial.println("ESP32 IoT - MQTT Sensor");
  Serial.println("================================");
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);
  Serial.println("================================\n");

  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // Enviar dados do sensor
  if (millis() - lastSend > SEND_INTERVAL) {
    lastSend = millis();
    sendSensorData();
  }
}

void connectWiFi() {
  Serial.print("Conectando WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nERRO: WiFi falhou!");
    Serial.println("Verifique se o Raspberry Pi está ligado");
    Serial.println("Reiniciando em 10s...");
    delay(10000);
    ESP.restart();
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT... ");

    if (client.connect(DEVICE_ID)) {
      Serial.println("OK!");
    } else {
      Serial.print("FALHOU! Código: ");
      Serial.println(client.state());
      Serial.println("Tentando novamente em 5s...");
      delay(5000);
    }
  }
}

void sendSensorData() {
  // Simular leitura de sensores (substitua com sensores reais)
  float temperatura = random(150, 350) / 10.0;  // 15.0 a 35.0°C
  float umidade = random(300, 900) / 10.0;      // 30.0 a 90.0%
  int luminosidade = random(0, 1024);           // 0 a 1023

  // Montar JSON
  String json = "{";
  json += "\"device\":\"" + String(DEVICE_ID) + "\",";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade\":" + String(umidade, 1) + ",";
  json += "\"luz\":" + String(luminosidade);
  json += "}";

  // Publicar no tópico MQTT
  bool ok = client.publish("iot/sensor/dados", json.c_str());

  if (ok) {
    Serial.println("[ENVIADO] " + json);
  } else {
    Serial.println("[ERRO] Falha ao enviar!");
  }
}