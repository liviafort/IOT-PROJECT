#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #error "Plataforma não suportada! Use ESP32 ou ESP8266"
#endif

#include <PubSubClient.h>

const char* DEVICE_ID = "esp32_sala_01";

const char* WIFI_SSID = "RPi-IoT-Gateway";
const char* WIFI_PASSWORD = "iotgateway2024";

const char* MQTT_SERVER = "192.168.50.1";
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSend = 0;
const long SEND_INTERVAL = 5000;  // Enviar a cada 5 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nESP32 IoT - MQTT Sensor");
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);

  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[AVISO] WiFi desconectado! Reconectando...");
    connectWiFi();
  }

  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  if (millis() - lastSend > SEND_INTERVAL) {
    lastSend = millis();
    sendSensorData();
  }
}

void connectWiFi() {
  Serial.print("\nConectando WiFi: ");
  Serial.println(WIFI_SSID);
  Serial.print("Procurando Raspberry Pi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK!");
    Serial.print("IP atribuído: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
  } else {
    Serial.println(" FALHOU!");
    Serial.println("[ERRO] Não conseguiu conectar ao Raspberry Pi");
    Serial.println("Reiniciando em 10 segundos...");
    delay(10000);
    ESP.restart();
  }
}

void connectMQTT() {
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Conectando MQTT Broker (");
    Serial.print(MQTT_SERVER);
    Serial.print(")... ");

    if (client.connect(DEVICE_ID)) {
      Serial.println("OK!");
      Serial.println("Pronto para enviar dados!\n");
    } else {
      attempts++;
      Serial.print("FALHOU! Código: ");
      Serial.println(client.state());

      if (attempts < 5) {
        Serial.println("Tentando novamente em 5s...");
        delay(5000);
      } else {
        Serial.println("[ERRO] Não conseguiu conectar ao MQTT");
        Serial.println("Reiniciando...");
        delay(3000);
        ESP.restart();
      }
    }
  }
}

void sendSensorData() {
  float temperatura = random(150, 350) / 10.0;
  float umidade = random(300, 900) / 10.0;
  int luminosidade = random(0, 1024);

  unsigned long uptime = millis() / 1000;

  String json = "{";
  json += "\"device\":\"" + String(DEVICE_ID) + "\",";
  json += "\"uptime\":" + String(uptime) + ",";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade\":" + String(umidade, 1) + ",";
  json += "\"luminosidade\":" + String(luminosidade) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";

  String topic = "iot/sensor/dados";
  bool ok = client.publish(topic.c_str(), json.c_str());

  if (ok) {
    Serial.print("[ENVIADO] ");
    Serial.println(json);
  } else {
    Serial.print("[ERRO] Falha ao enviar! Estado MQTT: ");
    Serial.println(client.state());
  }
}