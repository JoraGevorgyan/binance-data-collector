include(../sources.cmake)

set(SERVICE_VERSION "${CMAKE_PROJECT_VERSION}")
set(NAME Service)

add_library(${NAME} ${SOURCES} ${HEADERS})

target_compile_definitions(
    ${NAME}
    PRIVATE
    SERVICE_VERSION="${SERVICE_VERSION}"
    NAME_SERVICE="${NAME_SERVICE}"
)

target_link_libraries(
    ${NAME}
    PRIVATE
    ${LOGGER_NAME}
)
