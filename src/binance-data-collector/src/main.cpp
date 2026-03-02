#include <iostream>
#include "Config/Config.hpp"
#include "Service/Service.hpp"

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
	if (!(config->initLogger() && config->updateConfig())) {
		return EXIT_FAILURE;
	}

	spdlog::info("Binance Data Collector started");

	return EXIT_SUCCESS;
}

} // namespace DataCollector

int main(int argc, char* argv[]) {
	return DataCollector::mainImpl(argc, argv);
}
