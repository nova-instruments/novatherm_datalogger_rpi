#!/bin/bash

# Script para configurar ambiente de cross-compilation para ARMv6 (Raspberry Pi 1)
# Autor: Nova Instruments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "========================================================"
echo "🔧 Setup Cross-Compilation para Raspberry Pi 1 (ARMv6)"
echo "========================================================"

# Verificar se estamos no Ubuntu/Debian
if ! command -v apt-get &> /dev/null; then
    log_error "Este script requer Ubuntu/Debian com apt-get"
    exit 1
fi

# Instalar dependências do sistema
log_info "Instalando dependências do sistema..."
sudo apt-get update -qq
sudo apt-get install -y \
    gcc-arm-linux-gnueabi \
    g++-arm-linux-gnueabi \
    cmake \
    build-essential \
    autotools-dev \
    automake \
    autoconf \
    libtool \
    pkg-config \
    gperf \
    wget \
    git

log_success "Dependências do sistema instaladas"

# Compilar libmodbus para ARMv6
log_info "Compilando libmodbus para ARMv6..."
if [ -f "$SCRIPT_DIR/build_libmodbus_armv6.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libmodbus_armv6.sh"
    "$SCRIPT_DIR/build_libmodbus_armv6.sh"
    log_success "libmodbus compilada com sucesso!"
else
    log_error "Script build_libmodbus_armv6.sh não encontrado!"
    exit 1
fi

# Compilar libgpiod para ARMv6
log_info "Compilando libgpiod para ARMv6..."
if [ -f "$SCRIPT_DIR/build_libgpiod_armv6.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libgpiod_armv6.sh"
    "$SCRIPT_DIR/build_libgpiod_armv6.sh"
    log_success "libgpiod compilada com sucesso!"
else
    log_error "Script build_libgpiod_armv6.sh não encontrado!"
    exit 1
fi

# Compilar libudev para ARMv6
log_info "Compilando libudev (eudev) para ARMv6..."
if [ -f "$SCRIPT_DIR/build_libudev_armv6.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libudev_armv6.sh"
    "$SCRIPT_DIR/build_libudev_armv6.sh"
    log_success "libudev (eudev) compilada com sucesso!"
else
    log_error "Script build_libudev_armv6.sh não encontrado!"
    exit 1
fi

# Compilar SQLite3 para ARMv6
log_info "Compilando SQLite3 para ARMv6..."
if [ -f "$SCRIPT_DIR/build_sqlite3_armv6.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_sqlite3_armv6.sh"
    "$SCRIPT_DIR/build_sqlite3_armv6.sh"
    log_success "SQLite3 compilado com sucesso!"
else
    log_error "Script build_sqlite3_armv6.sh não encontrado!"
    exit 1
fi

# Verificar se as bibliotecas foram compiladas corretamente
log_info "Verificando bibliotecas compiladas..."

LIBMODBUS_PATH="$PROJECT_ROOT/deps-armv6/libmodbus/install/lib/libmodbus.so"
LIBGPIOD_PATH="$PROJECT_ROOT/deps-armv6/libgpiod/install/lib/libgpiod.so"
LIBUDEV_PATH="$PROJECT_ROOT/deps-armv6/eudev/install/lib/libudev.a"
SQLITE3_PATH="$PROJECT_ROOT/deps-armv6/sqlite3/install/lib/libsqlite3.so"

if [ -f "$LIBMODBUS_PATH" ]; then
    log_success "libmodbus encontrada: $LIBMODBUS_PATH"
    file "$LIBMODBUS_PATH" | grep -q "ARM" && log_success "libmodbus é ARM" || log_warning "libmodbus pode não ser ARM"
else
    log_error "libmodbus não encontrada em $LIBMODBUS_PATH"
    exit 1
fi

if [ -f "$LIBGPIOD_PATH" ]; then
    log_success "libgpiod encontrada: $LIBGPIOD_PATH"
    file "$LIBGPIOD_PATH" | grep -q "ARM" && log_success "libgpiod é ARM" || log_warning "libgpiod pode não ser ARM"
else
    log_error "libgpiod não encontrada em $LIBGPIOD_PATH"
    exit 1
fi

if [ -f "$LIBUDEV_PATH" ]; then
    log_success "libudev encontrada: $LIBUDEV_PATH"
    file "$LIBUDEV_PATH" | grep -q "ARM" && log_success "libudev é ARM" || log_warning "libudev pode não ser ARM"
else
    log_error "libudev não encontrada em $LIBUDEV_PATH"
    exit 1
fi

if [ -f "$SQLITE3_PATH" ]; then
    log_success "SQLite3 encontrado: $SQLITE3_PATH"
    file "$SQLITE3_PATH" | grep -q "ARM" && log_success "SQLite3 é ARM" || log_warning "SQLite3 pode não ser ARM"
else
    log_error "SQLite3 não encontrado em $SQLITE3_PATH"
    exit 1
fi

# Verificar se CMakeLists_armv6.txt existe
if [ -f "$PROJECT_ROOT/CMakeLists_armv6.txt" ]; then
    log_success "CMakeLists_armv6.txt encontrado"
else
    log_error "CMakeLists_armv6.txt não encontrado!"
    log_error "O arquivo CMakeLists_armv6.txt é necessário para compilar o projeto"
    exit 1
fi

# Criar diretório de build se não existir
BUILD_DIR="$PROJECT_ROOT/build-rpi1"
if [ ! -d "$BUILD_DIR" ]; then
    log_info "Criando diretório de build: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Configurar projeto com CMake
log_info "Configurando projeto com CMake para ARMv6..."
cd "$BUILD_DIR"
# Copiar CMakeLists específico para ARMv6
cp "$PROJECT_ROOT/CMakeLists_armv6.txt" "$PROJECT_ROOT/CMakeLists.txt.backup"
cp "$PROJECT_ROOT/CMakeLists_armv6.txt" "$PROJECT_ROOT/CMakeLists.txt"

cmake -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/user_cross_compile_setup_armv6.cmake" \
      -S "$PROJECT_ROOT" \
      -B "$BUILD_DIR"

# Restaurar CMakeLists original
mv "$PROJECT_ROOT/CMakeLists.txt.backup" "$PROJECT_ROOT/CMakeLists.txt"

# Compilar projeto
log_info "Compilando projeto para ARMv6..."
make -j$(nproc)

if [ -f "$BUILD_DIR/bin/app" ]; then
    log_success "Projeto compilado com sucesso!"
    log_info "Executável gerado: $BUILD_DIR/bin/app"
    
    # Verificar arquitetura do executável
    log_info "Verificando arquitetura do executável:"
    file "$BUILD_DIR/bin/app"
else
    log_error "Falha na compilação do projeto"
    exit 1
fi

echo
echo "========================================================"
log_success "Setup ARMv6 concluído com sucesso!"
echo "========================================================"
log_info "Ambiente de cross-compilation configurado:"
log_info "✅ Cross-compiler ARM instalado"
log_info "✅ gperf instalado (necessário para eudev)"
log_info "✅ libmodbus compilada para ARMv6"
log_info "✅ libgpiod 1.6.3 compilada para ARMv6"
log_info "✅ libudev (eudev) compilada para ARMv6"
log_info "✅ SQLite3 compilado para ARMv6"
log_info "✅ CMakeLists_armv6.txt verificado"
log_info "✅ CMake configurado com toolchain ARMv6"
log_info "✅ Projeto modbus_reader compila corretamente para ARMv6"
echo
log_info "Para recompilar o projeto:"
log_info "  cd build-rpi1 && make -j\$(nproc)"
echo
log_info "Para fazer deploy no Raspberry Pi 1:"
log_info "  scp build-rpi1/bin/app pi@<IP_DO_RPI>:~/"
echo
