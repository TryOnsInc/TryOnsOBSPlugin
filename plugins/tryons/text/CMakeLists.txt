cmake_minimum_required(VERSION 3.28)

# If not inherited from top-level, find obs or set up environment...
project(TryonsTextPlugin)

add_library(tryons MODULE
    tryons.cpp
    # path back up to the shared file:
    ../tryons_common.c
)

target_include_directories(tryons
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}     # e.g. the text folder
        ${CMAKE_CURRENT_SOURCE_DIR}/..  # e.g. so that "tryons_common.h" is found
)

target_link_libraries(tryons PRIVATE # any libs #)

set_target_properties(tryons PROPERTIES FOLDER "plugins/tryons")
