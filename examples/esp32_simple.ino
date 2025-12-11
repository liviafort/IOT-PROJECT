/*
 * ESP32/ESP8266 - MQTT IoT Gateway - Raspberry Pi Edition
 *
 * Código PLUG AND PLAY - Funciona em qualquer rede!
 * Conecta automaticamente ao Raspberry Pi via WiFi e envia dados via MQTT
 *
 * BIBLIOTECAS NECESSÁRIAS (instalar via Arduino IDE):
 * - PubSubClient: https://github.com/knolleary/pubsubclient
 *
 * CONFIGURAÇÃO ÚNICA:
 * 1. Altere apenas o DEVICE_ID abaixo (uma vez por ESP)
 * 2. Compile e faça upload
 * 3. PRONTO! Vai funcionar automaticamente
 *
 * COMPATIBILIDADE:
 * - ESP32 (todas as versões)
 * - ESP8266 (NodeMCU, Wemos D1, etc)
 */

// Detectar plataforma automaticamente
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #error "Plataforma não suportada! Use ESP32 ou ESP8266"
#endif

#include <PubSubClient.h>

// ============================================================================
// CONFIGURAÇÃO - MUDE APENAS O DEVICE_ID!
// ============================================================================

// Identificação única deste dispositivo (OBRIGATÓRIO: mude para cada ESP!)
const char* DEVICE_ID = "esp32_sala_01";          // ← ÚNICA LINHA QUE VOCÊ PRECISA MUDAR!
                                                   // Exemplos: esp32_quarto_01, esp32_cozinha_01
                                                   //           esp8266_jardim_01, esp32_sensor_temp

// ============================================================================
// CONFIGURAÇÃO FIXA DO RASPBERRY PI - NÃO ALTERE!
// ============================================================================

// WiFi do Raspberry Pi (Access Point) - NUNCA MUDA!
const char* WIFI_SSID = "RPi-IoT-Gateway";
const char* WIFI_PASSWORD = "iotgateway2024";

// MQTT Broker (Raspberry Pi) - IP FIXO!
const char* MQTT_SERVER = "192.168.50.1";
const int MQTT_PORT = 1883;

// ============================================================================
// NÃO ALTERE DAQUI PARA BAIXO - CÓDIGO AUTOMÁTICO
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
  // Verificar conexão WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[AVISO] WiFi desconectado! Reconectando...");
    connectWiFi();
  }

  // Verificar conexão MQTT
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
  Serial.println("\n----------------------------------");
  Serial.print("Conectando WiFi: ");
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
    Serial.println("----------------------------------");
    Serial.print("IP atribuído: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway (Raspberry): ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Sinal WiFi: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("----------------------------------");
  } else {
    Serial.println(" FALHOU!");
    Serial.println("----------------------------------");
    Serial.println("[ERRO] Não conseguiu conectar ao Raspberry Pi");
    Serial.println("");
    Serial.println("Verifique:");
    Serial.println("1. Raspberry Pi está ligado?");
    Serial.println("2. WiFi 'RPi-IoT-Gateway' está ativo?");
    Serial.println("3. ESP está próximo do Raspberry?");
    Serial.println("");
    Serial.println("Reiniciando em 10 segundos...");
    Serial.println("----------------------------------");
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
      Serial.print("Device ID: ");
      Serial.println(DEVICE_ID);
      Serial.println("Pronto para enviar dados!");
      Serial.println("----------------------------------\n");
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
  // Simular leitura de sensores (substitua com sensores reais)
  // Para usar sensores reais, substitua estas linhas por leituras reais
  // Exemplo: float temperatura = dht.readTemperature();
  float temperatura = random(150, 350) / 10.0;  // 15.0 a 35.0°C
  float umidade = random(300, 900) / 10.0;      // 30.0 a 90.0%
  int luminosidade = random(0, 1024);           // 0 a 1023

  // Montar JSON com timestamp (uptime em segundos)
  unsigned long uptime = millis() / 1000;

  String json = "{";
  json += "\"device\":\"" + String(DEVICE_ID) + "\",";
  json += "\"uptime\":" + String(uptime) + ",";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade\":" + String(umidade, 1) + ",";
  json += "\"luminosidade\":" + String(luminosidade) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";

  // Publicar no tópico MQTT
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