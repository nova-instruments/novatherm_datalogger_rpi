# Makefile para o projeto Modbus Reader
# Facilita o uso dos scripts de build e deploy

.PHONY: help setup build clean check deploy test armv6 setup-armv6 build-armv6 clean-armv6 deploy-armv6 info-armv6

# Configurações
BUILD_DIR = build-rpi
BUILD_DIR_ARMV6 = build-rpi1
RPI_IP ?= 192.168.3.22
RPI_USER ?= nova

help:
	@echo "=== Modbus Reader - Comandos Disponíveis ==="
	@echo ""
	@echo "📋 Configuração:"
	@echo "  make setup     - Configura ambiente de cross-compilation"
	@echo "  make check     - Verifica se ambiente está configurado"
	@echo ""
	@echo "🔨 Compilação:"
	@echo "  make build     - Compila o projeto para ARM"
	@echo "  make clean     - Limpa arquivos de build"
	@echo "  make rebuild   - Limpa e recompila"
	@echo ""
	@echo "🚀 Deploy:"
	@echo "  make deploy    - Envia para Raspberry Pi (IP=$(RPI_IP), USER=$(RPI_USER))"
	@echo "  make deploy RPI_IP=<ip> RPI_USER=<user> - Deploy com IP/usuário específicos"
	@echo ""
	@echo "🧪 Testes:"
	@echo "  make test      - Executa verificações básicas"
	@echo ""
	@echo "� ARMv6 (Raspberry Pi 1):"
	@echo "  make setup-armv6  - Configura ambiente para ARMv6"
	@echo "  make build-armv6  - Compila para ARMv6 (Raspberry Pi 1)"
	@echo "  make clean-armv6  - Limpa build ARMv6"
	@echo "  make deploy-armv6 - Deploy para Raspberry Pi 1"
	@echo "  make info-armv6   - Informações do build ARMv6"
	@echo ""
	@echo "�📖 Informações:"
	@echo "  make info      - Mostra informações do projeto"

setup:
	@echo "🔧 Configurando ambiente de cross-compilation..."
	@chmod +x scripts/setup_cross_compilation.sh
	@./scripts/setup_cross_compilation.sh

check:
	@echo "🔍 Verificando ambiente..."
	@chmod +x scripts/check_cross_compilation.sh
	@./scripts/check_cross_compilation.sh

build:
	@echo "🔨 Compilando projeto..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "⚠️  Diretório de build não existe. Execute 'make setup' primeiro."; \
		exit 1; \
	fi
	@make -C $(BUILD_DIR) -j$$(nproc)
	@echo "✅ Compilação concluída!"
	@echo "📁 Executável: $(BUILD_DIR)/bin/app"

clean:
	@echo "🧹 Limpando arquivos de build..."
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf $(BUILD_DIR); \
		echo "✅ Diretório $(BUILD_DIR) removido"; \
	else \
		echo "ℹ️  Nada para limpar"; \
	fi

rebuild: clean setup build

deploy:
	@echo "🚀 Fazendo deploy para Raspberry Pi (ARMv7+)..."
	@if [ ! -f "$(BUILD_DIR)/bin/app_armv7" ]; then \
		echo "❌ Executável ARMv7 não encontrado. Execute 'make build' primeiro."; \
		exit 1; \
	fi
	@echo "📤 Enviando arquivos para $(RPI_USER)@$(RPI_IP)..."
	@scp $(BUILD_DIR)/bin/app_armv7 $(RPI_USER)@$(RPI_IP):~/app_armv7
	@echo "✅ Deploy ARMv7 concluído!"

test: check
	@echo "🧪 Executando testes básicos..."
	@if [ -f "$(BUILD_DIR)/bin/app_armv7" ]; then \
		echo "✅ Executável existe"; \
		file $(BUILD_DIR)/bin/app_armv7; \
		ls -lh $(BUILD_DIR)/bin/app_armv7; \
	else \
		echo "❌ Executável não encontrado"; \
		exit 1; \
	fi

info:
	@echo "=== Informações do Projeto Modbus Reader ==="
	@echo ""
	@echo "📁 Estrutura:"
	@echo "  • src/main.c               - Código principal"
	@echo "  • lib/usb_manager.*        - Biblioteca USB"
	@echo "  • CMakeLists.txt           - Configuração CMake"
	@echo "  • user_cross_compile_setup.cmake - Toolchain ARM"
	@echo "  • scripts/                 - Scripts de build"
	@echo "  • deps/                    - Dependências compiladas"
	@echo ""
	@echo "🔧 Dependências:"
	@echo "  • libmodbus $(shell [ -f deps/libmodbus/install/lib/libmodbus.so ] && echo '✅' || echo '❌')"
	@echo "  • libgpiod  $(shell [ -f deps/libgpiod/install/lib/libgpiod.so ] && echo '✅' || echo '❌')"
	@echo "  • libudev   $(shell [ -f deps/eudev/install/lib/libudev.a ] && echo '✅' || echo '❌')"
	@echo "  • sqlite3   $(shell [ -f deps/sqlite3/install/lib/libsqlite3.so ] && echo '✅' || echo '❌')"
	@echo ""
	@echo "🎯 Alvo: Raspberry Pi 3 (ARM Cortex-A53)"
	@echo "📡 Protocolo: Modbus RTU via RS-485"
	@echo ""
	@if [ -f "$(BUILD_DIR)/bin/app" ]; then \
		echo "📦 Executável: ✅ $(BUILD_DIR)/bin/app"; \
		echo "📏 Tamanho: $$(ls -lh $(BUILD_DIR)/bin/app | awk '{print $$5}')"; \
	else \
		echo "📦 Executável: ❌ Não compilado"; \
	fi

# ========================================
# Comandos para ARMv6 (Raspberry Pi 1)
# ========================================

setup-armv6:
	@echo "🔧 Configurando ambiente de cross-compilation para ARMv6..."
	@chmod +x scripts/setup_armv6.sh
	@./scripts/setup_armv6.sh

build-armv6:
	@echo "🔨 Compilando projeto para ARMv6..."
	@if [ ! -d "$(BUILD_DIR_ARMV6)" ]; then \
		echo "⚠️  Diretório de build ARMv6 não existe. Execute 'make setup-armv6' primeiro."; \
		exit 1; \
	fi
	@make -C $(BUILD_DIR_ARMV6) -j$$(nproc)
	@echo "✅ Compilação ARMv6 concluída!"
	@echo "📁 Executável: $(BUILD_DIR_ARMV6)/bin/app"

clean-armv6:
	@echo "🧹 Limpando arquivos de build ARMv6..."
	@if [ -d "$(BUILD_DIR_ARMV6)" ]; then \
		rm -rf $(BUILD_DIR_ARMV6); \
		echo "✅ Diretório $(BUILD_DIR_ARMV6) removido"; \
	else \
		echo "ℹ️  Nada para limpar"; \
	fi
	@if [ -d "deps-armv6" ]; then \
		echo "🗑️  Removendo dependências ARMv6..."; \
		rm -rf deps-armv6; \
		echo "✅ Dependências ARMv6 removidas"; \
	fi

deploy-armv6:
	@echo "🚀 Fazendo deploy para Raspberry Pi 1 (ARMv6)..."
	@if [ ! -f "$(BUILD_DIR_ARMV6)/bin/app_armv6" ]; then \
		echo "❌ Executável ARMv6 não encontrado. Execute 'make build-armv6' primeiro."; \
		exit 1; \
	fi
	@echo "📤 Enviando arquivos para $(RPI_USER)@$(RPI_IP)..."
	@scp $(BUILD_DIR_ARMV6)/bin/app_armv6 $(RPI_USER)@$(RPI_IP):~/app_armv6
	@scp install_service.sh $(RPI_USER)@$(RPI_IP):~/install_service.sh
	@scp uninstall_service.sh $(RPI_USER)@$(RPI_IP):~/uninstall_service.sh
	@scp config.txt $(RPI_USER)@$(RPI_IP):~/config.txt
	@ssh $(RPI_USER)@$(RPI_IP) "chmod +x ~/install_service.sh ~/uninstall_service.sh"
	@echo "✅ Deploy ARMv6 concluído!"
	@echo ""
	@echo "📋 Arquivos enviados:"
	@echo "  • app_armv6 (executável)"
	@echo "  • install_service.sh (instalador do serviço)"
	@echo "  • uninstall_service.sh (desinstalador do serviço)"
	@echo "  • config.txt (arquivo de configuração)"
	@echo ""
	@echo "🎯 Para instalar como serviço:"
	@echo "   ssh $(RPI_USER)@$(RPI_IP)"
	@echo "   sudo ./install_service.sh"
	@echo ""
	@echo "🎯 Para executar manualmente:"
	@echo "   sudo ./app_armv6"

info-armv6:
	@echo "=== Informações do Projeto Modbus Reader ARMv6 ==="
	@echo ""
	@echo "🔧 Dependências ARMv6:"
	@echo "  • libmodbus $(shell [ -f deps-armv6/libmodbus/install/lib/libmodbus.so ] && echo '✅' || echo '❌')"
	@echo "  • libgpiod  $(shell [ -f deps-armv6/libgpiod/install/lib/libgpiod.so ] && echo '✅' || echo '❌')"
	@echo "  • libudev   $(shell [ -f deps-armv6/eudev/install/lib/libudev.a ] && echo '✅' || echo '❌')"
	@echo "  • sqlite3   $(shell [ -f deps-armv6/sqlite3/install/lib/libsqlite3.so ] && echo '✅' || echo '❌')"
	@echo ""
	@echo "🎯 Alvo: Raspberry Pi 1 (ARMv6)"
	@echo "📡 Protocolo: Modbus RTU via RS-485"
	@echo "🔧 Compilação: Estática (sem dependências externas)"
	@echo ""
	@if [ -f "$(BUILD_DIR_ARMV6)/bin/app_armv6" ]; then \
		echo "📦 Executável ARMv6: ✅ $(BUILD_DIR_ARMV6)/bin/app_armv6"; \
		echo "📏 Tamanho: $$(ls -lh $(BUILD_DIR_ARMV6)/bin/app_armv6 | awk '{print $$5}')"; \
		echo "🏗️  Arquitetura: $$(file $(BUILD_DIR_ARMV6)/bin/app_armv6 | cut -d: -f2)"; \
		echo "🔗 Linking: $$(file $(BUILD_DIR_ARMV6)/bin/app_armv6 | grep -o 'statically linked' || echo 'dinamicamente linkado')"; \
	else \
		echo "📦 Executável ARMv6: ❌ Não compilado"; \
	fi

# Atalhos convenientes
configure: setup
compile: build
install: deploy
status: info
verify: check
armv6: setup-armv6 build-armv6
