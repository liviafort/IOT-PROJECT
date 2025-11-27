/*
 * ESP32 - MQTT IoT Simple
 *
 * Código simplificado que envia dados aleatórios de sensores
 * para o broker MQTT a cada 5 segundos
 *
 * Biblioteca necessária:
 * - PubSubClient: https://github.com/knolleary/pubsubclient
 *
 * CONFIGURAÇÃO PARA RASPBERRY PI:
 * ================================
 * 1. Após instalar o gateway no Raspberry Pi, descubra o IP:
 *    SSH no RPi: ssh pi@raspberrypi.local
 *    Execute: hostname -I
 *
 * 2. Substitua o IP abaixo (MQTT_SERVER) pelo IP do seu Raspberry Pi
 * 3. O ESP32 e o Raspberry Pi devem estar na MESMA REDE WiFi
 * 4. Cada ESP32 deve ter um DEVICE_ID único (ex: esp32_sala_01, esp32_cozinha_01)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================================
// CONFIGURAÇÃO - AJUSTE AQUI!
// ============================================================================

// 1. Configuração WiFi
const char* WIFI_SSID = "SEU_WIFI";           // Nome da sua rede WiFi
const char* WIFI_PASSWORD = "SUA_SENHA";      // Senha da sua rede WiFi

// 2. Configuração MQTT Broker (Raspberry Pi)
const char* MQTT_SERVER = "192.168.1.100";    // ← COLOQUE O IP DO SEU RASPBERRY PI AQUI!
const int MQTT_PORT = 1883;                   // Porta padrão MQTT (não mude)

// 3. Identificação do Dispositivo
const char* DEVICE_ID = "esp32_sala_01";      // ← ID único para este ESP32
                                               // Exemplos: esp32_sala_01, esp32_quarto_01, etc.

// ============================================================================
// NÃO ALTERE DAQUI PARA BAIXO (a menos que saiba o que está fazendo)
// ============================================================================

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSend = 0;
const long SEND_INTERVAL = 5000;  // Enviar a cada 5 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("    ESP32 IoT MQTT Sender");
  Serial.println("=================================");

  connectWiFi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(messageReceived);
}

void loop() {
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
  Serial.print("\nConectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ Falha ao conectar WiFi!");
    Serial.println("Verifique SSID e senha");
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("\nConectando ao MQTT broker...");

    if (client.connect(DEVICE_ID)) {
      Serial.println(" ✓ Conectado!");

      String statusMsg = "{\"device\":\"" + String(DEVICE_ID) + "\",\"status\":\"online\"}";
      client.publish("iot/device/status", statusMsg.c_str());

      client.subscribe("iot/commands/#");

    } else {
      Serial.print(" ✗ Falhou! Código: ");
      Serial.print(client.state());
      Serial.println("\nTentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void sendSensorData() {
  float temperatura = random(150, 350) / 10.0;  // 15.0 a 35.0°C
  float umidade = random(300, 900) / 10.0;      // 30.0 a 90.0%
  int luminosidade = random(0, 1024);           // 0 a 1023 

  String jsonMsg = "{";
  jsonMsg += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonMsg += "\"temperatura\":" + String(temperatura, 1) + ",";
  jsonMsg += "\"umidade\":" + String(umidade, 1) + ",";
  jsonMsg += "\"luminosidade\":" + String(luminosidade) + ",";
  jsonMsg += "\"timestamp\":" + String(millis());
  jsonMsg += "}";

  bool success = client.publish("iot/sensor/dados", jsonMsg.c_str());

  if (success) {
    Serial.println("\n[ENVIADO]");
    Serial.println("Tópico: iot/sensor/dados");
    Serial.println("Dados: " + jsonMsg);
  } else {
    Serial.println("\n[ERRO] Falha ao enviar!");
  }

  Serial.println("----------------------------");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println("°C");
  Serial.print("Umidade: "); Serial.print(umidade); Serial.println("%");
  Serial.print("Luminosidade: "); Serial.println(luminosidade);
  Serial.println("----------------------------");
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n[MENSAGEM RECEBIDA] Tópico: ");
  Serial.println(topic);
  Serial.print("Conteúdo: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}
