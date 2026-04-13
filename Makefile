# Makefile para o projeto NovaTherm DataLogger (ARMv7 only)

.PHONY: help setup check configure-lcd configure-lvgl configure-st7567 build build-lcd build-lvgl build-st7567 deploy deploy-lcd deploy-lvgl deploy-st7567 test info clean rebuild

# Configuracoes
BUILD_DIR = build-rpi
APP_NAME = app_armv7
RPI_IP ?= 192.168.3.22
RPI_USER ?= nova

help:
	@echo "=== NovaTherm DataLogger - Comandos Disponiveis ==="
	@echo ""
	@echo "Configuracao de ambiente:"
	@echo "  make setup           - Instala/compila deps ARMv7 e configura CMake (LCD padrao)"
	@echo "  make check           - Verifica ambiente"
	@echo ""
	@echo "Perfis de display (sempre em $(BUILD_DIR)):"
	@echo "  make configure-lcd   - LCD 20x4 I2C (padrao)"
	@echo "  make configure-lvgl  - ILI9341 + LVGL"
	@echo "  make configure-st7567- ST7567 128x64"
	@echo ""
	@echo "Compilacao:"
	@echo "  make build           - Compila perfil atualmente configurado em $(BUILD_DIR)"
	@echo "  make build-lcd       - Reconfigura para LCD e compila"
	@echo "  make build-lvgl      - Reconfigura para LVGL e compila"
	@echo "  make build-st7567    - Reconfigura para ST7567 e compila"
	@echo ""
	@echo "Deploy:"
	@echo "  make deploy          - Envia $(BUILD_DIR)/bin/$(APP_NAME)"
	@echo "  make deploy-lcd      - build-lcd + deploy"
	@echo "  make deploy-lvgl     - build-lvgl + deploy"
	@echo "  make deploy-st7567   - build-st7567 + deploy"
	@echo "  make deploy RPI_IP=<ip> RPI_USER=<user>"
	@echo ""
	@echo "Outros:"
	@echo "  make info            - Status de deps/binario"
	@echo "  make clean           - Remove $(BUILD_DIR)"
	@echo "  make rebuild         - clean + setup + build"

setup:
	@echo "Configurando ambiente de cross-compilation ARMv7..."
	@chmod +x scripts/setup_cross_compilation.sh
	@./scripts/setup_cross_compilation.sh
	@$(MAKE) configure-lcd

check:
	@echo "Verificando ambiente..."
	@chmod +x scripts/check_cross_compilation.sh
	@./scripts/check_cross_compilation.sh

configure-lcd:
	@echo "Configurando CMake para LCD 20x4 em $(BUILD_DIR)..."
	@cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake \
		-DUSE_LVGL_ILI9341=OFF -DUSE_ST7567=OFF -DUSE_LCD_I2C=ON \
		-B $(BUILD_DIR) -S .

configure-lvgl:
	@echo "Configurando CMake para ILI9341 + LVGL em $(BUILD_DIR)..."
	@cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake \
		-DUSE_LVGL_ILI9341=ON -DUSE_ST7567=OFF -DUSE_LCD_I2C=OFF \
		-B $(BUILD_DIR) -S .

configure-st7567:
	@echo "Configurando CMake para ST7567 em $(BUILD_DIR)..."
	@cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake \
		-DUSE_LVGL_ILI9341=OFF -DUSE_ST7567=ON -DUSE_LCD_I2C=OFF \
		-B $(BUILD_DIR) -S .

build:
	@echo "Compilando projeto ARMv7 em $(BUILD_DIR)..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "Diretorio de build nao existe. Execute 'make configure-lcd' (padrao) ou outro perfil."; \
		exit 1; \
	fi
	@make -C $(BUILD_DIR) -j$$(nproc)
	@echo "Compilacao concluida: $(BUILD_DIR)/bin/$(APP_NAME)"

build-lcd: configure-lcd build

build-lvgl: configure-lvgl build

build-st7567: configure-st7567 build

deploy:
	@echo "Fazendo deploy ARMv7 para $(RPI_USER)@$(RPI_IP)..."
	@if [ ! -f "$(BUILD_DIR)/bin/$(APP_NAME)" ]; then \
		echo "Executavel nao encontrado. Execute 'make build' primeiro."; \
		exit 1; \
	fi
	@scp $(BUILD_DIR)/bin/$(APP_NAME) $(RPI_USER)@$(RPI_IP):~/$(APP_NAME)
	@echo "Deploy concluido: ~/$(APP_NAME)"

deploy-lcd: build-lcd deploy

deploy-lvgl: build-lvgl deploy

deploy-st7567: build-st7567 deploy

test: check
	@echo "Executando testes basicos..."
	@if [ -f "$(BUILD_DIR)/bin/$(APP_NAME)" ]; then \
		echo "Executavel existe"; \
		file $(BUILD_DIR)/bin/$(APP_NAME); \
		ls -lh $(BUILD_DIR)/bin/$(APP_NAME); \
	else \
		echo "Executavel nao encontrado"; \
		exit 1; \
	fi

clean:
	@echo "Limpando diretorio de build..."
	@if [ -d "$(BUILD_DIR)" ]; then rm -rf $(BUILD_DIR); fi
	@echo "Build removido"

rebuild: clean setup build

info:
	@echo "=== Informacoes do Projeto NovaTherm (ARMv7) ==="
	@echo "Alvo: Raspberry Pi Zero 2W (ARMv7/hard-float)"
	@echo "Build dir padrao: $(BUILD_DIR)"
	@echo ""
	@echo "Dependencias estaticas:"
	@echo "  libmodbus: $(shell [ -f deps/libmodbus/install/lib/libmodbus.a ] && echo 'OK' || echo 'MISSING')"
	@echo "  libgpiod:  $(shell [ -f deps/libgpiod/install/lib/libgpiod.a ] && echo 'OK' || echo 'MISSING')"
	@echo "  libudev:   $(shell [ -f deps/eudev/install/lib/libudev.a ] && echo 'OK' || echo 'MISSING')"
	@echo "  sqlite3:   $(shell [ -f deps/sqlite3/install/lib/libsqlite3.a ] && echo 'OK' || echo 'MISSING')"
	@echo ""
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "Perfil de display no cache atual:"; \
		grep -E "USE_LCD_I2C:BOOL|USE_LVGL_ILI9341:BOOL|USE_ST7567:BOOL" $(BUILD_DIR)/CMakeCache.txt || true; \
		echo ""; \
	fi
	@if [ -f "$(BUILD_DIR)/bin/$(APP_NAME)" ]; then \
		echo "Executavel: $(BUILD_DIR)/bin/$(APP_NAME)"; \
		echo "Tamanho: $$(ls -lh $(BUILD_DIR)/bin/$(APP_NAME) | awk '{print $$5}')"; \
		echo "Arquitetura: $$(file $(BUILD_DIR)/bin/$(APP_NAME) | cut -d: -f2)"; \
		echo "Dependencias dinamicas (NEEDED):"; \
		arm-linux-gnueabihf-readelf -d $(BUILD_DIR)/bin/$(APP_NAME) 2>/dev/null | grep NEEDED || echo "  nenhuma"; \
	else \
		echo "Executavel ainda nao compilado"; \
	fi
