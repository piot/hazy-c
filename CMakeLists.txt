cmake_minimum_required(VERSION 3.16.3)
project(hazy C)

add_subdirectory(deps/piot/clog/src/lib)
add_subdirectory(deps/piot/datagram-transport-c/src/lib)
add_subdirectory(deps/piot/discoid-c/src/lib)
add_subdirectory(deps/piot/imprint/src/lib)
add_subdirectory(deps/piot/monotonic-time-c/src/lib)
add_subdirectory(deps/piot/tiny-libc/src/lib)


add_subdirectory(src)
