#!/bin/bash
set -e

echo "Instalando dependências..."

sudo apt update
sudo apt upgrade -y

if ! command -v docker &> /dev/null; then
    curl -fsSL https://get.docker.com -o get-docker.sh
    sudo sh get-docker.sh
    rm get-docker.sh
    sudo usermod -aG docker pi
fi

sudo apt-get install docker-compose-plugin -y
sudo systemctl enable docker

if [ ! -f .env ]; then
    cat > .env << 'EOF'
MQTT_BROKER=0.0.0.0
MQTT_PORT=1883
DATA_DIR=data
EOF
fi

mkdir -p data mosquitto/config mosquitto/data mosquitto/log

IP=$(hostname -I | awk '{print $1}')

echo ""
echo "Instalação concluída!"
echo "IP do Raspberry Pi: $IP"
echo ""
echo "Próximos passos:"
echo "1. exit"
echo "2. ssh pi@$IP"
echo "3. cd ~/IOT-PROJECT"
echo "4. ./iniciar.sh"
