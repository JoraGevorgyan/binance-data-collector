#pragma once
#include <boost/program_options.hpp>
#include <memory>
#include <string_view>
#include "spdlog/sinks/basic_file_sink.h"
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

	std::string_view getServiceName() const noexcept;
	std::string_view getServiceDisplayName() const noexcept;
	std::chrono::seconds getIdleConnectPeriod() const noexcept;
	std::chrono::seconds getCheckPeriod() const noexcept;
	void printHelp() const noexcept;

private:
	static std::unique_ptr<Config> m_instance;
	po::variables_map m_po_var_map;
	po::options_description m_po_desc{"Allowed options"};

	std::optional<std::string> m_log_level;
	std::optional<std::chrono::seconds> m_idle_connect_period;
	std::optional<std::chrono::seconds> m_check_period;

private:
	std::string getLogLevel() const noexcept;

private:
	Config() = default;
};

} // namespace Config
