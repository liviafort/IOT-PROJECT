#!/usr/bin/env python3
"""
EXEMPLO - Como adicionar MQTT Bridge no Gateway do Raspberry

Adicione este código ao mqtt_gateway.py existente para fazer bridge
dos dados locais para o broker MQTT na Contabo.
"""

import paho.mqtt.client as mqtt

# ============================================================================
# CONFIGURAÇÃO - Altere o IP da Contabo
# ============================================================================
CONTABO_IP = "SEU_IP_CONTABO_AQUI"  # Ex: "123.456.789.10"
CONTABO_PORT = 1883
CONTABO_TOPIC = "iot/sensor/dados"

# ============================================================================
# Cliente MQTT para Contabo (Bridge)
# ============================================================================

contabo_client = mqtt.Client(client_id="raspberry_bridge")

def conectar_contabo():
    try:
        contabo_client.connect(CONTABO_IP, CONTABO_PORT, 60)
        contabo_client.loop_start()
        print(f"✅ Conectado ao broker da Contabo: {CONTABO_IP}:{CONTABO_PORT}")
    except Exception as e:
        print(f"❌ Erro ao conectar na Contabo: {e}")

# Chamar no início do seu gateway
conectar_contabo()

# ============================================================================
# Modificar a função on_message do seu gateway atual
# ============================================================================

def on_message(client, userdata, msg):
    """
    Callback quando recebe mensagem MQTT dos ESPs (local).
    Adicione esta linha no final da sua função on_message existente:
    """
    try:
        # ... seu código existente de processamento ...

        # ADICIONAR: Enviar para Contabo via MQTT Bridge
        contabo_client.publish(CONTABO_TOPIC, msg.payload)
        print(f"📤 Enviado para Contabo: {msg.topic}")

    except Exception as e:
        print(f"Erro no bridge: {e}")


# ============================================================================
# RESUMO: O que fazer no Raspberry
# ============================================================================
"""
1. Abra: /home/livia/IOT-PROJECT/mqtt_gateway.py

2. Adicione no topo (após os imports):

   CONTABO_IP = "123.456.789.10"  # IP do servidor Contabo
   CONTABO_PORT = 1883
   CONTABO_TOPIC = "iot/sensor/dados"

   contabo_client = mqtt.Client(client_id="raspberry_bridge")
   contabo_client.connect(CONTABO_IP, CONTABO_PORT, 60)
   contabo_client.loop_start()

3. Na função on_message, adicione:

   contabo_client.publish(CONTABO_TOPIC, msg.payload)

4. Reinicie o container:

   docker compose restart mqtt-gateway

PRONTO! Gateway fará bridge automático:
ESP → MQTT Local (Rasp) → Gateway → MQTT Remoto (Contabo) → Backend
"""
