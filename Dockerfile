# Dockerfile otimizado para Raspberry Pi 3 (ARM32)
FROM arm32v7/python:3.11-slim

# Metadados
LABEL maintainer="IoT Gateway"
LABEL description="MQTT Gateway para dispositivos IoT (ESP32/ESP8266)"

# Diretório de trabalho
WORKDIR /app

# Copiar e instalar dependências
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiar código da aplicação
COPY mqtt_gateway.py .

# Criar diretório de dados
RUN mkdir -p /app/data

# Comando padrão
CMD ["python", "-u", "mqtt_gateway.py"]
