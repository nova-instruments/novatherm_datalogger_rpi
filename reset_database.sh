#!/bin/bash

# Script para resetar banco de dados do NovaTherm DataLogger
# Remove todos os arquivos de log e banco de dados

echo "🗑️  Resetando banco de dados do NovaTherm DataLogger..."
echo ""

# Parar serviço se estiver rodando
if systemctl is-active --quiet novatherm_datalogger.service; then
    echo "⏸️  Parando serviço novatherm_datalogger..."
    sudo systemctl stop novatherm_datalogger.service
    sleep 2
fi

# Apagar arquivos de banco de dados SQLite
echo "🗄️  Removendo bancos de dados SQLite..."
rm -fv /home/nova/datalogger_*.db
rm -fv /home/nova/*.db

# Apagar arquivos de log TXT
echo "📄 Removendo arquivos de log TXT..."
rm -fv /home/nova/datalogger_*.txt
rm -fv /home/nova/*.txt

echo ""
echo "✅ Reset concluído!"
echo ""
echo "📋 Próximos passos:"
echo "   1. Execute o programa: sudo ./app_armv7"
echo "   2. O banco será recriado automaticamente com 2 canais (CH1 e CH2)"
echo ""

