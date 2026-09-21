cmake_minimum_required(VERSION 3.20)
project(auto_save_plugin LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(auto_save_plugin SHARED
    auto_save_plugin.c
)

target_include_directories(auto_save_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

if (WIN32)
    set_target_properties(auto_save_plugin PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif (APPLE)
    set_target_properties(auto_save_plugin PROPERTIES
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    set_target_properties(auto_save_plugin PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()
