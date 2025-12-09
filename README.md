# Instalação Rápida no Raspberry Pi

## Pré-requisitos

- Raspberry Pi 3 ou superior com Raspberry Pi OS instalado
- Acesso SSH ao Raspberry Pi
- Conexão à internet (via Ethernet de preferência)

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

## Passo 6: Conectar ESP32

1. Abra o arquivo `examples/esp32_simple.ino` no Arduino IDE
2. **Apenas mude a linha:**
   ```cpp
   const char* DEVICE_ID = "esp32_sala_01";  // ← ID único para cada ESP
   ```
3. Faça upload no ESP32
4. Abra o Serial Monitor (115200 baud)
5. O ESP deve conectar no WiFi `RPi-IoT-Gateway` e enviar dados

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

## Troubleshooting

**ESP32 não conecta:**
- Verifique se o Raspberry Pi está ligado e o WiFi está ativo
- Teste conectar com seu celular no WiFi `RPi-IoT-Gateway`
- Verifique a senha no código do ESP

**Containers não iniciam:**
```bash
docker compose logs
sudo systemctl status docker
```

**Sem espaço em disco:**
```bash
docker system prune -a
```