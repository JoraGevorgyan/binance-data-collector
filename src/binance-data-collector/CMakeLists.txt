add_compile_definitions(
    APP_NAME="${APP_NAME}"
)

include(../sources.cmake)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(
    ${EXECUTABLE_NAME}
    PRIVATE
        MainService
)
