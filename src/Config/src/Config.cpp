#include "../Config.hpp"
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "spdlog/sinks/rotating_file_sink.h"

namespace Config {

namespace {

constexpr auto def_log_name = "bca_service.log";
constexpr auto def_stats_name = "bca_statistics.log";
constexpr std::size_t megabytes = 1024 * 1024;
constexpr std::size_t def_max_log_size_mb = 10 * megabytes;
constexpr std::size_t def_max_log_files_num = 4;

namespace Key {

constexpr auto log_path = "log-path";
constexpr auto log_level = "log-level";
constexpr auto debug = "debug";
constexpr auto config = "config";
constexpr auto help = "help";
constexpr auto stats_path = "stats-output-path";
constexpr auto connect_period_seconds = "connect-period-seconds";
constexpr auto check_period_seconds = "check-period-seconds";
constexpr auto max_retries_num = "max-retries-num";
constexpr auto stats_flush_period_seconds = "stats-flush-period-seconds";
constexpr auto reconnection_delay_minutes = "reconnection-delay-minutes";
constexpr auto max_threads_num = "max-threads-num";
constexpr auto max_log_size = "max-log-size-megabytes";
constexpr auto max_log_files_num = "max-log-files-num";
constexpr auto streams = "streams";
constexpr auto host = "host";
constexpr auto port = "port";

} // namespace Key

spdlog::level::level_enum getLogLvlFromStr(const std::string& level) noexcept {
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
	return spdlog::level::info;
}

std::string logLvlToString(spdlog::level::level_enum level) noexcept {
	switch (level) {
		case spdlog::level::trace:
			return "trace";
		case spdlog::level::debug:
			return "debug";
		case spdlog::level::info:
			return "info";
		case spdlog::level::warn:
			return "warn";
		case spdlog::level::err:
			return "error";
		case spdlog::level::critical:
			return "critical";
		case spdlog::level::off:
			return "off";
		default:
			return "info";
	}
}

std::string getConfigPath(const po::variables_map& var_map) noexcept {
	return var_map[Key::config].as<std::string>();
}

bool writeJsonToFile(const nlohmann::json& obj,
                     const std::string& out_p) noexcept {
	try {
		std::ofstream ofs_out(out_p, std::ios::out | std::ios::trunc);
		if (ofs_out.is_open()) {
			ofs_out << obj.dump(4);
			ofs_out << std::endl;
			ofs_out.close();
			return true;
		}
		return false;
	} catch (const std::exception& err) {
		std::cerr << "error when trying to dump json to a file: " << err.what()
		          << std::endl;
		std::cerr << "filename: " << out_p << std::endl;
		return false;
	}
}

void writeJsonToFile(const nlohmann::json& obj,
                     const std::string& out_p,
                     const std::string& out_p_def) noexcept {
	if (writeJsonToFile(obj, out_p)) {
		return;
	}
	writeJsonToFile(obj, out_p_def);
}

std::shared_ptr<spdlog::logger> initFlusherLogger(const std::string& file_path,
                                                  std::size_t max_size,
                                                  std::size_t max_num) {
	auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
	    file_path, max_size, max_num);
	auto stats_logger =
	    std::make_shared<spdlog::logger>("bca_flusher_logger", sink);
	stats_logger->set_pattern("%v");
	stats_logger->flush_on(spdlog::level::critical);
	spdlog::register_logger(stats_logger);
	return stats_logger;
}

} // namespace

std::unique_ptr<Config> Config::m_instance = nullptr;

void Config::initValuesFromCli() noexcept {
	if (m_po_var_map.count(Key::log_level) > 0) {
		m_log_level = getLogLevel();
	}
	if (m_po_var_map.count(Key::log_path) > 0) {
		m_log_path = m_po_var_map[Key::log_path].as<std::string>();
	}
	if (m_po_var_map.count(Key::stats_path) > 0) {
		m_stats_output_path = m_po_var_map[Key::stats_path].as<std::string>();
	}
}

std::unique_ptr<Config>& Config::getInstance() {
	if (m_instance != nullptr) {
		return m_instance;
	}
	m_instance = std::unique_ptr<Config>(new Config());
	return m_instance;
}

nlohmann::json Config::getJsonValues() const noexcept {
	nlohmann::json res{};

	res[Key::log_level] = logLvlToString(m_log_level.value());
	res[Key::stats_path] = m_stats_output_path.value();
	res[Key::connect_period_seconds] = m_connect_period.count();
	res[Key::check_period_seconds] = m_check_period.count();
	res[Key::max_retries_num] = m_max_retries_num;
	res[Key::stats_flush_period_seconds] = m_stats_flush_period.count();
	res[Key::reconnection_delay_minutes] = m_reconnection_delay.count();
	res[Key::max_threads_num] = m_max_threads_num;
	res[Key::max_log_size] = m_max_log_size / megabytes;
	res[Key::max_log_files_num] = m_max_log_files_num;
	res[Key::streams] = m_streams_list;
	res[Key::host] = m_host_name;
	res[Key::port] = m_port;

	return res;
}

spdlog::level::level_enum Config::getLogLevel() const noexcept {
	if (m_po_var_map.count(Key::log_level) > 0) { // CLI overrides config file
		const auto level = m_po_var_map[Key::log_level].as<std::string>();
		return getLogLvlFromStr(level);
	}
	return m_log_level.value_or(spdlog::level::info);
}

bool Config::init(int argc, char* argv[]) noexcept {
	try {
		m_po_desc.add_options()(Key::help, "produce help message")(
		    Key::config, po::value<std::string>()->default_value("config.json"),
		    "path to config file")(
		    Key::log_path,
		    po::value<std::string>()->default_value(def_log_name),
		    "path to log file")(
		    Key::log_level, po::value<std::string>()->default_value("info"),
		    "log level (trace, debug, info, warn, error, critical, off)")(
		    Key::debug, po::bool_switch()->default_value(false),
		    "enable debug mode (overrides log level to debug)")(
		    Key::stats_path,
		    po::value<std::string>()->default_value(def_stats_name),
		    "path to statistics output file");

		po::store(po::parse_command_line(argc, argv, m_po_desc), m_po_var_map);

		po::notify(m_po_var_map);
	} catch (const po::error& err) {
		std::cerr << "Error parsing command line: " << err.what() << std::endl;
		printHelp();
		return false;
	} catch (const std::exception& err) {
		std::cerr << "Unexpected error: " << err.what() << std::endl;
		printHelp();
		return false;
	}
	return true;
}

bool Config::initLogger() noexcept {
	if (m_logger != nullptr) {
		m_logger->set_level(m_log_level.value());
		return true;
	}
	try {
		std::shared_ptr<spdlog::sinks::sink> sink = nullptr;
		auto log_level = getLogLevel();
		if (isDebugMode()) {
			log_level = spdlog::level::debug;
			sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
		} else {
			sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			    m_log_path.value_or(def_log_name), m_max_log_size,
			    m_max_log_files_num);
		}

		m_logger =
		    std::make_shared<spdlog::logger>("binance-data-collector", sink);
		m_logger->set_pattern("[%Y-%m-%d %H:%M:%S](tid:%t) [%^%l%$] %v");
		m_logger->set_level(log_level);
		m_logger->flush_on(log_level);
		spdlog::register_logger(m_logger);
		spdlog::set_default_logger(m_logger);

		m_stats_logger =
		    initFlusherLogger(m_stats_output_path.value_or(def_stats_name),
		                      m_max_log_size, m_max_log_files_num);
	} catch (const spdlog::spdlog_ex& err) {
		std::cerr << "Error initializing logger: " << err.what() << std::endl;
		return false;
	} catch (const std::exception& err) {
		std::cerr << "Unexpected error: " << err.what() << std::endl;
		return false;
	}
	return true;
}

void Config::setDefaultValues() noexcept {
	if (!m_log_path.has_value()) {
		m_log_path = def_log_name;
	}
	if (!m_log_level.has_value()) {
		m_log_level = spdlog::level::info;
	}
	if (!m_stats_output_path.has_value()) {
		m_stats_output_path = def_stats_name;
	}
	m_connect_period = std::chrono::seconds(60);
	m_check_period = std::chrono::seconds(10);
	m_max_retries_num = 10;
	m_stats_flush_period = std::chrono::seconds(40);
	m_reconnection_delay = std::chrono::minutes(20 * 60);
	m_max_threads_num = (std::thread::hardware_concurrency() + 1) * 3 / 4;
	m_max_log_size = def_max_log_size_mb;
	m_max_log_files_num = def_max_log_files_num;
	m_streams_list = {"btcusdt@trade", "ethusdt@trade", "bnbusdt@trade"};
	m_host_name = "stream.binance.com";
	m_port = "9443";
}

void Config::initValuesFromConfIfValid(
    const std::string& config_path) noexcept {
	try {
		std::ifstream in_stream(config_path);
		if (!in_stream.is_open()) {
			std::cerr << "Failed to open config file: " << config_path
			          << std::endl;
			return;
		}
		nlohmann::json config = nlohmann::json::parse(
		    in_stream, nullptr, false /*nothrow*/, true /*ignore comments*/);

		if (config.contains(Key::log_level)) {
			m_log_level =
			    getLogLvlFromStr(config[Key::log_level].get<std::string>());
		}
		if (config.contains(Key::stats_path)) {
			m_stats_output_path = config[Key::stats_path].get<std::string>();
		}
		if (config.contains(Key::connect_period_seconds)) {
			const auto tmp = std::chrono::seconds(
			    config[Key::connect_period_seconds].get<unsigned>());
			if (tmp.count() > 0 && tmp.count() < 10000) {
				m_connect_period = tmp;
			}
		}

		if (config.contains(Key::check_period_seconds)) {
			const auto tmp = std::chrono::seconds(
			    config[Key::check_period_seconds].get<unsigned>());
			if (tmp.count() > 0 &&
			    tmp.count() <= m_connect_period.count() / 2) {
				m_check_period = tmp;
			}
		}

		if (config.contains(Key::max_retries_num)) {
			const auto tmp = config[Key::max_retries_num].get<unsigned>();
			if (tmp > 0 && tmp < 10000) {
				m_max_retries_num = tmp;
			}
		}

		if (config.contains(Key::stats_flush_period_seconds)) {
			const auto tmp = std::chrono::seconds(
			    config[Key::stats_flush_period_seconds].get<unsigned>());
			if (tmp.count() > 0 && tmp.count() < m_connect_period.count()) {
				m_stats_flush_period = tmp;
			}
		}
		if (config.contains(Key::reconnection_delay_minutes)) {
			const auto tmp = std::chrono::minutes(
			    config[Key::reconnection_delay_minutes].get<unsigned>());
			if (tmp.count() > 5 && tmp.count() < 23 * 60 + 55) {
				m_reconnection_delay = tmp;
			}
		}

		if (config.contains(Key::max_threads_num)) {
			m_max_threads_num = config[Key::max_threads_num].get<unsigned>();
		}

		if (config.contains(Key::max_log_size)) {
			const auto tmp = config[Key::max_log_size].get<unsigned>();
			if (tmp > 0) {
				m_max_log_size = tmp * megabytes;
			}
		}

		if (config.contains(Key::max_log_files_num)) {
			const auto tmp = config[Key::max_log_files_num].get<unsigned>();
			if (tmp > 0) {
				m_max_log_files_num = tmp;
			}
		}

		if (config.contains(Key::streams) && config[Key::streams].is_array()) {
			m_streams_list = // check this too
			    config[Key::streams].get<std::vector<std::string>>();
		}

		if (config.contains(Key::host)) { // also check this
			m_host_name = config[Key::host].get<std::string>();
		}

		if (config.contains(Key::port)) { /// and this
			m_port = config[Key::port].get<std::string>();
		}
	} catch (const nlohmann::json::exception& err) {
		std::cerr << "Error parsing JSON config: " << err.what() << std::endl;
	} catch (const std::exception& err) {
		std::cerr << "Unexpected err when parsing: " << err.what() << std::endl;
	}
}

void Config::dumpValidConfValues(
    const std::string& config_path) const noexcept {
	const auto conf_values = getJsonValues();
	writeJsonToFile(conf_values, config_path, "generated_conf.json");
}

void Config::updateConfig() noexcept {
	auto config_path = getConfigPath(m_po_var_map);
	initValuesFromCli();   // will not be changed if set
	setDefaultValues();    // will be overridden by config values if exist any
	std::error_code err_c; // need this way to have no exception
	if (!std::filesystem::exists(config_path, err_c)) {
		dumpValidConfValues(config_path);
		return;
	}
	initValuesFromConfIfValid(config_path);
	dumpValidConfValues(config_path);
	if (m_logger != nullptr) {
		m_logger->set_level(m_log_level.value());
	}
}

std::shared_ptr<spdlog::logger> Config::getStatsLogger() const noexcept {
	return m_stats_logger;
}

bool Config::isHelp() const noexcept {
	return m_po_var_map.count(Key::help) > 0;
}

void Config::printHelp() const noexcept {
	m_po_desc.print(std::cout);
}

bool Config::isDebugMode() const {
	if (m_po_var_map.count(Key::debug) > 0) {
		return m_po_var_map[Key::debug].as<bool>();
	}
	return false;
}

std::string_view Config::getServiceName() const noexcept {
	return "binance-data-collector";
}

std::string_view Config::getServiceDisplayName() const noexcept {
	return "Binance Data Collector Service";
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
