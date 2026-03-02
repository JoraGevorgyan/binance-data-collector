include(lang-options.cmake)
include(tests/testing.cmake)
include(../cmake/paths_and_names.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

option(RUN_CLANG_TIDY "run clang-tidy on all sources" ON)
option(CONFIGURE_CLANG_FORMAT "run clang-format on all sources" ON)

if (RUN_CLANG_TIDY)
    find_program(CLANG_TIDY "clang-tidy")
    if (CLANG_TIDY)
        message(STATUS "found clang-tidy")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
    else()
        message(WARNING "clang-tidy not found, skipping")
    endif()
endif()

if (CONFIGURE_CLANG_FORMAT)
    find_program(CLANG_FORMAT "clang-format")
    if (CLANG_FORMAT)
        message(STATUS "found clang-format")
        file(GLOB_RECURSE ALL_SOURCES "*.cpp" "*.hpp")
		add_custom_target(format_apply
			COMMAND ${CLANG_FORMAT} -i --style=file:${CMAKE_SOURCE_DIR}/.clang-format ${ALL_SOURCES}
			COMMENT "applying clang-format suggestions...")
		add_custom_target(format_check
			COMMAND ${CLANG_FORMAT} --dry-run --style=file:${CMAKE_SOURCE_DIR}/.clang-format ${ALL_SOURCES}
			COMMENT "checking with clang-format...")
    else()
        message(WARNING "clang-format not found, skipping")
    endif()
endif()

find_package(
	Boost 1.83 REQUIRED
	COMPONENTS
		program_options
		unit_test_framework
	)

add_subdirectory(binance-data-collector)
add_subdirectory(Service)
add_subdirectory(Config)

# not supported yet
#include(cpack-config.cmake)
