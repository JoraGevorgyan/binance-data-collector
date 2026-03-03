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

	BOOST_TEST(config->isHelp());
	BOOST_TEST(config->isDebugMode());
}
