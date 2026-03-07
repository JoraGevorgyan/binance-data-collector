include(../sources.cmake)

set(NAME DataCollector)

add_library(${NAME} ${SOURCES} ${HEADERS})

target_include_directories(
	${NAME}
	PRIVATE
	"${CMAKE_SOURCE_DIR}/thirdparty/nlohmann"
)


target_link_libraries(
	${NAME}
	PRIVATE
		common
		Config
		Boost::system
		OpenSSL::SSL
		OpenSSL::Crypto
		${LOGGER_NAME}
)

