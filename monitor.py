#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import json
from datetime import datetime
from dotenv import load_dotenv
import os

load_dotenv()

MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("   MONITOR MQTT - CONECTADO")
        print(f"Broker: {MQTT_BROKER}:{MQTT_PORT}")

        client.subscribe('#')
        print("[SUBSCRIBE] Monitorando todos os tópicos (#)\n")
        print("Aguardando mensagens...\n")
    else:
        print(f"[ERRO] Falha na conexão. Código: {rc}")


def on_message(client, userdata, msg):
    if msg.topic.startswith('$SYS/'):
        return

    try:
        payload = msg.payload.decode('utf-8')
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        print(f"\n[{timestamp}] Nova Mensagem")
        print(f"Tópico: {msg.topic}")
        print(f"QoS: {msg.qos}")
        print(f"Retain: {msg.retain}")

        try:
            payload_json = json.loads(payload)
            print(f"Payload (JSON):")
            print(json.dumps(payload_json, indent=2, ensure_ascii=False))
        except json.JSONDecodeError:
            print(f"Payload (String): {payload}")
        print()

    except Exception as e:
        print(f"[ERRO] Falha ao processar mensagem: {e}")


def main():
    print("\nMQTT MONITOR - IoT Gateway")
    print(f"Conectando ao broker {MQTT_BROKER}:{MQTT_PORT}...")
    print("Pressione Ctrl+C para sair\n")

    client = mqtt.Client(client_id="mqtt_monitor")
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_forever()

    except KeyboardInterrupt:
        print("\n\n[SHUTDOWN] Encerrando monitor...")
        client.disconnect()
        print("[SHUTDOWN] Monitor encerrado!\n")

    except Exception as e:
        print(f"\n[ERRO] {e}")
        client.disconnect()


if __name__ == "__main__":
    main()
