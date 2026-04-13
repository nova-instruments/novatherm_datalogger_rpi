cmake_minimum_required(VERSION 3.16)
project(lcd_examples C)

# Configurar paths das dependências ARMv7
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/../deps")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../lib
)

# Configurações do projeto
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configurar diretório de saída
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Biblioteca LCD
add_library(lcd_i2c STATIC ../lib/lcd_i2c.c)

# Executável de demonstração de barras
add_executable(lcd_bargraph_demo lcd_bargraph_demo.c)

# Linking
target_link_libraries(lcd_bargraph_demo
    lcd_i2c
    m
)

# Informações do projeto
message(STATUS "=== Exemplos LCD NovaTherm ===")
message(STATUS "Compilador C: ${CMAKE_C_COMPILER}")
message(STATUS "Executáveis serão gerados em: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "==============================")

