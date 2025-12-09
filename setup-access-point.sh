#!/bin/bash
###############################################################################
# Script de Configuração do Raspberry Pi como Access Point WiFi
# Para uso com Gateway IoT MQTT
#
# Este script configura:
# - hostapd (Access Point)
# - dnsmasq (DHCP server)
# - IP estático na interface wlan0
# - Encaminhamento de pacotes (opcional)
#
# ATENÇÃO: Este script deve ser executado com sudo no Raspberry Pi
###############################################################################

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Função para imprimir mensagens
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[AVISO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERRO]${NC} $1"
}

# Verificar se está rodando como root
if [ "$EUID" -ne 0 ]; then
    print_error "Este script precisa ser executado como root"
    echo "Use: sudo bash setup-access-point.sh"
    exit 1
fi

# Verificar se está rodando em Raspberry Pi
if ! grep -q "Raspberry Pi" /proc/cpuinfo; then
    print_warning "Este script foi projetado para Raspberry Pi"
    read -p "Deseja continuar mesmo assim? (s/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        exit 1
    fi
fi

print_info "====================================================="
print_info "  Configuração do Raspberry Pi como Access Point"
print_info "====================================================="
echo ""

# Configurações padrão (podem ser alteradas)
AP_SSID="RPi-IoT-Gateway"
AP_PASSWORD="iotgateway2024"
AP_CHANNEL="7"
AP_IP="192.168.50.1"
AP_NETMASK="255.255.255.0"
AP_DHCP_RANGE="192.168.50.10,192.168.50.50"

# Perguntar configurações ao usuário
print_info "Configurações do Access Point WiFi:"
echo ""
read -p "Nome da rede WiFi (SSID) [${AP_SSID}]: " input_ssid
AP_SSID=${input_ssid:-$AP_SSID}

read -p "Senha WiFi (mínimo 8 caracteres) [${AP_PASSWORD}]: " input_password
AP_PASSWORD=${input_password:-$AP_PASSWORD}

# Validar senha
if [ ${#AP_PASSWORD} -lt 8 ]; then
    print_error "Senha precisa ter no mínimo 8 caracteres!"
    exit 1
fi

read -p "Canal WiFi (1-11) [${AP_CHANNEL}]: " input_channel
AP_CHANNEL=${input_channel:-$AP_CHANNEL}

read -p "IP do Raspberry Pi [${AP_IP}]: " input_ip
AP_IP=${input_ip:-$AP_IP}

echo ""
print_info "Configurações selecionadas:"
echo "  SSID: ${AP_SSID}"
echo "  Senha: ${AP_PASSWORD}"
echo "  Canal: ${AP_CHANNEL}"
echo "  IP do RPi: ${AP_IP}"
echo "  Range DHCP: ${AP_DHCP_RANGE}"
echo ""
read -p "Confirma as configurações? (S/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Nn]$ ]]; then
    print_info "Configuração cancelada"
    exit 0
fi

print_info "Iniciando instalação..."
echo ""

# 1. Atualizar sistema
print_info "[1/8] Atualizando sistema..."
apt update

# 2. Instalar pacotes necessários
print_info "[2/8] Instalando hostapd e dnsmasq..."
apt install -y hostapd dnsmasq iptables

# Parar serviços antes de configurar
systemctl stop hostapd 2>/dev/null || true
systemctl stop dnsmasq 2>/dev/null || true

# 3. Configurar IP estático na interface wlan0
print_info "[3/8] Configurando IP estático em wlan0..."

# Backup do dhcpcd.conf
cp /etc/dhcpcd.conf /etc/dhcpcd.conf.backup-$(date +%Y%m%d-%H%M%S)

# Remover configurações antigas de wlan0 se existirem
sed -i '/^interface wlan0/,/^$/d' /etc/dhcpcd.conf

# Adicionar configuração de IP estático
cat >> /etc/dhcpcd.conf << EOF

# Configuração Access Point - Adicionado por setup-access-point.sh
interface wlan0
    static ip_address=${AP_IP}/24
    nohook wpa_supplicant
EOF

# 4. Configurar hostapd (Access Point)
print_info "[4/8] Configurando hostapd..."

cat > /etc/hostapd/hostapd.conf << EOF
# Configuração do Access Point - Gerado por setup-access-point.sh
interface=wlan0
driver=nl80211

# Configurações da rede WiFi
ssid=${AP_SSID}
channel=${AP_CHANNEL}
hw_mode=g
ieee80211n=1
wmm_enabled=1
ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]

# Configurações de segurança
auth_algs=1
wpa=2
wpa_passphrase=${AP_PASSWORD}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP CCMP
rsn_pairwise=CCMP

# País (Brasil)
country_code=BR
EOF

# Configurar daemon do hostapd
cat > /etc/default/hostapd << EOF
# Configuração do daemon hostapd
DAEMON_CONF="/etc/hostapd/hostapd.conf"
EOF

# 5. Configurar dnsmasq (DHCP server)
print_info "[5/8] Configurando dnsmasq..."

# Backup da configuração original
mv /etc/dnsmasq.conf /etc/dnsmasq.conf.backup-$(date +%Y%m%d-%H%M%S) 2>/dev/null || true

cat > /etc/dnsmasq.conf << EOF
# Configuração DHCP - Gerado por setup-access-point.sh

# Interface para escutar
interface=wlan0

# Não usar como DNS para wlan0
no-resolv
no-poll

# Range de IPs DHCP
dhcp-range=${AP_DHCP_RANGE},12h

# Gateway padrão
dhcp-option=3,${AP_IP}

# DNS servers (Google DNS)
dhcp-option=6,8.8.8.8,8.8.4.4

# Domínio local
domain=local

# Logs
log-queries
log-dhcp
EOF

# 6. Configurar encaminhamento de pacotes (opcional, para compartilhar internet)
print_info "[6/8] Configurando encaminhamento de pacotes..."

# Descomentar ip_forward no sysctl.conf
sed -i 's/#net.ipv4.ip_forward=1/net.ipv4.ip_forward=1/' /etc/sysctl.conf

# Aplicar imediatamente
echo 1 > /proc/sys/net/ipv4/ip_forward

# 7. Configurar iptables (NAT) para compartilhar internet via eth0
print_info "[7/8] Configurando firewall (iptables)..."

# Limpar regras antigas
iptables -F
iptables -t nat -F

# Configurar NAT (se eth0 tiver internet, compartilha com wlan0)
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT

# Salvar regras iptables
apt install -y iptables-persistent
netfilter-persistent save

# 8. Habilitar e iniciar serviços
print_info "[8/8] Habilitando serviços..."

systemctl unmask hostapd
systemctl enable hostapd
systemctl enable dnsmasq

print_info "Configuração concluída!"
echo ""
print_info "====================================================="
print_info "  INFORMAÇÕES IMPORTANTES"
print_info "====================================================="
echo ""
echo "  Nome da rede WiFi: ${AP_SSID}"
echo "  Senha WiFi: ${AP_PASSWORD}"
echo "  IP do Raspberry Pi: ${AP_IP}"
echo "  Porta MQTT: 1883"
echo ""
echo "  Para dispositivos ESP32, use:"
echo "    const char* WIFI_SSID = \"${AP_SSID}\";"
echo "    const char* WIFI_PASSWORD = \"${AP_PASSWORD}\";"
echo "    const char* MQTT_SERVER = \"${AP_IP}\";"
echo ""
print_info "====================================================="
echo ""
print_warning "É necessário REINICIAR o Raspberry Pi para aplicar as mudanças"
echo ""
read -p "Deseja reiniciar agora? (S/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Nn]$ ]]; then
    print_info "Reiniciando em 5 segundos..."
    sleep 5
    reboot
else
    print_warning "Não esqueça de reiniciar manualmente: sudo reboot"
fi