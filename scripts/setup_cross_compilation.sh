#!/bin/bash

# Script para configurar ambiente de cross-compilation para Raspberry Pi
# Instala dependências e compila libmodbus e libgpiod para ARM

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

log_info "=== Configurando ambiente de cross-compilation para Raspberry Pi ==="
log_info "Diretório do projeto: $PROJECT_ROOT"

# Verificar se está rodando como usuário normal (não root)
if [ "$EUID" -eq 0 ]; then
    log_error "Este script não deve ser executado como root!"
    log_error "Execute como usuário normal. O script pedirá sudo quando necessário."
    exit 1
fi

# Função para verificar se um comando existe
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Função para verificar distribuição Linux
check_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    else
        echo "unknown"
    fi
}

DISTRO=$(check_distro)
log_info "Distribuição detectada: $DISTRO"

# Verificar se é Ubuntu/Debian
if [[ "$DISTRO" != "ubuntu" && "$DISTRO" != "debian" ]]; then
    log_warning "Este script foi testado apenas no Ubuntu/Debian"
    log_warning "Pode ser necessário ajustar os comandos de instalação para sua distribuição"
    read -p "Deseja continuar mesmo assim? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "Instalação cancelada pelo usuário"
        exit 0
    fi
fi

# Atualizar repositórios
log_info "Atualizando repositórios do sistema..."
sudo apt-get update

# Instalar dependências básicas
log_info "Instalando dependências básicas..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    autotools-dev \
    autoconf \
    automake \
    libtool \
    autoconf-archive \
    gperf \
    python3 \
    python3-pip \
    python3-venv

# Instalar cross-compiler ARM
log_info "Instalando cross-compiler ARM..."
sudo apt-get install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf

# Verificar se cross-compiler foi instalado corretamente
if ! command_exists arm-linux-gnueabihf-gcc; then
    log_error "Cross-compiler ARM não foi instalado corretamente!"
    exit 1
fi

log_success "Cross-compiler ARM instalado:"
arm-linux-gnueabihf-gcc --version | head -1

# Instalar dependências para LVGL
log_info "Instalando dependências para LVGL..."
sudo apt-get install -y \
    libsdl2-dev \
    libevdev-dev

# Criar diretório de dependências se não existir
DEPS_DIR="$PROJECT_ROOT/deps"
mkdir -p "$DEPS_DIR"

log_info "Diretório de dependências: $DEPS_DIR"

# Compilar libmodbus para ARM
log_info "Compilando libmodbus para ARM..."
if [ -f "$SCRIPT_DIR/build_libmodbus_arm.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libmodbus_arm.sh"
    "$SCRIPT_DIR/build_libmodbus_arm.sh"
    if [ $? -eq 0 ]; then
        log_success "libmodbus compilada com sucesso!"
    else
        log_error "Falha na compilação da libmodbus!"
        exit 1
    fi
else
    log_error "Script build_libmodbus_arm.sh não encontrado!"
    exit 1
fi

# Compilar libgpiod para ARM
log_info "Compilando libgpiod para ARM..."
if [ -f "$SCRIPT_DIR/build_libgpiod_arm.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libgpiod_arm.sh"
    "$SCRIPT_DIR/build_libgpiod_arm.sh"
    if [ $? -eq 0 ]; then
        log_success "libgpiod compilada com sucesso!"
    else
        log_error "Falha na compilação da libgpiod!"
        exit 1
    fi
else
    log_error "Script build_libgpiod_arm.sh não encontrado!"
    exit 1
fi

# Compilar libudev (eudev) para ARM
log_info "Compilando libudev (eudev) para ARM..."
if [ -f "$SCRIPT_DIR/build_libudev_arm.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_libudev_arm.sh"
    "$SCRIPT_DIR/build_libudev_arm.sh"
    if [ $? -eq 0 ]; then
        log_success "libudev (eudev) compilada com sucesso!"
    else
        log_error "Falha na compilação da libudev!"
        exit 1
    fi
else
    log_error "Script build_libudev_arm.sh não encontrado!"
    exit 1
fi

# Compilar SQLite3 para ARM
log_info "Compilando SQLite3 para ARM..."
if [ -f "$SCRIPT_DIR/build_sqlite3_arm.sh" ]; then
    chmod +x "$SCRIPT_DIR/build_sqlite3_arm.sh"
    "$SCRIPT_DIR/build_sqlite3_arm.sh"
    if [ $? -eq 0 ]; then
        log_success "SQLite3 compilado com sucesso!"
    else
        log_error "Falha na compilação do SQLite3!"
        exit 1
    fi
else
    log_error "Script build_sqlite3_arm.sh não encontrado!"
    exit 1
fi

# Verificar se as bibliotecas foram compiladas corretamente
log_info "Verificando bibliotecas compiladas..."

LIBMODBUS_PATH="$DEPS_DIR/libmodbus/install/lib/libmodbus.so"
LIBGPIOD_PATH="$DEPS_DIR/libgpiod/install/lib/libgpiod.so"
LIBUDEV_PATH="$DEPS_DIR/eudev/install/lib/libudev.a"

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

# Verificar SQLite3
SQLITE3_PATH="$PROJECT_ROOT/deps/sqlite3/install/lib/libsqlite3.so"
if [ -f "$SQLITE3_PATH" ]; then
    log_success "SQLite3 encontrado: $SQLITE3_PATH"
    file "$SQLITE3_PATH" | grep -q "ARM" && log_success "SQLite3 é ARM" || log_warning "SQLite3 pode não ser ARM"
else
    log_error "SQLite3 não encontrado em $SQLITE3_PATH"
    exit 1
fi

# Verificar se CMakeLists.txt existe
log_info "Verificando CMakeLists.txt..."
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"

if [ -f "$CMAKE_FILE" ]; then
    log_success "CMakeLists.txt encontrado"
else
    log_error "CMakeLists.txt não encontrado!"
    log_error "O arquivo CMakeLists.txt é necessário para compilar o projeto"
    exit 1
fi

# Criar diretório de build se não existir
BUILD_DIR="$PROJECT_ROOT/build-rpi"
if [ ! -d "$BUILD_DIR" ]; then
    log_info "Criando diretório de build: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Configurar projeto com CMake
log_info "Configurando projeto com CMake..."
cd "$PROJECT_ROOT"

if cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build-rpi -S . > /tmp/cmake_config.log 2>&1; then
    log_success "CMake configurado com sucesso!"
else
    log_error "Falha na configuração do CMake!"
    log_error "Veja os detalhes em /tmp/cmake_config.log"
    tail -20 /tmp/cmake_config.log
    exit 1
fi

# Testar compilação do projeto
log_info "Testando compilação do projeto..."

if make -C build-rpi -j$(nproc) > /tmp/build_test.log 2>&1; then
    log_success "Projeto compilado com sucesso!"
    log_info "Executável gerado: $BUILD_DIR/bin/app"

    # Verificar se o executável é ARM
    if file "$BUILD_DIR/bin/app" | grep -q "ARM"; then
        log_success "Executável é ARM - cross-compilation funcionando!"
    else
        log_warning "Executável pode não ser ARM"
    fi
else
    log_error "Falha na compilação do projeto!"
    log_error "Veja os detalhes em /tmp/build_test.log"
    tail -20 /tmp/build_test.log
    exit 1
fi

# Resumo final
log_success "=== CONFIGURAÇÃO CONCLUÍDA COM SUCESSO! ==="
echo
log_info "Ambiente de cross-compilation configurado:"
log_info "✅ Cross-compiler ARM instalado"
log_info "✅ gperf instalado (necessário para eudev)"
log_info "✅ libmodbus compilada para ARM"
log_info "✅ libgpiod 1.6.3 compilada para ARM"
log_info "✅ libudev (eudev) compilada para ARM"
log_info "✅ SQLite3 compilado para ARM"
log_info "✅ CMakeLists.txt verificado"
log_info "✅ CMake configurado com toolchain ARM"
log_info "✅ Projeto modbus_reader compila corretamente"
echo
log_info "Para compilar o projeto:"
log_info "  cd $PROJECT_ROOT"
log_info "  make -C build-rpi -j\$(nproc)"
echo
log_info "Para reconfigurar o CMake (se necessário):"
log_info "  cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build-rpi -S ."
echo
log_info "Executável será gerado em:"
log_info "  $BUILD_DIR/bin/app"
echo
log_info "Para transferir para Raspberry Pi:"
log_info "  ./deploy_to_rpi.sh [IP_DA_RPI] [USUARIO]"
echo
log_info "Ou manualmente:"
log_info "  scp build-rpi/bin/app pi@<IP_DA_RPI>:~/"
log_info "  scp deps/libmodbus/install/lib/libmodbus.so* pi@<IP_DA_RPI>:/usr/local/lib/"
log_info "  scp deps/libgpiod/install/lib/libgpiod.so* pi@<IP_DA_RPI>:/usr/local/lib/"
echo
log_success "Ambiente pronto para desenvolvimento!"
