#!/bin/bash

# Script para compilar eudev (libudev) para ARM (cross-compilation)
# Baseado no script da libgpiod

set -e  # Parar em caso de erro

# Configurações
EUDEV_VERSION="3.2.14"
EUDEV_URL="https://github.com/eudev-project/eudev/releases/download/v${EUDEV_VERSION}/eudev-${EUDEV_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps"
EUDEV_DIR="$DEPS_DIR/eudev"
BUILD_DIR="$EUDEV_DIR/build"
INSTALL_DIR="$EUDEV_DIR/install"

# Configuração do cross-compiler
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export AR=arm-linux-gnueabihf-ar
export STRIP=arm-linux-gnueabihf-strip
export PKG_CONFIG_PATH=""

echo "=== Compilando eudev (libudev) $EUDEV_VERSION para ARM32 ==="
echo "Diretório do projeto: $PROJECT_ROOT"
echo "Diretório de dependências: $DEPS_DIR"

# Verificar se o cross-compiler está disponível
if ! command -v $CC &> /dev/null; then
    echo "ERRO: Cross-compiler $CC não encontrado!"
    echo "Instale com: sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"
    exit 1
fi

# Criar diretórios
mkdir -p "$DEPS_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"

cd "$DEPS_DIR"

# Baixar eudev se não existir
if [ ! -f "eudev-${EUDEV_VERSION}.tar.gz" ]; then
    echo "Baixando eudev $EUDEV_VERSION..."
    wget "$EUDEV_URL" -O "eudev-${EUDEV_VERSION}.tar.gz"
fi

# Extrair se não existir
if [ ! -d "eudev-${EUDEV_VERSION}" ]; then
    echo "Extraindo eudev..."
    tar -xzf "eudev-${EUDEV_VERSION}.tar.gz"
fi

cd "eudev-${EUDEV_VERSION}"

# Gerar configure se não existir
if [ ! -f "configure" ]; then
    echo "Gerando scripts de configuração..."
    autoreconf -fiv
fi

# Configurar para cross-compilation
echo "Configurando eudev para ARM32..."
./configure \
    --host=arm-linux-gnueabihf \
    --prefix="$INSTALL_DIR" \
    --enable-static \
    --enable-shared \
    --disable-hwdb \
    --disable-manpages \
    --disable-kmod \
    --disable-selinux \
    --disable-blkid \
    CFLAGS="-O2 -fPIC" \
    CXXFLAGS="-O2 -fPIC"

# Compilar
echo "Compilando eudev..."
make -j$(nproc)

# Instalar
echo "Instalando eudev em $INSTALL_DIR..."
make install

# Executar ranlib para garantir que o índice da biblioteca esteja correto
echo "Executando ranlib na biblioteca..."
$AR -s "$INSTALL_DIR/lib/libudev.a"

echo "=== eudev (libudev) compilada com sucesso! ==="
echo "Biblioteca instalada em: $INSTALL_DIR"
echo "Headers: $INSTALL_DIR/include"
echo "Biblioteca: $INSTALL_DIR/lib/libudev.a"

# Verificar se os arquivos foram criados
if [ -f "$INSTALL_DIR/lib/libudev.a" ] && [ -f "$INSTALL_DIR/include/libudev.h" ]; then
    echo "✅ Compilação bem-sucedida!"
    ls -la "$INSTALL_DIR/lib/libudev.a"
    ls -la "$INSTALL_DIR/include/libudev.h"
else
    echo "❌ Erro: Arquivos esperados não foram encontrados!"
    exit 1
fi
