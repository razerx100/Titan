cmake_minimum_required(VERSION 3.21)

project(Titan
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ADD_TEST_TITAN "If test should be built" OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(ADD_HEADERS_TO_PROJECT TRUE)
endif()

add_subdirectory(library)

if(ADD_TEST_TITAN)
    enable_testing()
    add_subdirectory(test)
endif()

add_library(razer::titan ALIAS TitanLib)
