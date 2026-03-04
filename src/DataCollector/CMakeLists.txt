include(../sources.cmake)

set(NAME DataCollector)

add_library(${NAME} ${SOURCES} ${HEADERS})

target_link_libraries(
	${NAME}
	PRIVATE
		common
		Config
		Boost::system
		OpenSSL::SSL
		OpenSSL::Crypto
		nlohmann_json::nlohmann_json
		${LOGGER_NAME}
)

