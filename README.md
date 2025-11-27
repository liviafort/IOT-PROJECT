# Guia de Instalação - Raspberry Pi 3

Este guia detalha como configurar o projeto IoT Gateway em um Raspberry Pi 3.

---

## Requisitos de Hardware

- **Raspberry Pi 3 Model B** (ou superior)
- **Cartão microSD** de pelo menos 16GB (recomendado: SanDisk High Endurance 32GB)
- **Fonte de alimentação** 5V 2.5A
- **Cabo de rede Ethernet** (recomendado para estabilidade)
- Opcionalmente: **SSD USB** para armazenamento de dados (maior durabilidade)

---

## Parte 1: Preparação do Raspberry Pi

### 1.1 Instalar Raspberry Pi OS

1. Baixe o **Raspberry Pi Imager**: https://www.raspberrypi.com/software/

2. Insira o cartão microSD no computador

3. Abra o Raspberry Pi Imager e configure:
   ```
   Sistema Operacional: Raspberry Pi OS Lite (32-bit)
   Armazenamento: [Seu cartão SD]
   ```

4. **IMPORTANTE**: Clique na engrenagem ⚙️ (configurações avançadas) e configure:
   ```
   ✅ Ativar SSH
   ✅ Configurar nome de usuário e senha
      Usuário: pi
      Senha: [sua senha]

   ✅ Configurar WiFi (se não usar Ethernet)
      SSID: [nome da sua rede]
      Senha: [senha do WiFi]
      País: BR

   ✅ Configurar localização
      Fuso horário: America/Sao_Paulo
      Teclado: br
   ```

5. Clique em **Gravar** e aguarde

6. Insira o cartão SD no Raspberry Pi e ligue

---

### 1.2 Conectar ao Raspberry Pi

#### Via Ethernet (Recomendado)
```bash
# Descubra o IP do RPi no seu roteador, ou use:
ping raspberrypi.local

# Conecte via SSH:
ssh pi@raspberrypi.local
# ou
ssh pi@192.168.X.X
```

#### Via WiFi
Se configurou WiFi, conecte da mesma forma acima.

---

### 1.3 Atualizar o Sistema

```bash
# Atualizar pacotes
sudo apt update
sudo apt upgrade -y

# Reiniciar
sudo reboot
```

Aguarde 1-2 minutos e reconecte via SSH.

---

## Parte 2: Configurar IP Estático (IMPORTANTE!)

Um gateway IoT precisa de IP fixo para que os dispositivos ESP32 sempre encontrem o broker.

### 2.1 Descobrir Configuração Atual

```bash
# Ver IP atual
ip addr show

# Ver gateway padrão
ip route | grep default

# Ver DNS
cat /etc/resolv.conf
```

Anote:
- **IP atual**: Ex: `192.168.1.100`
- **Gateway (roteador)**: Ex: `192.168.1.1`
- **Máscara de rede**: Geralmente `/24` (255.255.255.0)

---

### 2.2 Configurar IP Estático

```bash
# Editar arquivo de configuração
sudo nano /etc/dhcpcd.conf
```

Adicione no **final do arquivo** (ajuste os valores para sua rede):

```bash
# IP Estático para Gateway IoT
interface eth0
static ip_address=192.168.1.100/24
static routers=192.168.1.1
static domain_name_servers=8.8.8.8 8.8.4.4

# Se usar WiFi ao invés de Ethernet, troque "eth0" por "wlan0":
# interface wlan0
# static ip_address=192.168.1.100/24
# static routers=192.168.1.1
# static domain_name_servers=8.8.8.8 8.8.4.4
```

Salve: `Ctrl+O`, Enter, `Ctrl+X`

```bash
# Reiniciar serviço de rede
sudo systemctl restart dhcpcd

# Verificar novo IP
ip addr show eth0
```

**Anote o IP configurado! Você vai precisar dele nos ESP32.**

---

## Parte 3: Instalar Docker e Docker Compose

### 3.1 Instalar Docker

```bash
# Baixar script oficial de instalação
curl -fsSL https://get.docker.com -o get-docker.sh

# Executar instalação
sudo sh get-docker.sh

# Adicionar usuário ao grupo docker (para rodar sem sudo)
sudo usermod -aG docker $USER

# Aplicar mudanças de grupo (ou faça logout/login)
newgrp docker

# Verificar instalação
docker --version
```

---

### 3.2 Instalar Docker Compose

```bash
# Instalar plugin do Docker Compose
sudo apt-get install docker-compose-plugin -y

# Verificar instalação
docker compose version
```

---

### 3.3 Habilitar Docker na Inicialização

```bash
sudo systemctl enable docker
```

---

## Parte 4: Transferir e Configurar o Projeto

### 4.1 Transferir Projeto do PC para RPi

**Opção A: Via SCP (do Windows)**
```powershell
# No PowerShell/CMD do Windows:
cd C:\Users\himer\Documents
scp -r IOT-PROJECT pi@192.168.1.100:/home/pi/

# Digite a senha quando solicitado
```

**Opção B: Via Git (se usar repositório)**
```bash
# No Raspberry Pi:
cd /home/pi
git clone [seu-repositorio-git] IOT-PROJECT
cd IOT-PROJECT
```

**Opção C: Via WinSCP / FileZilla**
- Use um cliente FTP/SFTP para copiar a pasta manualmente

---

### 4.2 Verificar Arquivos

```bash
# Conectar no RPi via SSH
ssh pi@192.168.1.100

# Navegar para o projeto
cd /home/pi/IOT-PROJECT

# Listar arquivos
ls -la

# Verificar se os arquivos principais existem:
ls -la Dockerfile mqtt_gateway.py docker-compose.yml docker-compose.rpi.yml
```

---

### 4.3 Criar Arquivo .env

```bash
# Criar arquivo de configuração
nano .env
```

Adicione o conteúdo:
```bash
MQTT_BROKER=0.0.0.0
MQTT_PORT=1883
DATA_DIR=data
```

Salve: `Ctrl+O`, Enter, `Ctrl+X`

---

## Parte 5: Build e Execução

### 5.1 Build da Imagem Docker (ARM)

```bash
# Certifique-se de estar no diretório do projeto
cd /home/pi/IOT-PROJECT

# Build da imagem (pode levar 5-10 minutos no RPi3)
docker compose -f docker-compose.rpi.yml build

# Verificar imagem criada
docker images
```

---

### 5.2 Iniciar os Serviços

```bash
# Subir containers em background
docker compose -f docker-compose.rpi.yml up -d

# Ver status dos containers
docker ps

# Ver logs em tempo real
docker compose -f docker-compose.rpi.yml logs -f

# Para sair dos logs: Ctrl+C (containers continuam rodando)
```

**Você deve ver 2 containers rodando:**
- `mqtt-broker` (Mosquitto)
- `mqtt-gateway` (Python)

---

### 5.3 Testar Conexão MQTT

**Do próprio Raspberry Pi:**
```bash
# Instalar cliente MQTT
sudo apt install mosquitto-clients -y

# Publicar mensagem de teste
mosquitto_pub -h localhost -t "test/hello" -m "Hello from RPi"

# Ver logs do gateway (deve aparecer a mensagem)
docker compose -f docker-compose.rpi.yml logs mqtt-gateway
```

**De outro computador na mesma rede:**
```bash
# No Windows (instale mosquitto clients primeiro)
mosquitto_pub -h 192.168.1.100 -t "test/hello" -m "Hello from PC"

# No Linux/Mac
mosquitto_pub -h 192.168.1.100 -t "test/hello" -m "Hello from external"
```

---

## Parte 6: Configurar ESP32 Devices

### 6.1 Atualizar Código do ESP32

Abra o arquivo `examples/esp32_simple.ino` e modifique:

```cpp
// CONFIGURAÇÃO DE REDE - AJUSTE AQUI!
const char* WIFI_SSID = "SUA_REDE_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI";

// CONFIGURAÇÃO DO MQTT BROKER - AJUSTE AQUI!
const char* MQTT_SERVER = "192.168.1.100";  // <-- IP DO SEU RASPBERRY PI
const int MQTT_PORT = 1883;

// IDENTIFICAÇÃO DO DISPOSITIVO - AJUSTE AQUI!
const char* DEVICE_ID = "esp32_sala_01";  // Único para cada ESP32
```

### 6.2 Upload no ESP32

1. Conecte o ESP32 no computador via USB
2. Abra o Arduino IDE
3. Selecione a porta correta
4. Faça o upload
5. Abra o Serial Monitor (115200 baud)
6. Verifique se conectou ao WiFi e ao MQTT

---

## Parte 7: Monitoramento e Manutenção

### 7.1 Comandos Úteis

```bash
# Ver logs em tempo real
docker compose -f docker-compose.rpi.yml logs -f mqtt-gateway

# Ver logs do broker Mosquitto
docker compose -f docker-compose.rpi.yml logs -f mosquitto

# Reiniciar serviços
docker compose -f docker-compose.rpi.yml restart

# Parar serviços
docker compose -f docker-compose.rpi.yml down

# Iniciar serviços
docker compose -f docker-compose.rpi.yml up -d

# Ver uso de recursos
docker stats

# Ver arquivos de dados gerados
ls -lh data/
```

---

### 7.2 Monitorar Temperatura do RPi

O Raspberry Pi pode aquecer. Monitore a temperatura:

```bash
# Ver temperatura atual
vcgencmd measure_temp

# Monitorar continuamente
watch -n 2 vcgencmd measure_temp
```

**Temperaturas normais:**
- Idle: 40-50°C
- Carga leve: 50-60°C
- Carga pesada: 60-70°C
- **⚠️ Crítico: >80°C** (considere adicionar dissipador/ventoinha)

---

### 7.3 Verificar Espaço em Disco

```bash
# Ver uso do SD card
df -h

# Ver tamanho da pasta de dados
du -sh /home/pi/IOT-PROJECT/data

# Ver tamanho dos arquivos de log por dia
ls -lh /home/pi/IOT-PROJECT/data/mqtt_log_*.json
```

---

### 7.4 Backup dos Dados

```bash
# Comprimir dados para backup
tar -czf backup_$(date +%Y%m%d).tar.gz data/

# Copiar backup para PC (do Windows)
scp pi@192.168.1.100:/home/pi/IOT-PROJECT/backup_*.tar.gz C:\Users\himer\Documents\Backups\
```

---

## Parte 8: Troubleshooting

### Problema: Container não inicia

```bash
# Ver logs detalhados
docker compose -f docker-compose.rpi.yml logs

# Verificar se portas estão em uso
sudo netstat -tulpn | grep 1883

# Rebuild forçado
docker compose -f docker-compose.rpi.yml build --no-cache
docker compose -f docker-compose.rpi.yml up -d
```

---

### Problema: ESP32 não conecta ao broker

1. **Verificar IP do broker no código ESP32**
   ```cpp
   const char* MQTT_SERVER = "192.168.1.100";  // IP correto?
   ```

2. **Testar conectividade de rede**
   ```bash
   # No RPi, verificar se porta 1883 está aberta
   sudo netstat -tulpn | grep 1883

   # Resultado esperado:
   # tcp 0.0.0.0:1883 ... LISTEN
   ```

3. **Verificar firewall (normalmente não é problema no RPi OS)**
   ```bash
   sudo iptables -L
   ```

4. **Ver logs do Mosquitto**
   ```bash
   docker compose -f docker-compose.rpi.yml logs mosquitto
   ```

---

### Problema: Arquivos JSON muito grandes

O código já tem limite de 10.000 mensagens por arquivo. Se ainda assim os arquivos crescerem muito:

```bash
# Limpar logs antigos (mais de 7 dias)
find /home/pi/IOT-PROJECT/data -name "mqtt_log_*.json" -mtime +7 -delete

# Ou criar cron job para limpeza automática
crontab -e

# Adicione a linha (executa todo dia às 3h da manhã):
0 3 * * * find /home/pi/IOT-PROJECT/data -name "mqtt_log_*.json" -mtime +7 -delete
```

---

### Problema: SD Card com pouco espaço

**Opção 1: Limpar logs do Docker**
```bash
docker system prune -a --volumes
```

**Opção 2: Mover pasta `data` para USB**
```bash
# Conectar pen drive/SSD USB
sudo mkdir -p /mnt/usb
sudo mount /dev/sda1 /mnt/usb

# Copiar dados existentes
sudo cp -r /home/pi/IOT-PROJECT/data/* /mnt/usb/data/

# Editar docker-compose.rpi.yml para apontar para /mnt/usb/data
```

---

## Parte 9: Inicialização Automática

O Docker já está configurado para iniciar automaticamente com `restart: unless-stopped`.

**Teste de inicialização automática:**
```bash
# Reiniciar o Raspberry Pi
sudo reboot

# Após 2-3 minutos, reconecte via SSH
ssh pi@192.168.1.100

# Verificar se containers estão rodando
docker ps

# Deve mostrar mqtt-broker e mqtt-gateway rodando
```

---

## Resumo Final

✅ **Checklist de Instalação Completa:**

- [ ] Raspberry Pi OS instalado e atualizado
- [ ] IP estático configurado
- [ ] Docker e Docker Compose instalados
- [ ] Projeto transferido para o RPi
- [ ] Arquivo `.env` criado
- [ ] Containers buildados e rodando
- [ ] Teste de conectividade MQTT OK
- [ ] ESP32 configurado com IP correto
- [ ] ESP32 conectando e enviando dados
- [ ] Arquivos JSON sendo criados em `data/`
- [ ] Sistema iniciando automaticamente após reboot

---

## Próximos Passos (Opcional)

1. **Adicionar autenticação MQTT** (usuário/senha)
2. **Implementar banco de dados** (SQLite ou InfluxDB)
3. **Criar API REST** para consultar dados
4. **Dashboard web** para visualização
5. **Alertas via Telegram/Email** para eventos críticos

---

## Suporte

Em caso de problemas:

1. Verifique os logs: `docker compose -f docker-compose.rpi.yml logs`
2. Verifique conectividade de rede: `ping 192.168.1.100`
3. Verifique espaço em disco: `df -h`
4. Verifique temperatura: `vcgencmd measure_temp`

---

**Documentação criada para o projeto IoT Gateway - Raspberry Pi 3**
*Última atualização: 2025-11-27*