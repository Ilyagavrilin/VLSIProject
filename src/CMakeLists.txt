# Get all the .cpp files in any projectFolder/src/dir directory
file(GLOB directories */)
foreach(dir ${directories})
    if(IS_DIRECTORY ${dir})
        string(FIND ${dir} "/" last_slash_pos REVERSE)
        math(EXPR string_start "${last_slash_pos}+1")
        string(SUBSTRING ${dir} ${string_start} -1 dir_stripped)
        file(GLOB_RECURSE sources ${dir_stripped}/*.cpp)
        list(APPEND sources_list ${sources})
    endif()
endforeach()

# Define the compiler flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMPILER_FLAGS
        -Wall
        -O0
    )
    if(UNIX)
        list(APPEND COMPILER_FLAGS
            -Werror
            -Wpedantic
            -Wextra
        )
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMPILER_FLAGS
        -DNDEBUG
    )
    if(UNIX)
        list(APPEND COMPILER_FLAGS
            -O3
            -march=native
            -ffast-math
        )
    endif()
endif()

# Define the preprocessor macros based on the build type.
# This is equivalent to using #define in the source code
# or using -D flags as a compiler option
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PREPROCESSOR_MACROS
        DEBUG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PREPROCESSOR_MACROS
        RELEASE
    )
endif()
if(LINUX)
    list(APPEND PREPROCESSOR_MACROS
        LINUX
    )
elseif(APPLE)
    list(APPEND PREPROCESSOR_MACROS
        MACOS
    )
elseif(WIN32)
    list(APPEND PREPROCESSOR_MACROS
        WINDOWS
    )
endif()

# Define the linker options based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LINKER_OPTIONS
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(LINKER_OPTIONS
    )
    if(UNIX)
        list(APPEND LINKER_OPTIONS
            -flto           # Enable link-time optimizations
        )
    endif()
    if(LINUX)
        list(APPEND LINKER_OPTIONS
            -Wl,--strip-all # Strip debug symbols
        )
    endif()
endif()
if(LINUX)
    list(APPEND LINKER_OPTIONS
        -Wl,--no-undefined # Fail if there are any unresolved symbols in the final linked output
    )
endif()




