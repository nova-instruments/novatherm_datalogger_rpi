cmake_minimum_required(VERSION 3.16)
project(modbus_reader C)

# Projeto exclusivo para build ARM (cross-compilation).
# Evita configuração acidental de build local x86/x64.
if(NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR
        "Build local desabilitado. Use cross-compilation ARM com: "
        "-DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake")
endif()

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv6l|armv7l)$")
    message(FATAL_ERROR
        "Arquitetura alvo invalida: ${CMAKE_SYSTEM_PROCESSOR}. "
        "Este projeto aceita apenas alvo ARM.")
endif()

# Configurar paths das dependências ARMv7
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(LIBMODBUS_DIR "${DEPS_DIR}/libmodbus/install")
set(LIBGPIOD_DIR "${DEPS_DIR}/libgpiod/install")
set(LIBUDEV_DIR "${DEPS_DIR}/eudev/install")
set(SQLITE3_DIR "${DEPS_DIR}/sqlite3/install")

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
    ${CMAKE_SOURCE_DIR}/lib
)

# Configurações do projeto
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opção para escolher o tipo de display
option(USE_LCD_I2C "Usar LCD 20x4 I2C ao invés de OLED SSD1306" ON)
option(USE_ST7789_EC11 "Usar display ST7789 + LVGL + EC11" OFF)

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

# Biblioteca de display (LCD 20x4, OLED SSD1306 ou ST7789+LVGL)
if(USE_ST7789_EC11)
    set(DISPLAY_ST7789_EC11_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(LV_BUILD_CONF_PATH "${CMAKE_SOURCE_DIR}/display_st7789_ec11_rpi/config/lv_conf.h" CACHE PATH "" FORCE)
    add_subdirectory(display_st7789_ec11_rpi)
    add_library(display_lib STATIC
        lib/display_st7789_lvgl.c
        lib/assets/bg_ihm.c
        lib/assets/bg_ihm_2.c
        lib/assets/logo_320x240.c
    )
    target_link_libraries(display_lib PUBLIC display_st7789_ec11)
    include_directories(${CMAKE_SOURCE_DIR}/display_st7789_ec11_rpi/include)
    add_definitions(-DUSE_ST7789_EC11)
    message(STATUS "Display selecionado: ST7789 + LVGL + EC11")
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

target_link_libraries(app_armv7 ${COMMON_LIBS})

# Informações do projeto
message(STATUS "=== Configuração do Projeto Modbus Reader ARMv7 ===")
message(STATUS "Compilador C: ${CMAKE_C_COMPILER}")
message(STATUS "Sistema alvo: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Cross-compiling: ${CMAKE_CROSSCOMPILING}")
message(STATUS "Diretório de dependências: ${DEPS_DIR}")
message(STATUS "Executável será gerado em: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/app_armv7")
message(STATUS "============================================")
