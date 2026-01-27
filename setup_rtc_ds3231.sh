#!/bin/bash

# ============================================================================
# Script de Configuração Automática do RTC DS3231 para Raspberry Pi
# ============================================================================
# 
# Este script automatiza a configuração do módulo RTC DS3231 no Raspberry Pi
# conforme descrito no tutorial_ds3231_raspberry.md
#
# Uso: sudo ./setup_rtc_ds3231.sh
# ============================================================================

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funções de log
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

# Verificar se está rodando como root
if [ "$EUID" -ne 0 ]; then 
    log_error "Este script precisa ser executado como root (use sudo)"
    exit 1
fi

echo "========================================================"
echo "  Configuração do RTC DS3231 para Raspberry Pi"
echo "========================================================"
echo ""

# ============================================================================
# 1. Instalar pacotes necessários
# ============================================================================
log_info "Instalando pacotes necessários (python3-smbus, i2c-tools)..."
apt-get update -qq
apt-get install -y python3-smbus i2c-tools
log_success "Pacotes instalados"

# ============================================================================
# 2. Habilitar I2C
# ============================================================================
log_info "Habilitando interface I2C..."

# Verificar se I2C já está habilitado
if ! grep -q "^dtparam=i2c_arm=on" /boot/firmware/config.txt; then
    echo "dtparam=i2c_arm=on" >> /boot/firmware/config.txt
    log_success "I2C habilitado em /boot/firmware/config.txt"
else
    log_warning "I2C já estava habilitado"
fi

# Carregar módulo I2C imediatamente (sem precisar reiniciar)
if ! lsmod | grep -q i2c_dev; then
    modprobe i2c-dev
    log_success "Módulo i2c-dev carregado"
fi

# Garantir que módulo I2C seja carregado no boot
if ! grep -q "i2c-dev" /etc/modules; then
    echo "i2c-dev" >> /etc/modules
    log_success "Módulo i2c-dev adicionado ao /etc/modules"
fi

# ============================================================================
# 3. Verificar dispositivo I2C
# ============================================================================
log_info "Verificando dispositivo I2C..."
if [ -e /dev/i2c-1 ]; then
    log_success "Dispositivo /dev/i2c-1 encontrado"
else
    log_error "Dispositivo /dev/i2c-1 não encontrado!"
    log_error "Verifique as conexões do módulo DS3231:"
    log_error "  VCC → 3V3 (pino 1 ou 17)"
    log_error "  GND → GND (pino 6, 9, 14, 20, 25, 30, 34, 39)"
    log_error "  SDA → GPIO2 (pino físico 3)"
    log_error "  SCL → GPIO3 (pino físico 5)"
    exit 1
fi

# ============================================================================
# 4. Detectar DS3231 no barramento I2C
# ============================================================================
log_info "Detectando DS3231 no barramento I2C (endereço 0x68)..."

# Capturar saída do i2cdetect
I2C_OUTPUT=$(i2cdetect -y 1)

# Verificar se DS3231 está presente (68 ou UU)
# UU = driver do kernel já está usando o dispositivo (correto!)
# 68 = dispositivo detectado mas driver não carregado
if echo "$I2C_OUTPUT" | grep -qE " (68|UU) "; then
    if echo "$I2C_OUTPUT" | grep -q " UU "; then
        log_success "DS3231 detectado no endereço 0x68 (driver já carregado - UU)"
    else
        log_success "DS3231 detectado no endereço 0x68"
    fi
else
    log_error "DS3231 NÃO detectado no endereço 0x68!"
    log_error "Verifique as conexões do módulo"
    log_info "Saída do i2cdetect:"
    echo "$I2C_OUTPUT"
    exit 1
fi

# ============================================================================
# 5. Ativar overlay do RTC
# ============================================================================
log_info "Configurando overlay do RTC DS3231..."

# Verificar se overlay já está configurado
if grep -q "^dtoverlay=i2c-rtc,ds3231" /boot/firmware/config.txt; then
    log_warning "Overlay do RTC DS3231 já estava configurado"
else
    echo "dtoverlay=i2c-rtc,ds3231" >> /boot/firmware/config.txt
    log_success "Overlay do RTC DS3231 adicionado ao /boot/firmware/config.txt"
fi

# ============================================================================
# 6. Desativar fake-hwclock
# ============================================================================
log_info "Removendo fake-hwclock..."
if dpkg -l | grep -q fake-hwclock; then
    apt-get remove -y fake-hwclock
    systemctl disable fake-hwclock 2>/dev/null || true
    log_success "fake-hwclock removido"
else
    log_warning "fake-hwclock já estava removido"
fi

# ============================================================================
# Finalização
# ============================================================================
echo ""
echo "========================================================"
log_success "Configuração do RTC DS3231 concluída!"
echo "========================================================"
echo ""
log_warning "É necessário REINICIAR o Raspberry Pi para aplicar as mudanças"
echo ""
log_info "Após reiniciar, você pode:"
log_info "  • Verificar o RTC: sudo hwclock -r"
log_info "  • Gravar hora no RTC: sudo hwclock -w"
log_info "  • Carregar hora do RTC: sudo hwclock -s"
echo ""
read -p "Deseja reiniciar agora? (s/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[SsYy]$ ]]; then
    log_info "Reiniciando em 3 segundos..."
    sleep 3
    reboot
else
    log_info "Reinicie manualmente com: sudo reboot"
fi

