# Makefile para exemplos LCD
.PHONY: help setup build clean deploy

BUILD_DIR = build
RPI_IP ?= 192.168.3.99
RPI_USER ?= nova

help:
	@echo "=== LCD Examples - Comandos Disponíveis ==="
	@echo ""
	@echo "🔧 Configuração:"
	@echo "  make setup     - Configura ambiente de compilação"
	@echo ""
	@echo "🔨 Compilação:"
	@echo "  make build     - Compila os exemplos para ARM"
	@echo "  make clean     - Limpa arquivos de build"
	@echo ""
	@echo "🚀 Deploy:"
	@echo "  make deploy    - Envia para Raspberry Pi (IP=$(RPI_IP), USER=$(RPI_USER))"
	@echo ""

setup:
	@echo "🔧 Configurando ambiente..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_TOOLCHAIN_FILE=../../user_cross_compile_setup.cmake ..
	@echo "✅ Ambiente configurado!"

build:
	@echo "🔨 Compilando exemplos..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "⚠️  Diretório de build não existe. Execute 'make setup' primeiro."; \
		exit 1; \
	fi
	@make -C $(BUILD_DIR) -j$$(nproc)
	@echo "✅ Compilação concluída!"
	@echo "📁 Executáveis: $(BUILD_DIR)/bin/"

clean:
	@echo "🧹 Limpando arquivos de build..."
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf $(BUILD_DIR); \
		echo "✅ Diretório $(BUILD_DIR) removido"; \
	else \
		echo "ℹ️  Nada para limpar"; \
	fi

deploy:
	@echo "🚀 Fazendo deploy para Raspberry Pi..."
	@if [ ! -f "$(BUILD_DIR)/bin/lcd_bargraph_demo" ]; then \
		echo "❌ Executável não encontrado. Execute 'make build' primeiro."; \
		exit 1; \
	fi
	@echo "📤 Enviando arquivos para $(RPI_USER)@$(RPI_IP)..."
	@scp $(BUILD_DIR)/bin/lcd_bargraph_demo $(RPI_USER)@$(RPI_IP):~/lcd_bargraph_demo
	@echo "✅ Deploy concluído!"
	@echo ""
	@echo "Para executar no Raspberry Pi:"
	@echo "  ssh $(RPI_USER)@$(RPI_IP)"
	@echo "  sudo ./lcd_bargraph_demo"

