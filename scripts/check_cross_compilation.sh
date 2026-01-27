#!/bin/bash

# Script para verificar se o ambiente de cross-compilation está configurado corretamente

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para log colorido
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

# Configurações
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

log_info "=== Verificando ambiente de cross-compilation ==="
log_info "Diretório do projeto: $PROJECT_ROOT"

# Função para verificar se um comando existe
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Verificar cross-compiler
log_info "Verificando cross-compiler ARM..."
if command_exists arm-linux-gnueabihf-gcc; then
    log_success "Cross-compiler ARM encontrado:"
    arm-linux-gnueabihf-gcc --version | head -1
else
    log_error "Cross-compiler ARM não encontrado!"
    log_error "Execute: sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"
    exit 1
fi

# Verificar gperf
log_info "Verificando gperf..."
if command_exists gperf; then
    log_success "gperf encontrado: $(gperf --version | head -1)"
else
    log_error "gperf não encontrado!"
    log_error "Execute: sudo apt-get install gperf"
    exit 1
fi

# Verificar CMake
log_info "Verificando CMake..."
if command_exists cmake; then
    log_success "CMake encontrado: $(cmake --version | head -1)"
else
    log_error "CMake não encontrado!"
    log_error "Execute: sudo apt-get install cmake"
    exit 1
fi

# Verificar bibliotecas ARM
log_info "Verificando bibliotecas ARM compiladas..."

DEPS_DIR="$PROJECT_ROOT/deps"
LIBMODBUS_PATH="$DEPS_DIR/libmodbus/install/lib/libmodbus.so"
LIBGPIOD_PATH="$DEPS_DIR/libgpiod/install/lib/libgpiod.so"
LIBUDEV_PATH="$DEPS_DIR/eudev/install/lib/libudev.a"

if [ -f "$LIBMODBUS_PATH" ]; then
    log_success "libmodbus ARM encontrada: $LIBMODBUS_PATH"
    file "$LIBMODBUS_PATH" | grep -q "ARM" && log_success "  ✓ Arquitetura ARM confirmada" || log_warning "  ⚠ Pode não ser ARM"
else
    log_error "libmodbus ARM não encontrada!"
    log_error "Execute: ./scripts/build_libmodbus_arm.sh"
fi

if [ -f "$LIBGPIOD_PATH" ]; then
    log_success "libgpiod ARM encontrada: $LIBGPIOD_PATH"
    file "$LIBGPIOD_PATH" | grep -q "ARM" && log_success "  ✓ Arquitetura ARM confirmada" || log_warning "  ⚠ Pode não ser ARM"
else
    log_error "libgpiod ARM não encontrada!"
    log_error "Execute: ./scripts/build_libgpiod_arm.sh"
fi

if [ -f "$LIBUDEV_PATH" ]; then
    log_success "libudev ARM encontrada: $LIBUDEV_PATH"
    file "$LIBUDEV_PATH" | grep -q "ARM" && log_success "  ✓ Arquitetura ARM confirmada" || log_warning "  ⚠ Pode não ser ARM"
else
    log_error "libudev ARM não encontrada!"
    log_error "Execute: ./scripts/build_libudev_arm.sh"
fi

# Verificar arquivo de configuração
log_info "Verificando CMakeLists.txt..."
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"

if [ -f "$CMAKE_FILE" ]; then
    log_success "CMakeLists.txt encontrado"

    # Verificar se é para modbus_reader
    if grep -q "modbus_reader" "$CMAKE_FILE"; then
        log_success "  ✓ Configurado para modbus_reader"
    else
        log_warning "  ⚠ Pode não estar configurado para modbus_reader"
    fi

    # Verificar dependências
    if grep -q "libmodbus" "$CMAKE_FILE"; then
        log_success "  ✓ libmodbus configurada"
    else
        log_warning "  ⚠ libmodbus pode não estar configurada"
    fi
else
    log_error "CMakeLists.txt não encontrado!"
fi

# Verificar toolchain file
log_info "Verificando toolchain file..."
TOOLCHAIN_FILE="$PROJECT_ROOT/user_cross_compile_setup.cmake"

if [ -f "$TOOLCHAIN_FILE" ]; then
    log_success "Toolchain file encontrado: $TOOLCHAIN_FILE"
else
    log_error "Toolchain file não encontrado!"
fi

# Verificar diretório de build
log_info "Verificando diretório de build..."
BUILD_DIR="$PROJECT_ROOT/build-rpi"

if [ -d "$BUILD_DIR" ]; then
    log_success "Diretório de build encontrado: $BUILD_DIR"
    
    # Verificar se há executável
    if [ -f "$BUILD_DIR/bin/app" ]; then
        log_success "Executável encontrado: $BUILD_DIR/bin/app"

        # Verificar arquitetura
        if file "$BUILD_DIR/bin/app" | grep -q "ARM"; then
            log_success "  ✓ Executável é ARM"
        else
            log_warning "  ⚠ Executável pode não ser ARM"
        fi

        # Mostrar tamanho
        SIZE=$(ls -lh "$BUILD_DIR/bin/app" | awk '{print $5}')
        log_info "  Tamanho: $SIZE"
    else
        log_warning "Executável não encontrado - execute make para compilar"
    fi
else
    log_warning "Diretório de build não encontrado"
fi

echo
log_info "=== RESUMO DA VERIFICAÇÃO ==="

# Contar sucessos e erros
ERRORS=0
WARNINGS=0

# Verificar novamente os componentes críticos
if ! command_exists arm-linux-gnueabihf-gcc; then ((ERRORS++)); fi
if ! command_exists gperf; then ((ERRORS++)); fi
if ! command_exists cmake; then ((ERRORS++)); fi
if [ ! -f "$LIBMODBUS_PATH" ]; then ((ERRORS++)); fi
if [ ! -f "$LIBGPIOD_PATH" ]; then ((ERRORS++)); fi
if [ ! -f "$LIBUDEV_PATH" ]; then ((ERRORS++)); fi
if [ ! -f "$CMAKE_FILE" ]; then ((ERRORS++)); fi
if [ ! -f "$TOOLCHAIN_FILE" ]; then ((ERRORS++)); fi

if [ $ERRORS -eq 0 ]; then
    log_success "✅ Ambiente de cross-compilation está configurado corretamente!"
    echo
    log_info "Para compilar o projeto:"
    log_info "  cd $PROJECT_ROOT"
    log_info "  make -C build-rpi -j\$(nproc)"
    echo
    log_info "Para reconfigurar (se necessário):"
    log_info "  cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build-rpi -S ."
else
    log_error "❌ Encontrados $ERRORS problemas no ambiente!"
    log_error "Execute o script de configuração: ./scripts/setup_cross_compilation.sh"
    exit 1
fi
