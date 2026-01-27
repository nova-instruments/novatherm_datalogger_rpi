#!/bin/bash

# Script para compilar libmodbus para ARMv6 (Raspberry Pi 1)
# Autor: Nova Instruments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps-armv6"
LIBMODBUS_DIR="$DEPS_DIR/libmodbus"

echo "=== Compilando libmodbus para ARMv6 (Raspberry Pi 1) ==="

# Criar diretório de dependências
mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

# Baixar libmodbus se não existir
if [ ! -d "libmodbus-3.1.11" ]; then
    echo "Baixando libmodbus 3.1.11..."
    wget -q https://github.com/stephane/libmodbus/releases/download/v3.1.11/libmodbus-3.1.11.tar.gz
    tar -xzf libmodbus-3.1.11.tar.gz
    rm libmodbus-3.1.11.tar.gz
fi

cd libmodbus-3.1.11

# Limpar build anterior
make clean 2>/dev/null || true

echo "Configurando libmodbus para ARMv6..."
./configure \
    --host=arm-linux-gnueabi \
    --prefix="$LIBMODBUS_DIR/install" \
    --enable-static \
    --enable-shared \
    --disable-tests \
    CC=arm-linux-gnueabi-gcc \
    CXX=arm-linux-gnueabi-g++ \
    CFLAGS="-march=armv6 -mfloat-abi=soft -O2" \
    CXXFLAGS="-march=armv6 -mfloat-abi=soft -O2"

echo "Compilando libmodbus..."
make -j$(nproc)

echo "Instalando libmodbus..."
make install

echo "✅ libmodbus compilada com sucesso para ARMv6!"
echo "   Biblioteca: $LIBMODBUS_DIR/install/lib/libmodbus.so"
echo "   Headers: $LIBMODBUS_DIR/install/include/modbus/"

# Verificar arquitetura
echo "Informações da biblioteca:"
file "$LIBMODBUS_DIR/install/lib/libmodbus.so"
