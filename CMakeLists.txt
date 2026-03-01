cmake_minimum_required(VERSION 3.28)
include(src/build-number.cmake)
Project(binance-data-collector VERSION 1.0.${BUILD_NUMBER})

enable_testing()

include(src/build-options.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(thirdparty EXCLUDE_FROM_ALL)
add_subdirectory(src)
