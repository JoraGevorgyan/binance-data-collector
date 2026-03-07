#pragma once
#include <boost/program_options.hpp>
#include <memory>
#include <string_view>
#include "../thirdparty/nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace Config {

namespace po = boost::program_options;

class Config {
public:
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	static std::unique_ptr<Config>& getInstance();

	bool init(int argc, char* argv[]) noexcept;
	void updateConfig() noexcept;
	bool initLogger() const noexcept;

	bool isHelp() const noexcept;
	bool isDebugMode() const noexcept;

	void printHelp() const noexcept;
	std::string_view getServiceName() const noexcept;
	std::string_view getServiceDisplayName() const noexcept;
	std::chrono::seconds getConnectPeriod() const noexcept;
	std::chrono::seconds getCheckPeriod() const noexcept;
	std::size_t getMaxRetriesNum() const noexcept;
	std::chrono::minutes getReconnectionDelay() const noexcept;
	std::chrono::seconds getStatsFlushPeriod() const noexcept;
	std::string getStatsOutputPath() const noexcept;
	std::size_t getMaxThreadsNum() const noexcept;
	std::vector<std::string> getStreamsList() const noexcept;
	std::string getHostName() const noexcept;
	std::string getPort() const noexcept;

private:
	static std::unique_ptr<Config> m_instance;
	po::variables_map m_po_var_map;
	po::options_description m_po_desc{"Allowed options"};

	std::optional<spdlog::level::level_enum> m_log_level;
	std::optional<std::string> m_log_path;
	std::optional<std::string> m_stats_output_path;
	std::chrono::seconds m_connect_period;
	std::chrono::seconds m_check_period;
	std::size_t m_max_retries_num;
	std::chrono::minutes m_reconnection_delay;
	std::chrono::seconds m_stats_flush_period;
	std::size_t m_max_threads_num;
	std::vector<std::string> m_streams_list;
	std::string m_host_name;
	std::string m_port;

private:
	spdlog::level::level_enum getLogLevel() const noexcept;
	void initValuesFromCli() noexcept;
	void initValuesFromConfIfValid(const std::string& config_path) noexcept;
	void dumpValidConfValues(const std::string& config_path) const noexcept;
	void setDefaultValues() noexcept;
	nlohmann::json getJsonValues() const noexcept;

private:
	Config() = default;
};

} // namespace Config
