#!/bin/bash

# Script para compilar SQLite3 para ARMv6 (Raspberry Pi 1)
# Autor: Nova Instruments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps-armv6"
SQLITE3_DIR="$DEPS_DIR/sqlite3"

echo "=== Compilando SQLite3 para ARMv6 (Raspberry Pi 1) ==="

# Criar diretório de dependências
mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

# Baixar SQLite3 se não existir
if [ ! -d "sqlite-autoconf-3430200" ]; then
    echo "Baixando SQLite3 3430200..."
    wget -q https://www.sqlite.org/2023/sqlite-autoconf-3430200.tar.gz
    tar -xzf sqlite-autoconf-3430200.tar.gz
    rm sqlite-autoconf-3430200.tar.gz
fi

cd sqlite-autoconf-3430200

# Limpar build anterior
make clean 2>/dev/null || true

echo "Configurando SQLite3 para ARMv6..."
./configure \
    --host=arm-linux-gnueabi \
    --prefix="$SQLITE3_DIR/install" \
    --enable-static \
    --enable-shared \
    --disable-tcl \
    --enable-fts3 \
    --enable-fts4 \
    --enable-rtree \
    CC=arm-linux-gnueabi-gcc \
    CXX=arm-linux-gnueabi-g++ \
    CFLAGS="-march=armv6 -mfloat-abi=soft -O2 -fPIC -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_RTREE" \
    CXXFLAGS="-march=armv6 -mfloat-abi=soft -O2"

echo "Compilando SQLite3..."
make -j$(nproc)

echo "Instalando SQLite3..."
make install

echo "✅ SQLite3 compilado com sucesso para ARMv6!"
echo "   Biblioteca: $SQLITE3_DIR/install/lib/libsqlite3.so"
echo "   Headers: $SQLITE3_DIR/install/include/sqlite3.h"

# Verificar arquitetura
echo "Informações da biblioteca:"
file "$SQLITE3_DIR/install/lib/libsqlite3.so"
