#!/bin/bash

# Script para compilar SQLite3 para ARM (cross-compilation)

set -e  # Parar em caso de erro

# Configurações
SQLITE_VERSION="3430200"  # SQLite 3.43.2
SQLITE_URL="https://www.sqlite.org/2023/sqlite-autoconf-${SQLITE_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps"
SQLITE_DIR="$DEPS_DIR/sqlite3"
BUILD_DIR="$SQLITE_DIR/build"
INSTALL_DIR="$SQLITE_DIR/install"

# Configuração do cross-compiler
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export AR=arm-linux-gnueabihf-ar
export STRIP=arm-linux-gnueabihf-strip
export PKG_CONFIG_PATH=""

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Compilando SQLite3 para ARM ===${NC}"

# Verificar se cross-compiler existe
if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
    echo -e "${RED}Erro: Cross-compiler ARM não encontrado!${NC}"
    echo "Execute: sudo apt-get install gcc-arm-linux-gnueabihf"
    exit 1
fi

# Criar diretórios
mkdir -p "$DEPS_DIR"
mkdir -p "$SQLITE_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"

cd "$SQLITE_DIR"

# Baixar SQLite3 se não existir
if [ ! -f "sqlite-autoconf-${SQLITE_VERSION}.tar.gz" ]; then
    echo -e "${YELLOW}Baixando SQLite3 ${SQLITE_VERSION}...${NC}"
    wget "$SQLITE_URL" -O "sqlite-autoconf-${SQLITE_VERSION}.tar.gz"
fi

# Extrair se não existir
if [ ! -d "sqlite-autoconf-${SQLITE_VERSION}" ]; then
    echo "Extraindo SQLite3..."
    tar -xzf "sqlite-autoconf-${SQLITE_VERSION}.tar.gz"
fi

cd "sqlite-autoconf-${SQLITE_VERSION}"

# Configurar para cross-compilation
echo -e "${YELLOW}Configurando SQLite3 para ARM32...${NC}"
./configure \
    --host=arm-linux-gnueabihf \
    --prefix="$INSTALL_DIR" \
    --enable-shared \
    --enable-static \
    --disable-tcl \
    --disable-readline \
    CFLAGS="-O2 -fPIC -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_RTREE"

# Compilar
echo -e "${YELLOW}Compilando SQLite3...${NC}"
make -j$(nproc)

# Instalar
echo -e "${YELLOW}Instalando SQLite3...${NC}"
make install

# Verificar instalação
if [ -f "$INSTALL_DIR/lib/libsqlite3.so" ] && [ -f "$INSTALL_DIR/lib/libsqlite3.a" ]; then
    echo -e "${GREEN}✅ SQLite3 compilado com sucesso!${NC}"
    echo -e "${GREEN}   Biblioteca: $INSTALL_DIR/lib/libsqlite3.so${NC}"
    echo -e "${GREEN}   Headers: $INSTALL_DIR/include/sqlite3.h${NC}"
    
    # Mostrar informações da biblioteca
    echo -e "${YELLOW}Informações da biblioteca:${NC}"
    file "$INSTALL_DIR/lib/libsqlite3.so"
    ls -lh "$INSTALL_DIR/lib/libsqlite3.*"
else
    echo -e "${RED}❌ Erro: SQLite3 não foi compilado corretamente!${NC}"
    exit 1
fi

echo -e "${GREEN}=== SQLite3 ARM build concluído ===${NC}"
