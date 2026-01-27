#!/bin/bash

# Script para compilar libmodbus para ARM (cross-compilation)
# Baseado no script da libgpiod que funcionou

set -e  # Parar em caso de erro

# Configurações
LIBMODBUS_VERSION="3.1.11"
LIBMODBUS_URL="https://github.com/stephane/libmodbus/releases/download/v${LIBMODBUS_VERSION}/libmodbus-${LIBMODBUS_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps"
LIBMODBUS_DIR="$DEPS_DIR/libmodbus"
BUILD_DIR="$LIBMODBUS_DIR/build"
INSTALL_DIR="$LIBMODBUS_DIR/install"

# Configuração do cross-compiler
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export AR=arm-linux-gnueabihf-ar
export STRIP=arm-linux-gnueabihf-strip
export PKG_CONFIG_PATH=""

echo "=== Compilando libmodbus $LIBMODBUS_VERSION para ARM32 ==="
echo "Diretório do projeto: $PROJECT_ROOT"
echo "Diretório de dependências: $DEPS_DIR"

# Verificar se o cross-compiler está disponível
if ! command -v $CC &> /dev/null; then
    echo "Erro: Cross-compiler ARM não encontrado!"
    echo "Instale com: sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"
    exit 1
fi

echo "Cross-compiler encontrado: $($CC --version | head -1)"

# Criar diretórios
mkdir -p "$DEPS_DIR"
mkdir -p "$LIBMODBUS_DIR"

# Entrar no diretório de trabalho
cd "$LIBMODBUS_DIR"

# Download do código fonte se não existir
if [ ! -f "libmodbus-${LIBMODBUS_VERSION}.tar.gz" ]; then
    echo "Baixando libmodbus $LIBMODBUS_VERSION..."
    wget "$LIBMODBUS_URL"
fi

# Extrair se não existir
if [ ! -d "libmodbus-${LIBMODBUS_VERSION}" ]; then
    echo "Extraindo libmodbus..."
    tar -xzf "libmodbus-${LIBMODBUS_VERSION}.tar.gz"
fi

# Entrar no diretório do código fonte
cd "libmodbus-${LIBMODBUS_VERSION}"

# Limpar compilação anterior se existir
if [ -f "Makefile" ]; then
    echo "Limpando compilação anterior..."
    make clean || true
fi

# Verificar se precisa executar autoreconf
if [ ! -f "configure" ]; then
    echo "Executando autoreconf..."
    autoreconf -fiv
fi

# Configurar para cross-compilation
echo "Configurando libmodbus para ARM32..."
./configure \
    --host=arm-linux-gnueabihf \
    --prefix="$INSTALL_DIR" \
    --enable-shared \
    --enable-static \
    --disable-tests \
    CFLAGS="-O2 -fPIC" \
    CXXFLAGS="-O2 -fPIC"

# Compilar
echo "Compilando libmodbus..."
make -j$(nproc)

# Instalar
echo "Instalando libmodbus em $INSTALL_DIR..."
make install

# Executar ranlib para garantir que o índice da biblioteca esteja correto
echo "Executando ranlib na biblioteca..."
$AR -s "$INSTALL_DIR/lib/libmodbus.a"

echo "=== libmodbus compilada com sucesso! ==="
echo "Biblioteca instalada em: $INSTALL_DIR"
echo "Headers: $INSTALL_DIR/include"
echo "Biblioteca: $INSTALL_DIR/lib/libmodbus.so"

# Verificar se os arquivos foram criados
if [ -f "$INSTALL_DIR/lib/libmodbus.so" ] && [ -f "$INSTALL_DIR/include/modbus/modbus.h" ]; then
    echo "✅ Compilação bem-sucedida!"
    ls -la "$INSTALL_DIR/lib/libmodbus.so"
    ls -la "$INSTALL_DIR/include/modbus/modbus.h"
else
    echo "❌ Erro: Arquivos esperados não foram encontrados!"
    exit 1
fi
