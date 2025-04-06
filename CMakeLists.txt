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

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# Clang compile commands are generated here:
# build/Debug/compile_commands.json
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()


add_executable(${PROJECT_NAME} src/main.cpp)
add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
