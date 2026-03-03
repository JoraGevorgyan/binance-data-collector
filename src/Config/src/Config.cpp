#include "../Config.hpp"
#include <iostream>

namespace Config {

namespace {

constexpr auto service_name_c = "binance-data-collector-service";
constexpr auto service_display_name_c = "Binance Data Collector Service";

} // namespace

std::unique_ptr<Config> Config::m_instance = nullptr;

std::unique_ptr<Config>& Config::getInstance() {
	if (m_instance != nullptr) {
		return m_instance;
	}
	m_instance = std::unique_ptr<Config>(new Config());
	return m_instance;
}

bool Config::init(int argc, char* argv[]) {
	try {
		constexpr auto def_conf_p = "/etc/binance-data-collector/config.yaml";
		constexpr auto def_log_p = "/var/log/binance-data-collector/cur.log";
		m_po_desc.add_options()("help,h", "produce help message")(
		    "config,c", po::value<std::string>()->default_value(def_conf_p),
		    "path to config file")(
		    "log-path", po::value<std::string>()->default_value(def_log_p),
		    "path to log file")(
		    "log-level,l", po::value<std::string>()->default_value("info"),
		    "log level (trace, debug, info, warn, error, critical, off)")(
		    "debug,d", po::bool_switch()->default_value(false),
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
		const auto log_path = m_po_var_map["log-path"].as<std::string>();
		auto log_level = m_po_var_map["log-level"].as<std::string>();

		if (isDebugMode()) {
			log_level = "debug";
		}

		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		    getLogPath(), true);
		std::vector<spdlog::sink_ptr> sinks{file_sink};
		auto logger = std::make_shared<spdlog::logger>(
		    "binance-data-collector", sinks.begin(), sinks.end());
		spdlog::register_logger(logger);
		spdlog::set_default_logger(logger);
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::err);
	} catch (const spdlog::spdlog_ex& err) {
		std::cerr << "Error initializing logger: " << err.what() << std::endl;
		return false;
	} catch (const std::exception& err) {
		std::cerr << "Unexcpected error: " << err.what() << std::endl;
		return false;
	}
	return true;
}

bool Config::updateConfig() const noexcept {
	try {
		auto config_path = getConfigPath();
		// load config from file and update
		// or generate the default one if doesn't exist
	} catch (const std::exception& err) {
		std::cerr << "Error updating config: " << err.what() << std::endl;
		return false;
	}
	return true;
}

bool Config::isHelp() const noexcept {
	return m_po_var_map.count("help") > 0;
}

void Config::printHelp() const noexcept {
	m_po_desc.print(std::cout);
}

bool Config::isDebugMode() const noexcept {
	return m_po_var_map["debug"].as<bool>();
}

std::string Config::getLogPath() const noexcept {
	return m_po_var_map["log-path"].as<std::string>();
}

std::string Config::getConfigPath() const noexcept {
	return m_po_var_map["config"].as<std::string>();
}

std::string_view Config::getServiceName() const noexcept {
	return service_name_c;
}

std::string_view Config::getServiceDisplayName() const noexcept {
	return service_display_name_c;
}

std::chrono::seconds Config::getIdleConnectPeriod() const noexcept {
	return m_idle_connect_period;
}

std::chrono::seconds Config::getCheckPeriod() const noexcept {
	return m_check_period;
}

} // namespace Config
