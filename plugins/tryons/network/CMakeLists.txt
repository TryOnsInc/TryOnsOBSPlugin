cmake_minimum_required(VERSION 3.28...3.30)
project(tryons_network)

add_library(tryons_network STATIC
    tryons_network.c
    ../tryons_common.c
)

target_include_directories(tryons_network
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
