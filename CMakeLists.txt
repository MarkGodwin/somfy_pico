# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "./pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico-sdk/pico_sdk_init.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(somfy_remote C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(somfy_remote 
  somfy_remote.cpp
  radio.cpp
  remotes.cpp
  remote.cpp
  blinds.cpp
  blind.cpp
  scheduler.cpp
  webInterface.cpp
  wifiConnection.cpp
  wifiScanner.cpp
  blockStorage.cpp
  deviceConfig.cpp
  mqttClient.cpp
  dhcpserver/dhcpserver.c
  dnsserver/dnsserver.c
)

pico_set_program_name(somfy_remote "somfy_remote")
pico_set_program_version(somfy_remote "0.1")

pico_enable_stdio_uart(somfy_remote 0)
pico_enable_stdio_usb(somfy_remote 1)

# Add the standard library to the build
target_link_libraries(somfy_remote
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(somfy_remote PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
  ${CMAKE_CURRENT_LIST_DIR}/dnsserver
  #  ${CMAKE_CURRENT_LIST_DIR}/.. 
)

# Add any user requested libraries
target_link_libraries(somfy_remote 
        hardware_spi
        hardware_pio
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_lwip_mqtt
        pico_flash
        )
        
pico_add_extra_outputs(somfy_remote)

