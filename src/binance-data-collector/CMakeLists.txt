add_compile_definitions(
	APP_NAME="${APP_NAME}"
)

include(../sources.cmake)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
install(
	TARGETS ${EXECUTABLE_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
target_link_libraries(
	${EXECUTABLE_NAME}
	PRIVATE
		Service
		Config
        ${LOGGER_NAME}
)
