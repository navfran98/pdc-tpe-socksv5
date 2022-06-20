#!/bin/bash
echo -e "[ Tester ] - Corriendo saturador de proxy.\n"
max_curl=50
max_nc=50
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
echo -e "${BLUE}[ Tester ]${NC} - [ CURLS = $max_curl ] - [ NC = $max_nc ]\n"
echo -e "${BLUE}[ Tester ]${NC} - Ejecutando comandos NETCAT\n";
for i in $(seq 1 $max_nc)
do
    echo -e "${BLUE}[ Tester ]${NC} - Conecto al usuario numero $i"
    task $(nc -X 5 -x localhost:1080 www.google.com 80) &
done
echo -e "${BLUE}[ Tester ]${NC} - Ejecutando comandos CURLs\n";
for i in $(seq 1 $max_curl)
do
    status_code=$(curl --write-out %{http_code} --silent --output /dev/null -x socks5h://localhost:1080 www.google.com)
    if [[ "$status_code" -eq 200 ]] ; then
    echo -e "${BLUE}[ Tester ]${NC} - Corrida $i - STATUS -> ${GREEN}[ OK ]${NC}"
    else
    echo -e "${BLUE}[ Tester ]${NC} - Corrida $i - STATUS -> ${RED}[ ERROR ]${NC}"
    fi
done
