FROM arm32v7/python:3.11-slim

# Definir diretório de trabalho
WORKDIR /app

# Copiar requirements
COPY requirements.txt .

# Instalar dependências
RUN pip install --no-cache-dir -r requirements.txt

# Copiar código da aplicação
COPY mqtt_gateway.py .
COPY monitor.py .

# Criar diretório de dados
RUN mkdir -p /app/data

# Expor porta MQTT (não necessária aqui, mas para documentação)
EXPOSE 1883

# Comando padrão
CMD ["python", "mqtt_gateway.py"]
