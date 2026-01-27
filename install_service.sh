#!/bin/bash
#
# Script de instalação do serviço NovaTherm DataLogger
# Nova Instruments
#
# Este script configura o serviço systemd para executar
# a aplicação automaticamente na inicialização do sistema
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
APP_NAME="app_armv6"
INSTALL_DIR="/opt/novatherm-datalogger"
SYSTEM_CONFIG_FILE="/boot/firmware/config.txt"

echo -e "${BLUE}=== Instalador do Serviço NovaTherm DataLogger ===${NC}"
echo -e "${BLUE}Nova Instruments${NC}\n"

# Verificar se está rodando como root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}❌ Este script precisa ser executado como root (use sudo)${NC}"
    exit 1
fi

# Verificar se o executável existe no diretório atual
if [ ! -f "./$APP_NAME" ]; then
    echo -e "${RED}❌ Executável '$APP_NAME' não encontrado no diretório atual${NC}"
    echo -e "${YELLOW}   Certifique-se de executar este script no mesmo diretório do executável${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Executável encontrado: $APP_NAME${NC}"

# Parar o serviço se já estiver rodando
if systemctl is-active --quiet $SERVICE_NAME; then
    echo -e "${YELLOW}⏸️  Parando serviço existente...${NC}"
    systemctl stop $SERVICE_NAME
fi

# Desabilitar o serviço se já estiver habilitado
if systemctl is-enabled --quiet $SERVICE_NAME 2>/dev/null; then
    echo -e "${YELLOW}🔓 Desabilitando serviço existente...${NC}"
    systemctl disable $SERVICE_NAME
fi

# Criar diretório de instalação
echo -e "${BLUE}📁 Criando diretório de instalação: $INSTALL_DIR${NC}"
mkdir -p $INSTALL_DIR

# Copiar executável
echo -e "${BLUE}📦 Copiando executável...${NC}"
cp -f ./$APP_NAME $INSTALL_DIR/$APP_NAME
chmod +x $INSTALL_DIR/$APP_NAME

# Verificar configuração do dispositivo em /boot/firmware/config.txt
echo -e "${BLUE}🔍 Verificando configuração do dispositivo...${NC}"
if [ -f "$SYSTEM_CONFIG_FILE" ]; then
    if grep -q "^DEVICE_NAME=" "$SYSTEM_CONFIG_FILE"; then
        DEVICE_NAME=$(grep "^DEVICE_NAME=" "$SYSTEM_CONFIG_FILE" | tail -n 1 | cut -d'=' -f2)
        echo -e "${GREEN}✅ Nome do dispositivo encontrado: $DEVICE_NAME${NC}"
    else
        echo -e "${YELLOW}⚠️  DEVICE_NAME não encontrado em $SYSTEM_CONFIG_FILE${NC}"
        echo -e "${YELLOW}   Adicione a linha 'DEVICE_NAME=NI00002' no final do arquivo${NC}"
        echo -e "${YELLOW}   Comando: sudo nano $SYSTEM_CONFIG_FILE${NC}"
    fi
else
    echo -e "${RED}❌ Arquivo $SYSTEM_CONFIG_FILE não encontrado${NC}"
fi

# Criar arquivo de serviço systemd
echo -e "${BLUE}⚙️  Criando arquivo de serviço systemd...${NC}"
cat > /etc/systemd/system/$SERVICE_NAME.service << EOF
[Unit]
Description=NovaTherm DataLogger Service
Documentation=https://github.com/novainstruments
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/$APP_NAME
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

# Configurações de segurança
NoNewPrivileges=false
PrivateTmp=true

# Limites de recursos
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
EOF

echo -e "${GREEN}✅ Arquivo de serviço criado: /etc/systemd/system/$SERVICE_NAME.service${NC}"

# Recarregar systemd
echo -e "${BLUE}🔄 Recarregando configuração do systemd...${NC}"
systemctl daemon-reload

# Habilitar serviço para iniciar no boot
echo -e "${BLUE}🔧 Habilitando serviço para iniciar automaticamente...${NC}"
systemctl enable $SERVICE_NAME

# Iniciar serviço
echo -e "${BLUE}🚀 Iniciando serviço...${NC}"
systemctl start $SERVICE_NAME

# Aguardar um momento para o serviço iniciar
sleep 2

# Verificar status
echo -e "\n${BLUE}📊 Status do serviço:${NC}"
systemctl status $SERVICE_NAME --no-pager -l

echo -e "\n${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Instalação concluída com sucesso!                      ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}\n"

echo -e "${BLUE}📋 Comandos úteis:${NC}"
echo -e "  ${YELLOW}Ver status:${NC}        sudo systemctl status $SERVICE_NAME"
echo -e "  ${YELLOW}Parar serviço:${NC}     sudo systemctl stop $SERVICE_NAME"
echo -e "  ${YELLOW}Iniciar serviço:${NC}   sudo systemctl start $SERVICE_NAME"
echo -e "  ${YELLOW}Reiniciar serviço:${NC} sudo systemctl restart $SERVICE_NAME"
echo -e "  ${YELLOW}Ver logs:${NC}          sudo journalctl -u $SERVICE_NAME -f"
echo -e "  ${YELLOW}Ver logs (últimas 50 linhas):${NC} sudo journalctl -u $SERVICE_NAME -n 50"
echo -e "  ${YELLOW}Desabilitar serviço:${NC} sudo systemctl disable $SERVICE_NAME"

echo -e "\n${BLUE}📁 Arquivos instalados em:${NC} $INSTALL_DIR"
echo -e "  ${YELLOW}Executável:${NC}     $INSTALL_DIR/$APP_NAME"

echo -e "\n${BLUE}💡 Configuração do Dispositivo:${NC}"
echo -e "  O nome do dispositivo é configurado em: ${YELLOW}$SYSTEM_CONFIG_FILE${NC}"
echo -e "  Adicione no final do arquivo: ${YELLOW}DEVICE_NAME=NI00003${NC}"
echo -e "  Comando para editar: ${YELLOW}sudo nano $SYSTEM_CONFIG_FILE${NC}"
echo -e "  Depois reinicie o serviço: ${YELLOW}sudo systemctl restart $SERVICE_NAME${NC}\n"

