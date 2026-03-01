#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include <iostream>

namespace DataCollector {
int mainImpl(int argc, char* argv[]) {
	// Register logger with hardcoded YET(get it from config in future)
	try {
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		    "/var/log/binance-data-collector/current.log", true);
		std::vector<spdlog::sink_ptr> sinks{file_sink};
		auto logger = std::make_shared<spdlog::logger>("binance", sinks.begin(),
		                                               sinks.end());
		spdlog::register_logger(logger);
		spdlog::set_default_logger(logger);
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::err);

		spdlog::info("Binance Data Collector started");

		for (int i = 0; i < argc; ++i) {
			spdlog::info("Argument {}: {}", i, argv[i]);
		}

	} catch (const spdlog::spdlog_ex& ex) {
		std::cerr << "Log initialization failed: " << ex.what() << std::endl;
	}

	return 0;
}
} // namespace DataCollector

int main(int argc, char* argv[]) {
	return DataCollector::mainImpl(argc, argv);
}
