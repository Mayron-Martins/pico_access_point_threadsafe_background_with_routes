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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_access_point_with_routes C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pico_access_point_with_routes 
    main.c
    dhcpserver/dhcpserver.c
    dnsserver/dnsserver.c
    src/http_server.c
    src/http_utils.c
    src/routes.c
    src/setup.c 
)

pico_set_program_name(pico_access_point_with_routes "pico_access_point_with_routes")
pico_set_program_version(pico_access_point_with_routes "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico_access_point_with_routes 0)
pico_enable_stdio_usb(pico_access_point_with_routes 1)

# Add the standard library to the build
target_link_libraries(pico_access_point_with_routes
        pico_stdlib
        pico_cyw43_arch_lwip_poll
        hardware_uart
)

# Add the standard include files to the build
target_include_directories(pico_access_point_with_routes PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/src
)

pico_add_extra_outputs(pico_access_point_with_routes)

# Static IP of the Access Point (for CYW43)
pico_configure_ip4_address(pico_access_point_with_routes PRIVATE CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1)

target_compile_definitions(pico_access_point_with_routes PRIVATE 
    PICO_CYW43_ARCH_POLL=1
    CYW43_LWIP=1
    LWIP_IPV4=1
    LWIP_DHCP=1
    NO_SYS=1
)