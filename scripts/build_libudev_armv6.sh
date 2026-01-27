#!/bin/bash

# Script para compilar libudev (eudev) para ARMv6 (Raspberry Pi 1)
# Autor: Nova Instruments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps-armv6"
LIBUDEV_DIR="$DEPS_DIR/eudev"

echo "=== Compilando libudev (eudev) para ARMv6 (Raspberry Pi 1) ==="

# Verificar se gperf está instalado
if ! command -v gperf &> /dev/null; then
    echo "❌ gperf não encontrado. Instale com: sudo apt-get install gperf"
    exit 1
fi

# Criar diretório de dependências
mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

# Baixar eudev se não existir
if [ ! -d "eudev-3.2.11" ]; then
    echo "Baixando eudev 3.2.11..."
    wget -q https://github.com/eudev-project/eudev/releases/download/v3.2.11/eudev-3.2.11.tar.gz
    tar -xzf eudev-3.2.11.tar.gz
    rm eudev-3.2.11.tar.gz
fi

cd eudev-3.2.11

# Limpar build anterior
make clean 2>/dev/null || true

echo "Configurando eudev para ARMv6..."
./configure \
    --host=arm-linux-gnueabi \
    --prefix="$LIBUDEV_DIR/install" \
    --disable-programs \
    --disable-hwdb \
    --disable-selinux \
    --enable-static \
    --enable-shared \
    CC=arm-linux-gnueabi-gcc \
    CXX=arm-linux-gnueabi-g++ \
    CFLAGS="-march=armv6 -mfloat-abi=soft -O2" \
    CXXFLAGS="-march=armv6 -mfloat-abi=soft -O2"

echo "Compilando eudev..."
make -j$(nproc)

echo "Instalando eudev..."
make install

echo "✅ libudev (eudev) compilada com sucesso para ARMv6!"
echo "   Biblioteca: $LIBUDEV_DIR/install/lib/libudev.a"
echo "   Headers: $LIBUDEV_DIR/install/include/libudev.h"

# Verificar arquitetura
echo "Informações da biblioteca:"
file "$LIBUDEV_DIR/install/lib/libudev.a"
