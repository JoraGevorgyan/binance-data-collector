#pragma once
#include <boost/program_options.hpp>
#include <memory>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace Config {

namespace po = boost::program_options;

class Config {
public:
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	static std::unique_ptr<Config>& getInstance();

	bool init(int argc, char* argv[]);

	bool initLogger() const noexcept;
	bool updateConfig() const noexcept;
	bool isHelp() const noexcept;
	void printHelp() const noexcept;
	bool isDebugMode() const noexcept;

private:
	static std::unique_ptr<Config> m_instance;
	po::variables_map m_po_var_map;
	po::options_description m_po_desc{"Allowed options"};

private:
	std::string getLogPath() const noexcept;
	std::string getConfigPath() const noexcept;

private:
	Config() = default;
};

} // namespace Config
