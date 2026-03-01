option(RUN_CODE_COVERAGE "Enable code coverage reporting" OFF)

if (RUN_CODE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Enabling code coverage for ${CMAKE_CXX_COMPILER_ID}")
        add_compile_options(--coverage)
        add_link_options(--coverage)

        find_program(GRCOV grcov HINTS /usr/bin /usr/local/bin)
        if (GRCOV)
            message(STATUS "Found grcov: ${GRCOV}")
            set(COVERAGE_TOOL ${GRCOV})

            function(func_run_coverage TEST_PROGRAM MODULE)
                set(COVERAGE_TOOL_ARGS
                    "--llvm"
                    "--source-dir" "${CMAKE_SOURCE_DIR}"
                    "--ignore=*/test/*"
                    "--ignore=thirdparty/*"
                    "--threads=3"
                    "-t" "html"
                    "-o" "CoverageReport/${MODULE}"
                )
                add_custom_command(
                    TARGET ${TEST_PROGRAM}
                    POST_BUILD
                    COMMAND bash -c './${TEST_PROGRAM}\; ${COVERAGE_TOOL} ${COVERAGE_TOOL_ARGS} ${CMAKE_CURRENT_BINARY_DIR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating code coverage report for ${TEST_PROGRAM} using ${COVERAGE_TOOL}"
                    USES_TERMINAL
                )
            endfunction()
        else()
            message(ERROR "grcov not found. run 'cargo install grcov' to install")
        endif()
    else()
        message(WARNING "Code coverage is only supported with Clang. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()
