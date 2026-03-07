enable_testing()

include(../cmake/code_coverage.cmake)

option(RUN_TESTS "Enable running tests" ON)

if (RUN_TESTS)
	set(UNIT_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR}/unit)
	set(UNIT_TESTS_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/tests)
	file(MAKE_DIRECTORY ${UNIT_TESTS_GEN_DIR})

	function(add_boost_unit_test TARGET_NAME MODULE_NAME)
		cmake_parse_arguments(TEST "" "" "SOURCES;LIBRARIES" ${ARGN})

		set(MODULE ${MODULE_NAME})
		configure_file(
			${CMAKE_CURRENT_LIST_DIR}/template.in
			${UNIT_TESTS_GEN_DIR}/${TARGET_NAME}_main.cpp
		)

		add_executable(
			${TARGET_NAME}
			${UNIT_TESTS_GEN_DIR}/${TARGET_NAME}_main.cpp
			${TEST_SOURCES}
		)

		target_link_libraries(
			${TARGET_NAME}
			PRIVATE
				Boost::unit_test_framework
				${TEST_LIBRARIES}
		)

		add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
	endfunction()

	add_boost_unit_test(
		ut_canceler
		CancelerTests
		SOURCES
			${UNIT_TESTS_DIR}/test_canceler.cpp
			${CMAKE_CURRENT_LIST_DIR}/../common/src/Canceler.cpp
	)

	add_boost_unit_test(
		ut_service_base
		ServiceBaseTests
		SOURCES
			${UNIT_TESTS_DIR}/test_service_base.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Service/src/ServiceBase.cpp
		LIBRARIES
			${LOGGER_NAME}
	)

	add_boost_unit_test(
		ut_config_flags
		ConfigFlagsTests
		SOURCES
			${UNIT_TESTS_DIR}/test_config_flags.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Config/src/Config.cpp
		LIBRARIES
			Boost::program_options
			${LOGGER_NAME}
	)

	add_boost_unit_test(
		ut_data_collector_aggregator
		DataCollectorAggregatorTests
		SOURCES
			${UNIT_TESTS_DIR}/test_data_collector_aggregator.cpp
			${CMAKE_CURRENT_LIST_DIR}/../DataCollector/src/Aggregator.cpp
			${CMAKE_CURRENT_LIST_DIR}/../common/src/Canceler.cpp
		LIBRARIES
			${LOGGER_NAME}
	)

	add_boost_unit_test(
		ut_service
		ServiceTests
		SOURCES
			${UNIT_TESTS_DIR}/test_service.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Service/src/Service.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Service/src/ServiceBase.cpp
		LIBRARIES
			${LOGGER_NAME}
	)

	add_boost_unit_test(
		ut_service_data_collector
		ServiceDataCollectorTests
		SOURCES
			${UNIT_TESTS_DIR}/test_service_data_collector.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Service/src/ServiceDataCollector.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Service/src/ServiceBase.cpp
			${CMAKE_CURRENT_LIST_DIR}/../DataCollector/src/DataCollector.cpp
			${CMAKE_CURRENT_LIST_DIR}/../DataCollector/src/Aggregator.cpp
			${CMAKE_CURRENT_LIST_DIR}/../Config/src/Config.cpp
			${CMAKE_CURRENT_LIST_DIR}/../common/src/Canceler.cpp
		LIBRARIES
			Boost::program_options
			Boost::system
			OpenSSL::SSL
			OpenSSL::Crypto
			${LOGGER_NAME}
	)
endif()
