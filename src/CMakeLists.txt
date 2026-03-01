include(lang-options.cmake)
include(tests/testing.cmake)
include(../cmake/paths_and_names.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

option(RUN_CLANG_TIDY "run clang-tidy on all sources" ON)
option(RUN_CLANG_FORMAT "run clang-format on all sources" ON)

if (RUN_CLANG_TIDY)
    find_program(CLANG_TIDY "clang-tidy")
    if (CLANG_TIDY)
        message(found clang-tidy)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
    else()
        message(warning "clang-tidy not found, skipping")
    endif()
endif()

if (RUN_CLANG_FORMAT)
    find_program(CLANG_FORMAT "clang-format")
    if (CLANG_FORMAT)
        message(found clang-format)
        set(CMAKE_CXX_CLANG_FORMAT ${CLANG_FORMAT})
    else()
        message(warning "clang-format not found, skipping")
    endif()
endif()

add_subdirectory(binance-data-collector)
add_subdirectory(Service)


# not supported yet
#include(cpack-config.cmake)
