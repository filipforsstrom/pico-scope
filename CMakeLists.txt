cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PROJECT_NAME pico-scope)

project(${PROJECT_NAME} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT_NAME} src/picow_http_client.c )

pico_set_program_name(${PROJECT_NAME} "pico-scope")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
    pico_cyw43_arch_none
    # pico_cyw43_arch_lwip_threadsafe_background
    )

pico_add_extra_outputs(${PROJECT_NAME})

# Add the new targets and libraries from the other CMake file
pico_add_library(example_lwip_http_util NOFLAG)
target_sources(example_lwip_http_util INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/example_http_client_util.c
    )
pico_mirrored_target_link_libraries(example_lwip_http_util INTERFACE
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
    )
target_include_directories(example_lwip_http_util INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src
    )

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/.. # for our common lwipopts and mbedtls_config
    )
target_link_libraries(${PROJECT_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    example_lwip_http_util
    pico_stdlib
    )
pico_add_extra_outputs(${PROJECT_NAME})

add_executable(picow_http_client_verify
    src/picow_http_verify.c
    )
target_compile_definitions(picow_http_client_verify PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    # By default verification is optional (MBEDTLS_SSL_VERIFY_OPTIONAL)
    # Make it required for this test
    ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_REQUIRED
    )
target_include_directories(picow_http_client_verify PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/.. # for our common lwipopts and mbedtls_config
    )
target_link_libraries(picow_http_client_verify
    pico_cyw43_arch_lwip_threadsafe_background
    example_lwip_http_util
    pico_stdlib
    )
pico_add_extra_outputs(picow_http_client_verify)

# Ignore warnings from lwip code
set_source_files_properties(
    ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
    PROPERTIES
    COMPILE_OPTIONS "-Wno-unused-result"
    )

find_program(CPPCHECK cppcheck)
if (CPPCHECK)
    add_custom_target(
        cppcheck
        COMMAND cppcheck --enable=all --inconclusive --quiet --project=${CMAKE_BINARY_DIR}/compile_commands.json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis"
    )
endif()