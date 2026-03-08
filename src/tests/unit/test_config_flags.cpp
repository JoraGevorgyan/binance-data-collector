#include <boost/test/unit_test.hpp>
#include "Config/Config.hpp"

BOOST_AUTO_TEST_CASE(config_parses_help_and_debug_flags) {
	char program_name[] = "config-flags-test";
	char help_flag[] = "--help";
	char debug_flag[] = "--debug";
	char* argv[] = {program_name, help_flag, debug_flag};
	constexpr int argc = static_cast<int>(sizeof(argv) / sizeof(argv[0]));

	auto& config = Config::Config::getInstance();
	BOOST_REQUIRE(config != nullptr);
	BOOST_REQUIRE(config->init(argc, argv));
	config->updateConfig();

	BOOST_TEST(config->isHelp());
	BOOST_TEST(config->isDebugMode());
	BOOST_TEST(config->getServiceName() == "binance-data-collector-service");
	BOOST_TEST(config->getServiceDisplayName() ==
	           "Binance Data Collector Service");

	BOOST_TEST(config->getStatsOutputPath() ==
	           "/var/log/binance-data-collector/statistics.log");
	BOOST_TEST(!config->getHostName().empty());
	BOOST_TEST(config->getPort() == "9443");

	const auto streams = config->getStreamsList();
	BOOST_TEST(streams.size() == 3u);
	BOOST_TEST(streams[0] == "btcusdt@trade");
	BOOST_TEST(streams[1] == "ethusdt@trade");
	BOOST_TEST(streams[2] == "bnbusdt@trade");
}
