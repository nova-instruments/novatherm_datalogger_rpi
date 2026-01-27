#!/bin/bash
#
# Script de desinstalação do serviço NovaTherm DataLogger
# Nova Instruments
#

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configurações
SERVICE_NAME="novatherm-datalogger"
INSTALL_DIR="/opt/novatherm-datalogger"

echo -e "${BLUE}=== Desinstalador do Serviço NovaTherm DataLogger ===${NC}"
echo -e "${BLUE}Nova Instruments${NC}\n"

# Verificar se está rodando como root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}❌ Este script precisa ser executado como root (use sudo)${NC}"
    exit 1
fi

# Verificar se o serviço existe
if [ ! -f "/etc/systemd/system/$SERVICE_NAME.service" ]; then
    echo -e "${YELLOW}⚠️  Serviço '$SERVICE_NAME' não está instalado${NC}"
    exit 0
fi

# Parar o serviço se estiver rodando
if systemctl is-active --quiet $SERVICE_NAME; then
    echo -e "${YELLOW}⏸️  Parando serviço...${NC}"
    systemctl stop $SERVICE_NAME
    echo -e "${GREEN}✅ Serviço parado${NC}"
fi

# Desabilitar o serviço
if systemctl is-enabled --quiet $SERVICE_NAME 2>/dev/null; then
    echo -e "${YELLOW}🔓 Desabilitando serviço...${NC}"
    systemctl disable $SERVICE_NAME
    echo -e "${GREEN}✅ Serviço desabilitado${NC}"
fi

# Remover arquivo de serviço
echo -e "${BLUE}🗑️  Removendo arquivo de serviço...${NC}"
rm -f /etc/systemd/system/$SERVICE_NAME.service
echo -e "${GREEN}✅ Arquivo de serviço removido${NC}"

# Recarregar systemd
echo -e "${BLUE}🔄 Recarregando configuração do systemd...${NC}"
systemctl daemon-reload
systemctl reset-failed

# Perguntar se deseja remover os arquivos de instalação
echo -e "\n${YELLOW}Deseja remover os arquivos de instalação em $INSTALL_DIR? (s/N)${NC}"
read -r response
if [[ "$response" =~ ^([sS][iI][mM]|[sS])$ ]]; then
    echo -e "${BLUE}🗑️  Removendo diretório de instalação...${NC}"
    rm -rf $INSTALL_DIR
    echo -e "${GREEN}✅ Diretório removido${NC}"
else
    echo -e "${YELLOW}⚠️  Mantendo arquivos em $INSTALL_DIR${NC}"
fi

echo -e "\n${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Desinstalação concluída com sucesso!                   ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}\n"

echo -e "${BLUE}💡 Nota:${NC} A configuração do dispositivo permanece em:"
echo -e "  ${YELLOW}/boot/firmware/config.txt${NC}"
echo -e "  (linha DEVICE_NAME= não foi removida)\n"

