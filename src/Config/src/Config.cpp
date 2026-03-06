#include "../Config.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include "spdlog/sinks/basic_file_sink.h"

namespace Config {

namespace {

namespace Key {

constexpr auto log_path = "log-path";
constexpr auto log_level = "log-level";
constexpr auto debug = "debug";
constexpr auto config = "config";
constexpr auto help = "help";

} // namespace Key

constexpr auto g_service_name_c = "binance-data-collector-service";
constexpr auto g_service_display_name_c = "Binance Data Collector Service";
// TODO: add working dir in future and save all files there
constexpr auto g_def_stats_out_path_c =
    "/var/log/binance-data-collector/statistics.log";

std::string getLogPath(const po::variables_map& var_map) noexcept {
	return var_map[Key::log_path].as<std::string>();
}

std::string getConfigPath(const po::variables_map& var_map) noexcept {
	return var_map[Key::config].as<std::string>();
}

} // namespace

std::unique_ptr<Config> Config::m_instance = nullptr;

std::unique_ptr<Config>& Config::getInstance() {
	if (m_instance != nullptr) {
		return m_instance;
	}
	m_instance = std::unique_ptr<Config>(new Config());
	return m_instance;
}

spdlog::level::level_enum Config::getLogLevel() const noexcept {
	if (m_po_var_map.count(Key::log_level) > 0) { // CLI overrides config file
		const auto level = m_po_var_map[Key::log_level].as<std::string>();
		if (boost::iequals(level, "trace")) {
			return spdlog::level::trace;
		}
		if (boost::iequals(level, "debug")) {
			return spdlog::level::debug;
		}
		if (boost::iequals(level, "info")) {
			return spdlog::level::info;
		}
		if (boost::iequals(level, "warn")) {
			return spdlog::level::warn;
		}
		if (boost::iequals(level, "error")) {
			return spdlog::level::err;
		}
		if (boost::iequals(level, "critical")) {
			return spdlog::level::critical;
		}
		if (boost::iequals(level, "off")) {
			return spdlog::level::off;
		}
	}
	return m_log_level.value_or(spdlog::level::info);
}

bool Config::init(int argc, char* argv[]) noexcept {
	try {
		constexpr auto def_conf_p = "/etc/binance-data-collector/config.yaml";
		constexpr auto def_log_p = "/var/log/binance-data-collector/cur.log";
		m_po_desc.add_options()(Key::help, "produce help message")(
		    Key::config, po::value<std::string>()->default_value(def_conf_p),
		    "path to config file")(
		    Key::log_path, po::value<std::string>()->default_value(def_log_p),
		    "path to log file")(
		    Key::log_level, po::value<std::string>()->default_value("info"),
		    "log level (trace, debug, info, warn, error, critical, off)")(
		    Key::debug, po::bool_switch()->default_value(false),
		    "enable debug mode (overrides log level to debug)");

		po::store(po::parse_command_line(argc, argv, m_po_desc), m_po_var_map);

		po::notify(m_po_var_map);
	} catch (const po::error& err) {
		std::cerr << "Error parsing command line: " << err.what() << std::endl;
		return false;
	} catch (const std::exception& err) {
		std::cerr << "Unexpected error: " << err.what() << std::endl;
		return false;
	}
	return true;
}

bool Config::initLogger() const noexcept {
	try {
		auto log_level = getLogLevel();
		if (isDebugMode()) {
			log_level = spdlog::level::debug;
			spdlog::set_level(log_level);
			return true;
		}

		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		    getLogPath(m_po_var_map), true);
		auto logger = std::make_shared<spdlog::logger>(
		    "binance-data-collector", spdlog::sinks_init_list{file_sink});
		logger->set_pattern("[%Y-%m-%d %H:%M:%S](tid:%t) [%^%l%$] %v");
		logger->set_level(log_level);
		logger->flush_on(spdlog::level::err);
		spdlog::register_logger(logger);
		spdlog::set_default_logger(logger);
	} catch (const spdlog::spdlog_ex& err) {
		std::cerr << "Error initializing logger: " << err.what() << std::endl;
		return false;
	} catch (const std::exception& err) {
		std::cerr << "Unexpected error: " << err.what() << std::endl;
		return false;
	}
	return true;
}

bool Config::updateConfig() noexcept {
	try {
		auto config_path = getConfigPath(m_po_var_map);
		// TODO:
		// 1. get config file and parse if exists
		// 2. update existing values(override with CLI if provided)
		// 3. validate and apply updated config to a file
		// 4. think about which ones need to be configurable and which ones can
		// be hardcoded
		m_log_level = spdlog::level::info;
		m_connect_period = std::chrono::seconds(60);
		m_check_period = std::chrono::seconds(10);

		m_max_retries_num = 100;
		m_reconnection_delay = std::chrono::minutes(20 * 60);
		m_stats_flush_period = std::chrono::seconds(40);
		m_stats_output_path = g_def_stats_out_path_c;
		m_max_threads_num = (std::thread::hardware_concurrency() + 1) * 3 / 4;
		m_streams_list = {"btcusdt@trade", "ethusdt@trade", "bnbusdt@trade"};
		m_host_name = "stream.binance.com";
		m_port = "9443";
	} catch (const std::exception& err) {
		std::cerr << "Error updating config: " << err.what() << std::endl;
		return false;
	}
	return true;
}

bool Config::isHelp() const noexcept {
	return m_po_var_map.count(Key::help) > 0;
}

void Config::printHelp() const noexcept {
	m_po_desc.print(std::cout);
}

bool Config::isDebugMode() const noexcept {
	return m_po_var_map[Key::debug].as<bool>();
}

std::string_view Config::getServiceName() const noexcept {
	return g_service_name_c;
}

std::string_view Config::getServiceDisplayName() const noexcept {
	return g_service_display_name_c;
}

std::chrono::seconds Config::getConnectPeriod() const noexcept {
	return m_connect_period;
}

std::size_t Config::getMaxRetriesNum() const noexcept {
	return m_max_retries_num;
}

std::chrono::seconds Config::getCheckPeriod() const noexcept {
	return m_check_period;
}
std::chrono::seconds Config::getStatsFlushPeriod() const noexcept {
	return m_stats_flush_period;
}

std::chrono::minutes Config::getReconnectionDelay() const noexcept {
	return m_reconnection_delay;
}

std::string Config::getStatsOutputPath() const noexcept {
	return m_stats_output_path;
}

std::size_t Config::getMaxThreadsNum() const noexcept {
	return m_max_threads_num;
}

std::vector<std::string> Config::getStreamsList() const noexcept {
	return m_streams_list;
}

std::string Config::getHostName() const noexcept {
	return m_host_name;
}

std::string Config::getPort() const noexcept {
	return m_port;
}

} // namespace Config
