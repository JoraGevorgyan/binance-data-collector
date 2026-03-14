add_compile_definitions(
	APP_NAME="${APP_NAME}"
)

include(GNUInstallDirs)

set(SYSTEMD_UNIT_DIR "/etc/systemd/system" CACHE PATH "Directory for systemd unit files")
set(SERVICE_CONFIG_DIR "/etc/binance-data-collector" CACHE PATH "Directory for service configuration files")

include(../sources.cmake)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/binance-data-collector.service.in
	${CMAKE_CURRENT_BINARY_DIR}/${NAME_SERVICE}.service
	@ONLY
)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

install(
	TARGETS ${EXECUTABLE_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
	FILES ${CMAKE_SOURCE_DIR}/config.json
	DESTINATION ${SERVICE_CONFIG_DIR}
)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME_SERVICE}.service
	DESTINATION ${SYSTEMD_UNIT_DIR}
)

install(CODE "message(STATUS \"Run 'systemctl daemon-reload && systemctl enable --now ${NAME_SERVICE}' to activate service\")")

target_link_libraries(
	${EXECUTABLE_NAME}
	PRIVATE
		Service
		Config
        ${LOGGER_NAME}
)
