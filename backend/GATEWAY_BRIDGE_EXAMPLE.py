#!/usr/bin/env python3

import paho.mqtt.client as mqtt

CONTABO_IP = "SEU_IP_CONTABO_AQUI"
CONTABO_PORT = 1883
CONTABO_TOPIC = "iot/sensor/dados"

contabo_client = mqtt.Client(client_id="raspberry_bridge")

def conectar_contabo():
    try:
        contabo_client.connect(CONTABO_IP, CONTABO_PORT, 60)
        contabo_client.loop_start()
        print(f"Conectado ao broker da Contabo: {CONTABO_IP}:{CONTABO_PORT}")
    except Exception as e:
        print(f"Erro ao conectar na Contabo: {e}")

conectar_contabo()

def on_message(client, userdata, msg):
    try:
        contabo_client.publish(CONTABO_TOPIC, msg.payload)
        print(f"Enviado para Contabo: {msg.topic}")
    except Exception as e:
        print(f"Erro no bridge: {e}")
