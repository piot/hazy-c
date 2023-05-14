cmake_minimum_required(VERSION 3.17)
project(hazy C)

set(CMAKE_C_STANDARD 99)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else ()
    message("no debug")
    set(isDebug FALSE)
endif ()

if (isDebug)
    message("Debug build detected")
    add_compile_definitions(CONFIGURATION_DEBUG TYRAN_CONFIGURATION_DEBUG)
    set(CONFIGURATION_DEBUG 1)
endif ()


set(deps ../deps/)

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "${deps}piot/*/src/lib/*.c"
        "${deps}piot/*/src/platform/posix/*.c"
        )

file(GLOB_RECURSE lib_src FOLLOW_SYMLINKS
        "lib/*.c"
        )



add_executable(hazy_example
        ${deps_src}
        ${lib_src}
        examples/main.c
        )


target_include_directories(hazy_example PUBLIC ${deps}piot/tiny-libc/src/include)
target_include_directories(hazy_example PUBLIC ${deps}piot/datagram-transport-c/src/include)
target_include_directories(hazy_example PUBLIC ${deps}piot/monotonic-time-c/src/include)
target_include_directories(hazy_example PUBLIC ${deps}piot/clog/src/include)
target_include_directories(hazy_example PUBLIC ${deps}piot/discoid-c/src/include)
target_include_directories(hazy_example PUBLIC ${deps}piot/imprint/src/include)

target_include_directories(hazy_example PUBLIC include)





if (OS_WINDOWS)
    target_compile_options(hazy_example PRIVATE -Wall -pedantic -W4)
else ()
    target_compile_options(hazy_example PRIVATE -Wall -Wextra -Wshadow -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Werror=implicit-function-declaration)
endif ()

target_link_libraries(hazy_example m)
