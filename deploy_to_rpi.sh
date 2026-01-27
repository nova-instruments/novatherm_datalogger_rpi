#!/bin/bash

# Script para transferir e executar modbus_reader na Raspberry Pi
# Uso: ./deploy_to_rpi.sh [IP_DA_RPI] [USUARIO]

RPI_IP=${1:-"192.168.3.22"}  # IP padrão da RPi
RPI_USER=${2:-"nova"}           # Usuário padrão
BINARY_PATH="build-rpi/bin/app"
REMOTE_PATH="/home/$RPI_USER/app"

echo "🚀 Deploying Modbus Reader to Raspberry Pi..."
echo "   Target: $RPI_USER@$RPI_IP"
echo "   Binary: $BINARY_PATH"

# Verificar se o binário existe
if [ ! -f "$BINARY_PATH" ]; then
    echo "❌ Erro: Binário não encontrado em $BINARY_PATH"
    echo "   Execute primeiro: make -C build-rpi -j"
    exit 1
fi

# Verificar se o binário é ARM
file_info=$(file "$BINARY_PATH")
if [[ ! "$file_info" == *"ARM"* ]]; then
    echo "❌ Erro: Binário não é ARM:"
    echo "   $file_info"
    exit 1
fi

echo "✅ Binário ARM encontrado ($(du -h $BINARY_PATH | cut -f1))"

# Transferir binário
echo "📤 Transferindo binário..."
scp "$BINARY_PATH" "$RPI_USER@$RPI_IP:$REMOTE_PATH"

if [ $? -eq 0 ]; then
    echo "✅ Transferência concluída!"
    
    # Tornar executável
    echo "🔧 Configurando permissões..."
    ssh "$RPI_USER@$RPI_IP" "chmod +x $REMOTE_PATH"
    
    echo ""
    echo "🎯 Para executar na Raspberry Pi:"
    echo "   ssh $RPI_USER@$RPI_IP"
    echo "   sudo $REMOTE_PATH"
    echo ""
    echo "📋 Configurações importantes na RPi:"
    echo "   • UART deve estar habilitado (/dev/serial0)"
    echo "   • Dispositivo Modbus conectado corretamente"
    echo "   • Configuração serial: 9600-N-8-1"
    echo ""
    echo "🔧 Comandos úteis na RPi:"
    echo "   • Verificar UART: ls -la /dev/serial*"
    echo "   • Verificar configuração: cat /boot/config.txt | grep uart"
    echo "   • Testar comunicação: sudo $REMOTE_PATH"
    echo "   • Verificar logs: dmesg | grep tty"
    
else
    echo "❌ Erro na transferência!"
    exit 1
fi
