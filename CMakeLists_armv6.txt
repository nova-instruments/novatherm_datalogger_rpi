cmake_minimum_required(VERSION 3.16)
project(modbus_reader C)

# Configurar paths das dependências ARMv6
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps-armv6")
set(LIBMODBUS_DIR "${DEPS_DIR}/libmodbus/install")
set(LIBGPIOD_DIR "${DEPS_DIR}/libgpiod/install")
set(LIBUDEV_DIR "${DEPS_DIR}/eudev/install")
set(SQLITE3_DIR "${DEPS_DIR}/sqlite3/install")

# Verificar se as bibliotecas existem
if(NOT EXISTS "${LIBMODBUS_DIR}/lib/libmodbus.so")
    message(WARNING "libmodbus não encontrada em ${LIBMODBUS_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libmodbus_armv6.sh")
endif()

if(NOT EXISTS "${LIBGPIOD_DIR}/lib/libgpiod.so")
    message(WARNING "libgpiod não encontrada em ${LIBGPIOD_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libgpiod_armv6.sh")
endif()

if(NOT EXISTS "${LIBUDEV_DIR}/lib/libudev.a")
    message(WARNING "libudev não encontrada em ${LIBUDEV_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_libudev_armv6.sh")
endif()

if(NOT EXISTS "${SQLITE3_DIR}/lib/libsqlite3.so")
    message(WARNING "SQLite3 não encontrada em ${SQLITE3_DIR}/lib/")
    message(WARNING "Execute: ./scripts/build_sqlite3_armv6.sh")
endif()

# Include directories
include_directories(
    ${LIBMODBUS_DIR}/include
    ${LIBGPIOD_DIR}/include
    ${LIBUDEV_DIR}/include
    ${SQLITE3_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Link directories
link_directories(
    ${LIBMODBUS_DIR}/lib
    ${LIBGPIOD_DIR}/lib
    ${LIBUDEV_DIR}/lib
    ${SQLITE3_DIR}/lib
)

# Configurações do projeto
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configurar diretório de saída
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Bibliotecas do projeto
add_library(modbus_lib STATIC lib/modbus.c)
add_library(datalogger_lib STATIC lib/datalogger.c)
add_library(usb_manager STATIC lib/usb_manager.c)
add_library(relay_control STATIC lib/relay_control.c)
add_library(reset_button STATIC lib/reset_button.c)
add_library(oled_ssd1306 STATIC lib/oled_ssd1306.c)

# Executável principal
add_executable(app_armv6 src/main.c)

# Linking das bibliotecas
target_link_libraries(app_armv6
    modbus_lib
    datalogger_lib
    usb_manager
    relay_control
    reset_button
    oled_ssd1306
    modbus
    gpiod
    udev
    sqlite3
    pthread
    dl
    m
)

# Informações do projeto
message(STATUS "=== Configuração do Projeto Modbus Reader ARMv6 ===")
message(STATUS "Compilador C: ${CMAKE_C_COMPILER}")
message(STATUS "Sistema alvo: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Cross-compiling: ${CMAKE_CROSSCOMPILING}")
message(STATUS "Diretório de dependências: ${DEPS_DIR}")
message(STATUS "Executável será gerado em: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/app")
message(STATUS "============================================")
