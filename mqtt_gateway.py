#!/usr/bin/env python3
"""
MQTT Gateway para IoT
Servidor MQTT para receber dados de dispositivos ESP32/ESP8266
"""

import paho.mqtt.client as mqtt
import json
import os
from datetime import datetime
from dotenv import load_dotenv

# Carregar variáveis de ambiente
load_dotenv()

# Configurações
MQTT_BROKER = os.getenv('MQTT_BROKER', '0.0.0.0')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))
DATA_DIR = os.getenv('DATA_DIR', 'data')

# Criar diretório de dados se não existir
os.makedirs(DATA_DIR, exist_ok=True)


def salvar_dados(client_id, topic, payload):
    """Salva os dados recebidos em arquivo JSON"""
    try:
        # Criar arquivo de log por dia
        date = datetime.now().strftime('%Y-%m-%d')
        log_file = os.path.join(DATA_DIR, f'mqtt_log_{date}.json')

        # Dados a serem salvos
        data = {
            'timestamp': datetime.now().isoformat(),
            'client_id': client_id,
            'topic': topic,
            'payload': payload
        }

        # Ler arquivo existente ou criar novo array
        logs = []
        if os.path.exists(log_file):
            with open(log_file, 'r', encoding='utf-8') as f:
                try:
                    logs = json.load(f)
                except json.JSONDecodeError:
                    logs = []

        # Adicionar novo log
        logs.append(data)

        # Limitar tamanho do array para evitar arquivos muito grandes (otimização para RPi)
        MAX_LOGS_PER_FILE = 10000
        if len(logs) > MAX_LOGS_PER_FILE:
            logs = logs[-MAX_LOGS_PER_FILE:]  # Mantém apenas últimas 10k mensagens

        # Salvar arquivo
        with open(log_file, 'w', encoding='utf-8') as f:
            json.dump(logs, f, indent=2, ensure_ascii=False)

        print(f"  [SALVO] Dados salvos em {log_file}")

    except Exception as e:
        print(f"  [ERRO] Falha ao salvar dados: {e}")


def on_connect(client, userdata, flags, rc):
    """Callback quando cliente conecta ao broker"""
    if rc == 0:
        print("\n========================================")
        print("   CONECTADO AO BROKER MQTT")
        print("========================================")

        # Inscrever em todos os tópicos IoT comuns
        topics = [
            'iot/#',           # Todos os tópicos IoT
            'sensor/#',        # Todos os sensores
            'device/#',        # Todos os dispositivos
            'esp32/#',         # ESP32 específico
            'esp8266/#',       # ESP8266 específico
        ]

        for topic in topics:
            client.subscribe(topic)
            print(f"  [SUBSCRIBE] Inscrito em: {topic}")

        print("========================================\n")
        print("Aguardando mensagens dos dispositivos...\n")
    else:
        print(f"[ERRO] Falha na conexão. Código: {rc}")


def on_message(client, userdata, msg):
    """Callback quando mensagem é recebida"""
    try:
        payload = msg.payload.decode('utf-8')

        print(f"\n[MENSAGEM RECEBIDA]")
        print(f"  Tópico: {msg.topic}")
        print(f"  Payload: {payload}")
        print(f"  QoS: {msg.qos}")
        print(f"  Timestamp: {datetime.now().isoformat()}")

        # Tentar decodificar como JSON
        try:
            payload_json = json.loads(payload)
            print(f"  Dados JSON: {json.dumps(payload_json, indent=4)}")
            salvar_dados('unknown', msg.topic, payload_json)
        except json.JSONDecodeError:
            # Se não for JSON, salvar como string
            salvar_dados('unknown', msg.topic, payload)

    except Exception as e:
        print(f"[ERRO] Falha ao processar mensagem: {e}")


def on_subscribe(client, userdata, mid, granted_qos):
    """Callback quando inscrição é confirmada"""
    print(f"  [OK] Inscrição confirmada (QoS: {granted_qos})")


def on_disconnect(client, userdata, rc):
    """Callback quando desconecta do broker"""
    if rc != 0:
        print(f"\n[AVISO] Desconexão inesperada. Código: {rc}")
        print("Tentando reconectar...")


def main():
    """Função principal"""
    print("========================================")
    print("   MQTT GATEWAY IOT - INICIANDO")
    print("========================================")
    print(f"Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Diretório de dados: {DATA_DIR}")
    print(f"Timestamp: {datetime.now().isoformat()}")
    print("========================================\n")

    # Criar cliente MQTT
    client = mqtt.Client(client_id="mqtt_gateway_iot")

    # Configurar callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_subscribe = on_subscribe
    client.on_disconnect = on_disconnect

    try:
        # Conectar ao broker
        print(f"Conectando ao broker {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)

        # Loop infinito para processar mensagens
        client.loop_forever()

    except KeyboardInterrupt:
        print("\n\n[SHUTDOWN] Encerrando servidor...")
        client.disconnect()
        print("[SHUTDOWN] Servidor encerrado com sucesso!")

    except Exception as e:
        print(f"\n[ERRO CRÍTICO] {e}")
        client.disconnect()


if __name__ == "__main__":
    main()
