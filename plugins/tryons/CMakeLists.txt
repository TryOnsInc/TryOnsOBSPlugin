cmake_minimum_required(VERSION 3.10)
project(tryons)

set(PLUGIN_NAME "tryons")
set(PLUGIN_MODULE_NAME "tryons")

add_library(${PLUGIN_MODULE_NAME} MODULE
    tryons_module.c
    tryons_network.c
    tryons_text.c
    tryons_audio.c
)
target_include_directories(${PLUGIN_MODULE_NAME} PRIVATE 
    ${OBS_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/../../frontend/api"
)

target_link_libraries(
  ${PLUGIN_MODULE_NAME}
  PRIVATE
    OBS::libobs
    OBS::frontend-api
)