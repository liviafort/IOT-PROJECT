# Gateway IoT MQTT - Raspberry Pi + ESP32/ESP8266

Sistema **PLUG AND PLAY** para criar uma rede WiFi IoT portátil usando Raspberry Pi como gateway MQTT.

## ✨ Características

- 🔌 **100% Portátil**: Funciona em qualquer rede (casa, universidade, etc)
- 📡 **WiFi Access Point**: Raspberry cria rede WiFi própria para os ESPs
- 🔄 **Automático**: Código ESP inalterável - só muda o DEVICE_ID uma vez
- 🌐 **Compartilhamento de Internet**: ESPs têm internet através do Raspberry
- 💾 **Dados Persistentes**: Salva logs MQTT em JSON no Raspberry

## Pré-requisitos

- Raspberry Pi 3 ou superior com Raspberry Pi OS instalado
- Acesso SSH ao Raspberry Pi
- Conexão Ethernet à internet (cabo de rede)

---

## Passo 1: Clonar o Repositório

No Raspberry Pi, execute:

```bash
cd ~
git clone https://github.com/SEU-USUARIO/IOT-PROJECT.git
cd IOT-PROJECT
```

**OU via SCP do seu PC:**

```bash
# No seu PC Windows (PowerShell):
scp -r C:\Users\himer\Documents\IOT-PROJECT pi@IP_DO_RPI:/home/pi/
```

---

## Passo 2: Configurar Access Point WiFi

```bash
cd ~/IOT-PROJECT
sudo bash setup-access-point.sh
```

**Durante a instalação, o script vai perguntar:**
- Nome da rede WiFi (deixe padrão: `RPi-IoT-Gateway` ou escolha outro)
- Senha WiFi (deixe padrão: `iotgateway2024` ou escolha outra)
- Canal WiFi (deixe padrão: `7`)
- IP do RPi (deixe padrão: `192.168.50.1`)

**Ao final, reinicie:**
```bash
sudo reboot
```

⏳ Aguarde 2-3 minutos para o RPi reiniciar.

---

## Passo 3: Instalar Docker

Reconecte via SSH e instale o Docker:

```bash
# Baixar script de instalação
curl -fsSL https://get.docker.com -o get-docker.sh

# Instalar
sudo sh get-docker.sh

# Adicionar usuário ao grupo docker
sudo usermod -aG docker $USER

# Aplicar mudanças
newgrp docker

# Instalar Docker Compose
sudo apt-get install docker-compose-plugin -y

# Habilitar Docker na inicialização
sudo systemctl enable docker
```

---

## Passo 4: Configurar e Iniciar Servidor MQTT

```bash
cd ~/IOT-PROJECT

# Criar arquivo de configuração
cat > .env << EOF
MQTT_BROKER=0.0.0.0
MQTT_PORT=1883
DATA_DIR=data
EOF

# Build e iniciar containers
docker compose build
docker compose up -d

# Verificar se está rodando
docker ps
```

Você deve ver 2 containers rodando:
- `mqtt-broker` (Mosquitto)
- `mqtt-gateway` (Python)

---

## Passo 5: Testar Conexão

```bash
# Instalar cliente MQTT
sudo apt install mosquitto-clients -y

# Publicar mensagem de teste
mosquitto_pub -h localhost -t "test/hello" -m "Hello from RPi"

# Ver logs do gateway (deve aparecer a mensagem)
docker compose logs mqtt-gateway
```

---

## Passo 6: Programar ESP32/ESP8266

### 📋 Bibliotecas Necessárias (Arduino IDE)

Instale via **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas**:
- `PubSubClient` (by Nick O'Leary)

### 🚀 Upload do Código

1. Abra `examples/esp32_simple.ino` no Arduino IDE
2. **Mude APENAS esta linha** (ID único para cada ESP):
   ```cpp
   const char* DEVICE_ID = "esp32_sala_01";  // ← Exemplos: esp32_quarto_01, esp8266_jardim_01
   ```
3. Selecione a placa correta:
   - **ESP32**: `Tools → Board → ESP32 → ESP32 Dev Module`
   - **ESP8266**: `Tools → Board → ESP8266 → NodeMCU 1.0`
4. Selecione a porta COM do ESP
5. Clique em **Upload** (seta →)
6. Abra o **Serial Monitor** (115200 baud)

### ✅ O ESP vai automaticamente:
- Conectar no WiFi `RPi-IoT-Gateway`
- Obter um IP via DHCP (ex: 192.168.50.10)
- Conectar no MQTT Broker (192.168.50.1:1883)
- Enviar dados a cada 5 segundos

### 📊 Exemplo de saída no Serial Monitor:
```
================================
ESP32 IoT - MQTT Sensor
================================
Device ID: esp32_sala_01
================================

----------------------------------
Conectando WiFi: RPi-IoT-Gateway
Procurando Raspberry Pi........... OK!
----------------------------------
IP atribuído: 192.168.50.12
Gateway (Raspberry): 192.168.50.1
Sinal WiFi: -45 dBm
----------------------------------
Conectando MQTT Broker (192.168.50.1)... OK!
Device ID: esp32_sala_01
Pronto para enviar dados!
----------------------------------

[ENVIADO] {"device":"esp32_sala_01","uptime":15,"temperatura":23.4,"umidade":65.2,"luminosidade":512,"rssi":-45}
[ENVIADO] {"device":"esp32_sala_01","uptime":20,"temperatura":23.8,"umidade":64.1,"luminosidade":520,"rssi":-44}
```

---

## Verificar Dados Recebidos

```bash
# Ver logs em tempo real
docker compose logs -f mqtt-gateway

# Ver arquivos JSON gerados
ls -lh data/

# Ver conteúdo de um arquivo de dados
cat data/mqtt_log_$(date +%Y-%m-%d).json | python3 -m json.tool
```

---

## Comandos Úteis

```bash
# Ver status dos containers
docker ps

# Ver logs
docker compose logs -f

# Reiniciar serviços
docker compose restart

# Parar tudo
docker compose down

# Iniciar tudo
docker compose up -d

# Ver temperatura do RPi
vcgencmd measure_temp

# Ver uso de disco
df -h
```

---

## 🎉 Pronto!

Seu Gateway IoT está funcionando! Os ESPs vão se conectar automaticamente no WiFi do Raspberry Pi e enviar dados via MQTT.

**Configuração final:**
- **WiFi:** `RPi-IoT-Gateway` (ou o que você escolheu)
- **IP do RPi:** `192.168.50.1`
- **Porta MQTT:** `1883`
- **Dados salvos em:** `~/IOT-PROJECT/data/`

---

## 🌍 Portabilidade - Funciona em Qualquer Rede!

### Como funciona:

```
Casa:          Internet → Roteador Casa → [eth0] Raspberry Pi [wlan0] → ESPs
Universidade:  Internet → Roteador Univ → [eth0] Raspberry Pi [wlan0] → ESPs
Qualquer:      Internet → Roteador      → [eth0] Raspberry Pi [wlan0] → ESPs
```

### ✅ Para trocar de rede:

1. **Desligue** o Raspberry Pi
2. **Desconecte** o cabo Ethernet
3. **Leve** para outro local (casa → universidade)
4. **Conecte** o cabo Ethernet na nova rede
5. **Ligue** o Raspberry Pi
6. **Aguarde** 30-60 segundos (boot)
7. **Pronto!** ESPs continuam funcionando normalmente

### 🔧 O que acontece automaticamente:

- ✅ `eth0` pega IP da nova rede via DHCP
- ✅ `wlan0` mantém IP fixo `192.168.50.1`
- ✅ WiFi `RPi-IoT-Gateway` continua ativo
- ✅ ESPs reconectam automaticamente
- ✅ MQTT continua funcionando
- ✅ Dados continuam sendo salvos

### 📱 Código ESP **NUNCA MUDA!**

```cpp
// Essas configurações NUNCA mudam, em qualquer rede:
const char* WIFI_SSID = "RPi-IoT-Gateway";
const char* WIFI_PASSWORD = "iotgateway2024";
const char* MQTT_SERVER = "192.168.50.1";
```

---

## Troubleshooting

### ESP32 não conecta no WiFi:

1. Verifique se o Raspberry Pi está ligado
2. Teste conectar com seu **celular** no WiFi `RPi-IoT-Gateway` (senha: `iotgateway2024`)
3. Se o celular conectar mas ESP não:
   ```cpp
   // Verifique se o DEVICE_ID está correto e sem caracteres especiais
   const char* DEVICE_ID = "esp32_sala_01";  // OK
   const char* DEVICE_ID = "esp32 sala 01";  // ERRO (espaços)
   ```

### WiFi do Raspberry não aparece:

```bash
# Verificar se hostapd está rodando
sudo systemctl status hostapd

# Verificar se wlan0 tem IP correto
ip addr show wlan0
# Deve mostrar: inet 192.168.50.1/24

# Reiniciar serviços
sudo systemctl restart wlan0-setup hostapd dnsmasq

# Ver logs de erro
sudo journalctl -u hostapd -n 50
```

### ESP conecta no WiFi mas não envia dados MQTT:

```bash
# Verificar se Mosquitto está rodando
docker ps | grep mqtt-broker

# Ver logs do Mosquitto
docker compose logs mqtt-broker

# Testar MQTT manualmente
mosquitto_pub -h localhost -t "test" -m "hello"
```

### Containers não iniciam:

```bash
# Ver logs
docker compose logs

# Verificar status do Docker
sudo systemctl status docker

# Reiniciar Docker
sudo systemctl restart docker
docker compose up -d
```

### Sem espaço em disco:

```bash
# Limpar containers e imagens antigas
docker system prune -a

# Ver uso de disco
df -h

# Limpar logs antigos
sudo journalctl --vacuum-time=7d
```

### Após trocar de rede, nada funciona:

```bash
# Verificar se eth0 pegou IP
ip addr show eth0

# Testar internet
ping -c 3 8.8.8.8

# Verificar serviços
sudo systemctl status hostapd dnsmasq wlan0-setup

# Se necessário, reiniciar tudo
sudo reboot
```