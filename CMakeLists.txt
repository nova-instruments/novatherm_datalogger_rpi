cmake_minimum_required(VERSION 3.16)
project(modbus_reader C)

# Configurar paths das dependências ARMv7
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(LIBMODBUS_DIR "${DEPS_DIR}/libmodbus/install")
set(LIBGPIOD_DIR "${DEPS_DIR}/libgpiod/install")
set(LIBUDEV_DIR "${DEPS_DIR}/eudev/install")
set(SQLITE3_DIR "${DEPS_DIR}/sqlite3/install")
set(LVGL_DIR "${DEPS_DIR}/lvgl/install")

# Verificar se as bibliotecas existem
if(NOT EXISTS "${LIBMODBUS_DIR}/lib/libmodbus.a")
    message(WARNING "libmodbus não encontrada em ${LIBMODBUS_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libmodbus_arm.sh")
endif()

if(NOT EXISTS "${LIBGPIOD_DIR}/lib/libgpiod.a")
    message(WARNING "libgpiod não encontrada em ${LIBGPIOD_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libgpiod_arm.sh")
endif()

if(NOT EXISTS "${LIBUDEV_DIR}/lib/libudev.a")
    message(WARNING "libudev não encontrada em ${LIBUDEV_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libudev_arm.sh")
endif()

if(NOT EXISTS "${SQLITE3_DIR}/lib/libsqlite3.a")
    message(WARNING "SQLite3 não encontrada em ${SQLITE3_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_sqlite3_arm.sh")
endif()

# Include directories
include_directories(
    ${LIBMODBUS_DIR}/include
    ${LIBGPIOD_DIR}/include
    ${LIBUDEV_DIR}/include
    ${SQLITE3_DIR}/include
    ${LVGL_DIR}/include
    ${LVGL_DIR}/include/lvgl
    ${CMAKE_SOURCE_DIR}/lib
)

# Configurações do projeto
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opção para escolher o tipo de display
option(USE_LCD_I2C "Usar LCD 20x4 I2C ao invés de OLED SSD1306" ON)
option(USE_ST7567 "Usar ST7567 128x64 COG LCD" OFF)
option(USE_LVGL_ILI9341 "Usar LVGL v9.x com ILI9341 240x320 TFT SPI" OFF)

# Configurar diretório de saída
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Bibliotecas do projeto
add_library(modbus_lib STATIC lib/modbus.c)
add_library(datalogger_lib STATIC lib/datalogger.c)
add_library(usb_manager STATIC lib/usb_manager.c)
add_library(relay_control STATIC lib/relay_control.c)
add_library(keyboard STATIC lib/keyboard.c)
add_library(input STATIC lib/input.c)
add_library(controller STATIC lib/controller.c)

# Biblioteca de display (LVGL+ILI9341, ST7567, LCD 20x4 ou OLED)
if(USE_LVGL_ILI9341)
    # Verificar se LVGL foi compilado
    if(NOT EXISTS "${LVGL_DIR}/lib/liblvgl.a")
        message(FATAL_ERROR "LVGL não encontrado em ${LVGL_DIR}/lib/liblvgl.a")
    endif()

    add_library(ili9341_lib STATIC lib/ili9341_spi.c)
    add_library(display_lib STATIC lib/display_lvgl.c)
    target_link_libraries(display_lib ili9341_lib)
    add_definitions(-DUSE_LVGL_ILI9341)
    add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
    message(STATUS "Display selecionado: LVGL v9.x + ILI9341 240x320 TFT SPI")
    message(STATUS "  GPIO_DC=22, GPIO_RST=27")
elseif(USE_ST7567)
    add_library(display_lib STATIC lib/lcd_st7567.c)
    add_definitions(-DUSE_ST7567)
    message(STATUS "Display selecionado: ST7567 128x64 COG LCD (0x3F)")
elseif(USE_LCD_I2C)
    add_library(display_lib STATIC lib/lcd_i2c.c)
    add_definitions(-DUSE_LCD_I2C)
    message(STATUS "Display selecionado: LCD 20x4 I2C (0x27)")
else()
    add_library(display_lib STATIC lib/oled_ssd1306.c)
    message(STATUS "Display selecionado: OLED SSD1306 (0x3C)")
endif()

# Executável principal
add_executable(app_armv7 src/main.c)

# Forçar linkagem estática das bibliotecas customizadas
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Linking das bibliotecas
set(COMMON_LIBS
    modbus_lib
    datalogger_lib
    usb_manager
    relay_control
    keyboard
    input
    display_lib
    controller
    ${LIBMODBUS_DIR}/lib/libmodbus.a
    ${LIBGPIOD_DIR}/lib/libgpiod.a
    ${LIBUDEV_DIR}/lib/libudev.a
    ${SQLITE3_DIR}/lib/libsqlite3.a
    pthread
    dl
    m
)

# Adicionar LVGL se estiver habilitado
if(USE_LVGL_ILI9341)
    list(APPEND COMMON_LIBS
        ${LVGL_DIR}/lib/liblvgl.a
        ili9341_lib
    )
endif()

target_link_libraries(app_armv7 ${COMMON_LIBS})

# Informações do projeto
message(STATUS "=== Configuração do Projeto Modbus Reader ARMv7 ===")
message(STATUS "Compilador C: ${CMAKE_C_COMPILER}")
message(STATUS "Sistema alvo: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Cross-compiling: ${CMAKE_CROSSCOMPILING}")
message(STATUS "Diretório de dependências: ${DEPS_DIR}")
message(STATUS "Executável será gerado em: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/app_armv7")
message(STATUS "============================================")
