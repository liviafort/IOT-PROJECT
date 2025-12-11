# Gateway IoT MQTT - Arquitetura em 3 Camadas

Sistema portátil de coleta de dados IoT usando Raspberry Pi como gateway MQTT entre dispositivos ESP32/ESP8266 e um backend em nuvem.

## Arquitetura do Sistema

O sistema é composto por três camadas que se comunicam via MQTT:

```
┌─────────────────────────────────────────────────────────────────┐
│                      CAMADA 3: BACKEND                          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Servidor Contabo (173.212.213.63)                       │   │
│  │  - API REST (FastAPI/Node.js)                            │   │
│  │  - Banco de dados (PostgreSQL/MongoDB)                   │   │
│  │  - Frontend web (React/Vue)                              │   │
│  │  - WebSocket para tempo real                             │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ▲                                   │
│                              │ MQTT Bridge                       │
│                              │ (publish para contabo)            │
└──────────────────────────────┼───────────────────────────────────┘
                               │
┌──────────────────────────────┼───────────────────────────────────┐
│                   CAMADA 2: GATEWAY (RASPBERRY PI)               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Raspberry Pi (192.168.50.1)                             │   │
│  │                                                           │   │
│  │  Componentes:                                            │   │
│  │  1. Access Point WiFi (RPi-IoT-Gateway)                  │   │
│  │  2. Broker MQTT local (Mosquitto - porta 1883)           │   │
│  │  3. Gateway Python (mqtt_gateway.py)                     │   │
│  │     - Recebe dados dos ESPs                              │   │
│  │     - Salva em arquivos JSON locais                      │   │
│  │     - Faz bridge para o backend (Contabo)                │   │
│  │  4. DHCP Server (dnsmasq)                                │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ▲                                   │
│                              │ WiFi (MQTT)                       │
│                              │ iot/sensor/dados                  │
└──────────────────────────────┼───────────────────────────────────┘
                               │
┌──────────────────────────────┼───────────────────────────────────┐
│                    CAMADA 1: DISPOSITIVOS IoT                    │
│  ┌────────────┐    ┌────────────┐    ┌────────────┐             │
│  │  ESP32 #1  │    │  ESP32 #2  │    │  ESP32 #N  │             │
│  │            │    │            │    │            │             │
│  │  Sensores: │    │  Sensores: │    │  Sensores: │             │
│  │  - DHT22   │    │  - BMP280  │    │  - LDR     │             │
│  │  - LDR     │    │  - PIR     │    │  - PIR     │             │
│  └────────────┘    └────────────┘    └────────────┘             │
│                                                                  │
│  Cada ESP:                                                       │
│  - Conecta no WiFi do Raspberry (RPi-IoT-Gateway)                │
│  - Recebe IP via DHCP (192.168.50.x)                            │
│  - Publica dados MQTT para 192.168.50.1:1883                    │
│  - Tópico: iot/sensor/dados                                     │
│  - Intervalo: 5 segundos                                        │
└──────────────────────────────────────────────────────────────────┘
```

## Fluxo de Comunicação

### 1. ESP para Gateway (MQTT local)

```
ESP32 → WiFi → Raspberry Pi (Broker MQTT)
```

**Exemplo de mensagem publicada:**
```json
{
  "device": "esp32_sala_01",
  "uptime": 120,
  "temperatura": 23.5,
  "umidade": 65.2,
  "luminosidade": 512,
  "rssi": -45
}
```

**Protocolo:**
- Transporte: WiFi (2.4GHz)
- Protocolo: MQTT v3.1.1
- Broker: 192.168.50.1:1883
- Tópico: `iot/sensor/dados`
- QoS: 0 (fire and forget)

### 2. Gateway para Backend (MQTT Bridge)

```
Raspberry Pi → Internet → Servidor Contabo
```

O gateway Python:
1. Subscreve no broker local (`iot/#`, `sensor/#`, `device/#`)
2. Recebe mensagens dos ESPs
3. Salva localmente em JSON (backup)
4. Republica para o broker remoto (Contabo)

**Código do Bridge:**
```python
# Quando mensagem chega do ESP
def on_message(client, userdata, msg):
    # Salvar localmente
    salvar_dados('device_id', msg.topic, payload)

    # Enviar para Contabo
    contabo_client.publish(CONTABO_TOPIC, msg.payload)
```

### 3. Backend para Frontend (WebSocket/REST)

```
Servidor Contabo → WebSocket/HTTP → Navegador
```

**APIs disponíveis:**
- `GET /api/devices` - Lista dispositivos cadastrados
- `GET /api/sensor-data?device=xxx` - Dados de um dispositivo
- `WebSocket /mqtt-ws/` - Stream de dados em tempo real

## Instalação

### Pré-requisitos

- Raspberry Pi 3+ com Raspberry Pi OS
- Acesso SSH
- Conexão Ethernet (para internet)

### 1. Configurar Gateway (Raspberry Pi)

```bash
# Clonar repositório
git clone https://github.com/SEU-USUARIO/IOT-PROJECT.git
cd IOT-PROJECT

# Configurar Access Point WiFi
sudo bash setup-access-point.sh

# Reiniciar
sudo reboot
```

### 2. Instalar Docker e Iniciar Serviços

```bash
# Instalar Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
newgrp docker

# Configurar variáveis
cat > .env << EOF
MQTT_BROKER=0.0.0.0
MQTT_PORT=1883
DATA_DIR=data
EOF

# Iniciar containers
docker compose up -d
```

### 3. Programar ESP32/ESP8266

**Bibliotecas necessárias (Arduino IDE):**
- PubSubClient

**Código (`examples/esp32_simple.ino`):**

```cpp
const char* DEVICE_ID = "esp32_sala_01";  // Mudar para cada ESP

const char* WIFI_SSID = "RPi-IoT-Gateway";
const char* WIFI_PASSWORD = "iotgateway2024";
const char* MQTT_SERVER = "192.168.50.1";
const int MQTT_PORT = 1883;
```

**Passos:**
1. Abrir arquivo no Arduino IDE
2. Alterar apenas o `DEVICE_ID`
3. Selecionar placa (ESP32 Dev Module ou NodeMCU)
4. Upload

## Configuração do Backend (Opcional)

Para enviar dados para um servidor remoto, edite `mqtt_gateway.py`:

```python
# Configurar IP do servidor
CONTABO_IP = "SEU_IP_AQUI"
CONTABO_PORT = 1883
CONTABO_TOPIC = "iot/sensor/dados"
```

Reinicie o container:
```bash
docker compose restart mqtt-gateway
```

## Dados Persistentes

O gateway salva todos os dados recebidos em arquivos JSON diários:

```bash
# Localização
~/IOT-PROJECT/data/mqtt_log_YYYY-MM-DD.json

# Estrutura
[
  {
    "timestamp": "2024-01-15T10:30:00",
    "client_id": "unknown",
    "topic": "iot/sensor/dados",
    "payload": {
      "device": "esp32_sala_01",
      "temperatura": 23.5,
      "umidade": 65.2,
      ...
    }
  }
]
```

## Portabilidade

O sistema funciona em qualquer rede sem reconfiguração:

**Rede Local Isolada:**
- Raspberry cria sua própria rede WiFi (192.168.50.0/24)
- ESPs sempre conectam em 192.168.50.1
- Código nunca muda

**Para trocar de ambiente:**
1. Desligar Raspberry
2. Levar para novo local
3. Conectar cabo Ethernet na nova rede
4. Ligar Raspberry
5. ESPs reconectam automaticamente

## Monitoramento

```bash
# Ver logs em tempo real
docker compose logs -f mqtt-gateway

# Listar dados salvos
ls -lh data/

# Testar MQTT
mosquitto_pub -h localhost -t "test" -m "hello"
mosquitto_sub -h localhost -t "#"
```

## Troubleshooting

**ESP não conecta:**
- Verificar se WiFi `RPi-IoT-Gateway` está ativo
- Testar conexão com celular primeiro
- Verificar serial monitor do ESP (115200 baud)

**Dados não chegam no backend:**
- Verificar IP do servidor Contabo em `mqtt_gateway.py`
- Verificar firewall/portas abertas no servidor remoto
- Verificar logs: `docker compose logs mqtt-gateway`

**Gateway offline:**
```bash
# Verificar containers
docker ps

# Reiniciar serviços
docker compose restart

# Verificar WiFi
sudo systemctl status hostapd
ip addr show wlan0
```

## Comandos Úteis

```bash
# Docker
docker compose up -d          # Iniciar
docker compose down           # Parar
docker compose logs -f        # Ver logs

# Serviços
sudo systemctl status hostapd    # WiFi AP
sudo systemctl status dnsmasq    # DHCP

# Rede
ip addr show wlan0            # IP do WiFi
ip addr show eth0             # IP da Ethernet
```

## Estrutura de Arquivos

```
IOT-PROJECT/
├── mqtt_gateway.py           # Gateway principal (Python)
├── monitor.py                # Monitor de dados MQTT
├── docker-compose.yml        # Orquestração containers
├── Dockerfile                # Imagem do gateway
├── setup-access-point.sh     # Script setup WiFi AP
├── examples/
│   └── esp32_simple.ino      # Código ESP32/ESP8266
├── backend/
│   ├── GATEWAY_BRIDGE_EXAMPLE.py
│   └── frontend/
│       └── app.js            # Frontend web
└── data/                     # Dados persistentes (JSON)
```

## Especificações Técnicas

**Gateway (Raspberry Pi):**
- WiFi: 2.4GHz (canal 7)
- IP fixo: 192.168.50.1/24
- DHCP range: 192.168.50.10-50
- MQTT Broker: Mosquitto 2.0+
- Container engine: Docker

**Dispositivos (ESP32/ESP8266):**
- WiFi: 802.11n (2.4GHz)
- IP: DHCP (192.168.50.x)
- MQTT Client: PubSubClient
- Intervalo envio: 5s

**Backend (Servidor remoto):**
- MQTT Broker: porta 1883
- API: REST + WebSocket
- Persistência: JSON/Database
