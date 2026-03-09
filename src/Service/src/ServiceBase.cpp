#include "Service/ServiceBase.hpp"
#include "spdlog/spdlog.h"

#include <thread>

namespace Service {

ServiceBase::ServiceBase(std::string_view name, std::string_view display_name)
    : m_name(name), m_display_name(display_name) {}

std::string_view ServiceBase::getName() const {
	return m_name;
}

std::string_view ServiceBase::getDisplayName() const {
	return m_display_name;
}

int ServiceBase::onDebug() {
	spdlog::info("Debugging service: {}", getDisplayName());
	m_exit_code = EXIT_FAILURE;
	if (!onStart()) {
		spdlog::error("Failed to start service {}", getDisplayName());
		return m_exit_code;
	}

	auto debug = std::thread(&ServiceBase::doWork, this);
	debug.join();
	spdlog::info("Service {} exited: {}", getDisplayName(), m_exit_code);
	return m_exit_code;
}

} // namespace Service
