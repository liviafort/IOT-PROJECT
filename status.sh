#!/bin/bash
echo "IP do Raspberry Pi:"
hostname -I
echo ""
echo "Containers:"
docker ps
echo ""
echo "Espaço em disco:"
df -h /
echo ""
echo "Temperatura:"
vcgencmd measure_temp