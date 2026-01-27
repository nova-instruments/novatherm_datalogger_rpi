# Usage:
# cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup_armv6.cmake -B build-rpi1 -S .
# make  -C build-rpi1 -j

# Configuration for Raspberry Pi 1 (ARMv6) cross-compilation
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Explicitly set cross-compiling flag
set(CMAKE_CROSSCOMPILING TRUE)

# Set the cross-compilation toolchain (Ubuntu packages)
set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabi-g++)

# Set the target environment for the cross-compiler to look for libraries
set(CMAKE_FIND_ROOT_PATH
    /usr/arm-linux-gnueabi
    ${CMAKE_SOURCE_DIR}/deps-armv6/libmodbus/install
    ${CMAKE_SOURCE_DIR}/deps-armv6/libgpiod/install
    ${CMAKE_SOURCE_DIR}/deps-armv6/eudev/install
    ${CMAKE_SOURCE_DIR}/deps-armv6/sqlite3/install
)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Raspberry Pi 1 specific flags (ARMv6 with soft-float)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfloat-abi=soft")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfloat-abi=soft")

# For cross-compilation, we'll manually handle library linking
# Disable automatic pkg-config dependency resolution
set(PKG_CONFIG_EXECUTABLE "")

# Configurações adicionais para linking estático
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Informações de debug
message(STATUS "Cross-compilation configurada para Raspberry Pi 1 (ARMv6)")
message(STATUS "Toolchain: arm-linux-gnueabi")
message(STATUS "CPU: ARMv6 com soft-float")
