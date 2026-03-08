#include <boost/test/unit_test.hpp>
#include "Service/ServiceDataCollector.hpp"

namespace {

Config::Config& getInitializedConfig() {
	auto& config_ptr = Config::Config::getInstance();
	BOOST_REQUIRE(config_ptr != nullptr);

	static bool initialized = false;
	if (!initialized) {
		char program_name[] = "service-data-collector-test";
		char* argv[] = {program_name};
		constexpr int argc = static_cast<int>(sizeof(argv) / sizeof(argv[0]));

		BOOST_REQUIRE(config_ptr->init(argc, argv));
		config_ptr->updateConfig();
		initialized = true;
	}

	return *config_ptr;
}

} // namespace

BOOST_AUTO_TEST_CASE(service_data_collector_lifecycle_transitions) {
	auto& config = getInitializedConfig();
	Service::ServiceDataCollector service(config);

	BOOST_TEST(service.onStart());
	BOOST_TEST(service.onPause());
	BOOST_TEST(!service.onPause());

	BOOST_TEST(service.onResume());
	BOOST_TEST(!service.onResume());

	BOOST_TEST(service.onStop());
	BOOST_TEST(service.onShutdown());
	BOOST_TEST(service.onReload());
}
