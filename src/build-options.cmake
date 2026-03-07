# this needs to be imporved yet, but yet, for now we use clang only:D
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
			"-Wall"
			"-Wextra"
			"-g3")
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(
			"-Wall"
			"-Wextra"
			"-Werror"
			"-O3"
			"-flto"
        )
        add_link_options(-flto)
    endif()
endif()
