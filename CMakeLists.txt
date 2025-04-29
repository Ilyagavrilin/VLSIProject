cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

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



FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)


set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# Clang compile commands are generated here:
# build/Debug/compile_commands.json
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME} src/main.cpp)
add_compile_options(-Wall -g)
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
