#!/bin/bash

# Script para compilar libgpiod para ARM (cross-compilation)
# Baseado no script da libmodbus

set -e  # Parar em caso de erro

# Configurações
LIBGPIOD_VERSION="1.6.3"
LIBGPIOD_URL="https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/snapshot/libgpiod-${LIBGPIOD_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps"
LIBGPIOD_DIR="$DEPS_DIR/libgpiod"
BUILD_DIR="$LIBGPIOD_DIR/build"
INSTALL_DIR="$LIBGPIOD_DIR/install"

# Configuração do cross-compiler
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export AR=arm-linux-gnueabihf-ar
export STRIP=arm-linux-gnueabihf-strip
export PKG_CONFIG_PATH=""

echo "=== Compilando libgpiod $LIBGPIOD_VERSION para ARM32 ==="
echo "Diretório do projeto: $PROJECT_ROOT"
echo "Diretório de dependências: $DEPS_DIR"

# Verificar se o cross-compiler está disponível
if ! command -v $CC &> /dev/null; then
    echo "ERRO: Cross-compiler $CC não encontrado!"
    echo "Instale com: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
    exit 1
fi

# Criar diretórios
mkdir -p "$DEPS_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"

cd "$DEPS_DIR"

# Baixar libgpiod se não existir
if [ ! -f "libgpiod-${LIBGPIOD_VERSION}.tar.gz" ]; then
    echo "Baixando libgpiod $LIBGPIOD_VERSION..."
    wget "$LIBGPIOD_URL" -O "libgpiod-${LIBGPIOD_VERSION}.tar.gz"
fi

# Extrair se não existir
if [ ! -d "libgpiod-${LIBGPIOD_VERSION}" ]; then
    echo "Extraindo libgpiod..."
    tar -xzf "libgpiod-${LIBGPIOD_VERSION}.tar.gz"
fi

cd "libgpiod-${LIBGPIOD_VERSION}"

# Gerar configure se não existir
if [ ! -f "configure" ]; then
    echo "Gerando scripts de configuração..."
    autoreconf -fiv
fi

# Configurar para cross-compilation
echo "Configurando libgpiod para ARM32..."
./configure \
    --host=arm-linux-gnueabihf \
    --prefix="$INSTALL_DIR" \
    --enable-tools=no \
    --enable-bindings-cxx=no \
    --enable-bindings-python=no \
    --enable-shared \
    --enable-static \
    --disable-tests \
    CFLAGS="-O2 -fPIC" \
    CXXFLAGS="-O2 -fPIC"

# Compilar
echo "Compilando libgpiod..."
make -j$(nproc)

# Instalar
echo "Instalando libgpiod em $INSTALL_DIR..."
make install

# Executar ranlib para garantir que o índice da biblioteca esteja correto
echo "Executando ranlib na biblioteca..."
$AR -s "$INSTALL_DIR/lib/libgpiod.a"

echo "=== libgpiod compilada com sucesso! ==="
echo "Biblioteca instalada em: $INSTALL_DIR"
echo "Headers: $INSTALL_DIR/include"
echo "Biblioteca: $INSTALL_DIR/lib/libgpiod.a"

# Verificar se os arquivos foram criados
if [ -f "$INSTALL_DIR/lib/libgpiod.a" ] && [ -f "$INSTALL_DIR/include/gpiod.h" ]; then
    echo "✅ Compilação bem-sucedida!"
    ls -la "$INSTALL_DIR/lib/libgpiod.a"
    ls -la "$INSTALL_DIR/include/gpiod.h"
else
    echo "❌ Erro: Arquivos esperados não foram encontrados!"
    exit 1
fi
