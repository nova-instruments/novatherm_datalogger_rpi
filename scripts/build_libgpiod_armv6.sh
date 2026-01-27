#!/bin/bash

# Script para compilar libgpiod para ARMv6 (Raspberry Pi 1)
# Autor: Nova Instruments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps-armv6"
LIBGPIOD_DIR="$DEPS_DIR/libgpiod"

echo "=== Compilando libgpiod para ARMv6 (Raspberry Pi 1) ==="

# Criar diretório de dependências
mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

# Baixar libgpiod se não existir
if [ ! -d "libgpiod-1.6.3" ]; then
    echo "Baixando libgpiod 1.6.3..."
    wget -q https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/snapshot/libgpiod-1.6.3.tar.gz
    tar -xzf libgpiod-1.6.3.tar.gz
    rm libgpiod-1.6.3.tar.gz
fi

cd libgpiod-1.6.3

# Limpar build anterior
make clean 2>/dev/null || true

# Gerar configure se não existir
if [ ! -f configure ]; then
    echo "Gerando configure..."
    ./autogen.sh --enable-tools=no --enable-bindings-cxx=no --enable-bindings-python=no
fi

echo "Configurando libgpiod para ARMv6..."
./configure \
    --host=arm-linux-gnueabi \
    --prefix="$LIBGPIOD_DIR/install" \
    --enable-tools=no \
    --enable-bindings-cxx=no \
    --enable-bindings-python=no \
    --disable-tests \
    CC=arm-linux-gnueabi-gcc \
    CXX=arm-linux-gnueabi-g++ \
    CFLAGS="-march=armv6 -mfloat-abi=soft -O2" \
    CXXFLAGS="-march=armv6 -mfloat-abi=soft -O2"

echo "Compilando libgpiod..."
make -j$(nproc)

echo "Instalando libgpiod..."
make install

echo "✅ libgpiod compilada com sucesso para ARMv6!"
echo "   Biblioteca: $LIBGPIOD_DIR/install/lib/libgpiod.so"
echo "   Headers: $LIBGPIOD_DIR/install/include/gpiod.h"

# Verificar arquitetura
echo "Informações da biblioteca:"
file "$LIBGPIOD_DIR/install/lib/libgpiod.so"
