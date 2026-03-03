#pragma once
#include <boost/program_options.hpp>
#include <memory>
#include <string_view>
#include "spdlog/spdlog.h"

namespace Config {

namespace po = boost::program_options;

class Config {
public:
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	static std::unique_ptr<Config>& getInstance();

	bool init(int argc, char* argv[]) noexcept;
	bool updateConfig() noexcept;
	bool initLogger() const noexcept;

	bool isHelp() const noexcept;
	bool isDebugMode() const noexcept;

	void printHelp() const noexcept;
	std::string_view getServiceName() const noexcept;
	std::string_view getServiceDisplayName() const noexcept;
	std::chrono::seconds getConnectPeriod() const noexcept;
	std::chrono::seconds getCheckPeriod() const noexcept;
	std::chrono::seconds getStatsFlushPeriod() const noexcept;
	std::string getStatsOutputPath() const noexcept;
	std::vector<std::string> getSymbols() const noexcept;
	std::size_t getMaxThreadsNum() const noexcept;

private:
	static std::unique_ptr<Config> m_instance;
	po::variables_map m_po_var_map;
	po::options_description m_po_desc{"Allowed options"};

	std::optional<std::string> m_log_level;

	std::optional<std::chrono::seconds> m_connect_period;
	std::optional<std::chrono::seconds> m_check_period;
	std::optional<std::chrono::seconds> m_stats_flush_period;
	std::optional<std::string> m_stats_output_path;
	std::optional<std::vector<std::string>> m_symbols;
	std::optional<std::size_t> m_max_threads_num;

private:
	std::string getLogLevel() const noexcept;

private:
	Config() = default;
};

} // namespace Config
