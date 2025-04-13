cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

message("Looking for compiler caching tool")
find_program(CCACHE "ccache")
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    message("${CCACHE} will be used as the compiler caching tool")
else()
    find_program(SCCACHE "sccache")
    if(SCCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
        message("${SCCACHE} will be used as the compiler caching tool")
    else()
        message("Neither ccache nor sccache have been found in the system. No compiler caching tool will be used")
    endif(SCCACHE)
endif(CCACHE)

# Project
set(PROJECT_DESCRIPTION "Project for the VLSI course")
project(
  "VLSIProject"
        DESCRIPTION ${PROJECT_DESCRIPTION}
)

# Find JSON library (nlohmann/json)
# Fristly try to find out installation
find_package(nlohmann_json 3.10.0)

# If not found, look for single file installation
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS ${CMAKE_SOURCE_DIR}/include
              ${CMAKE_SOURCE_DIR}/external
              /usr/include
              /usr/local/include
    )
    if(NLOHMANN_JSON_INCLUDE_DIR)
        message(STATUS "Found nlohmann/json header at: ${NLOHMANN_JSON_INCLUDE_DIR}")
        include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "nlohmann/json library not found. Please install it or provide the path.")
    endif()
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# Clang compile commands are generated here:
# build/Debug/compile_commands.json
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} src/main.cpp)
add_compile_options(-Wall -std=c++2a -g)
add_library(VG STATIC ${CMAKE_SOURCE_DIR}/src/BufferInsertVG.cpp)
add_library(JSON STATIC ${CMAKE_SOURCE_DIR}/src/JSONTools.cpp)
target_link_libraries(JSON PRIVATE nlohmann_json::nlohmann_json)

add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} VG JSON)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
