include(../sources.cmake)

set(NAME Config)

add_library(${NAME} ${SOURCES} ${HEADERS})

target_compile_definitions(
	${NAME}
	PRIVATE
	NAME_CONFIG="${NAME_CONFIG}"
)

target_link_libraries(
    ${NAME}
    PRIVATE
		${LOGGER_NAME}
		Boost::program_options
)

