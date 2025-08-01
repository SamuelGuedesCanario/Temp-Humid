# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(estacao_meteorologica C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

include_directories( ${CMAKE_SOURCE_DIR}/lib ) # Inclui os files .h na pasta lib

add_executable( estacao_meteorologica 
        estacao_meteorologica.c 
        lib/aht20.c 
        lib/bmp280.c 
        lib/ssd1306.c
        )

# Generate PIO header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_set_program_name(estacao_meteorologica "estacao_meteorologica")
pico_set_program_version(estacao_meteorologica "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(estacao_meteorologica 0)
pico_enable_stdio_usb(estacao_meteorologica 1)

# Add the standard library to the build
target_link_libraries(estacao_meteorologica
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(estacao_meteorologica PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(estacao_meteorologica 
        hardware_i2c
        hardware_pwm
        hardware_timer
        hardware_pio
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(estacao_meteorologica)

