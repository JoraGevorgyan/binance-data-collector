#include <iostream>
#include "Config/Config.hpp"
#include "Service/Service.hpp"
#include "Service/ServiceDataCollector.hpp"

namespace DataCollector {

int mainImpl(int argc, char* argv[]) {
	auto& config = Config::Config::getInstance();
	if (config == nullptr) {
		std::cerr << "Failed to create Config instance" << std::endl;
		return EXIT_FAILURE;
	}
	if (!config->init(argc, argv)) {
		return EXIT_FAILURE;
	}
	if (config->isHelp()) {
		config->printHelp();
		return EXIT_SUCCESS;
	}
	config->updateConfig();
	if (!config->initLogger()) {
		return EXIT_FAILURE;
	}

	spdlog::info("Binance Data Collector started");
	Service::ServiceDataCollector data_collector(*config);
	return Service::runService(data_collector) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace DataCollector

int main(int argc, char* argv[]) {
	return DataCollector::mainImpl(argc, argv);
}
