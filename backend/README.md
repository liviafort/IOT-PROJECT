# IoT Backend - MQTT → PostgreSQL → WebSocket

Backend simples com MQTT Bridge. Recebe dados do Gateway do Raspberry via MQTT.

## 🏗️ Arquitetura

```
ESP → MQTT (Rasp) → Gateway Python (Bridge) → MQTT (Contabo) → Backend → PostgreSQL
                                                                      ↓
                                                                  WebSocket
```

## 🚀 Deploy na Contabo

### 1. Copiar pasta backend

```bash
scp -r backend/ root@SEU_IP_CONTABO:/root/iot-backend/
```

### 2. No servidor Contabo

```bash
cd /root/iot-backend

# Subir tudo
docker compose up -d --build

# Ver logs
docker compose logs -f backend
```

## 📡 Configurar Gateway no Raspberry

Edite `/home/livia/IOT-PROJECT/mqtt_gateway.py` e adicione:

```python
import paho.mqtt.client as mqtt

# No topo do arquivo, após imports:
CONTABO_IP = "SEU_IP_CONTABO"
contabo_client = mqtt.Client(client_id="raspberry_bridge")
contabo_client.connect(CONTABO_IP, 1883, 60)
contabo_client.loop_start()

# Na função on_message, adicione esta linha:
contabo_client.publish("iot/sensor/dados", msg.payload)
```

Reinicie o gateway:
```bash
docker compose restart mqtt-gateway
```

## 🌐 API Endpoints

### Health Check
```
GET /health
```

### Get All Data
```
GET /api/sensor-data?limit=100&device=esp32_sala_01
```

### Get Latest by Device
```
GET /api/sensor-data/latest/esp32_sala_01
```

### Get Devices List
```
GET /api/devices
```

## 📊 WebSocket (Tempo Real)

```javascript
import { io } from 'socket.io-client';

const socket = io('http://SEU_IP_CONTABO:3000');

socket.on('sensor-data', (data) => {
  console.log('Novo dado:', data);
});
```

## 🔧 Serviços

- **Mosquitto**: Porta 1883 (MQTT)
- **Backend**: Porta 3000 (HTTP + WebSocket)
- **PostgreSQL**: Porta 5432

## ✅ Testar se funciona

```bash
# 1. Ver se MQTT broker está rodando
docker compose logs mosquitto

# 2. Publicar teste manualmente (de qualquer lugar)
mosquitto_pub -h SEU_IP_CONTABO -t "iot/sensor/dados" \
  -m '{"device":"test","temperatura":25.5,"umidade":60}'

# 3. Ver se backend recebeu
docker compose logs backend

# 4. Ver no banco
docker compose exec postgres psql -U postgres -d iot_data \
  -c "SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 5;"
```

## 🛑 Parar

```bash
docker compose down
```

## 📝 Fluxo Completo

1. ESP publica MQTT → Raspberry (192.168.50.1:1883)
2. Gateway Python no Rasp recebe e faz bridge → Contabo (SEU_IP:1883)
3. Backend na Contabo escuta Mosquitto local
4. Backend salva PostgreSQL + emite WebSocket
5. Frontend recebe dados em tempo real

**Vantagens:**
- ✅ Raspberry = MQTT Broker (requisito da disciplina)
- ✅ Gateway = Bridge MQTT (arquitetura IoT correta)
- ✅ Não precisa porta aberta no Raspberry
- ✅ IP do Raspberry pode mudar (conexão é de saída)
