#!/bin/bash
set -e

echo "Iniciando IoT Gateway..."

docker compose -f docker-compose.rpi.yml build
docker compose -f docker-compose.rpi.yml up -d

echo ""
echo "Gateway iniciado!"
echo ""
docker ps
echo ""
echo "Ver logs: docker compose -f docker-compose.rpi.yml logs -f"
