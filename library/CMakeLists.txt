cmake_minimum_required(VERSION 3.21)

if(ADD_HEADERS_TO_PROJECT)
    file(GLOB_RECURSE SRC includes/*.hpp)

    add_library(TitanLib INTERFACE ${SRC})
else()
    add_library(TitanLib INTERFACE)
endif()

target_include_directories(TitanLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/includes/)
