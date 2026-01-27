# Usage:
# cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build -S .
# make  -C build -j

# Configuration for Raspberry Pi 3 cross-compilation
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Explicitly set cross-compiling flag
set(CMAKE_CROSSCOMPILING TRUE)

# Set the cross-compilation toolchain (Ubuntu packages)
set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)

# Set the target environment for the cross-compiler to look for libraries
set(CMAKE_FIND_ROOT_PATH
    /usr/arm-linux-gnueabihf
    ${CMAKE_SOURCE_DIR}/deps/libmodbus/install
    ${CMAKE_SOURCE_DIR}/deps/libgpiod/install
    ${CMAKE_SOURCE_DIR}/deps/eudev/install
    ${CMAKE_SOURCE_DIR}/deps/sqlite3/install
)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Raspberry Pi 3 specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard")

# For cross-compilation, we'll manually handle library linking
# Disable automatic pkg-config dependency resolution
set(PKG_CONFIG_EXECUTABLE "")

# Configurações adicionais para linking
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")

# Informações de debug
message(STATUS "Cross-compilation configurada para Raspberry Pi 3")
message(STATUS "Toolchain: arm-linux-gnueabihf")
message(STATUS "CPU: Cortex-A53 com NEON")

