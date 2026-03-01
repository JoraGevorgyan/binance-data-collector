#include "Service/ServiceBase.hpp"
#include "spdlog/spdlog.h"

#include <thread>

namespace Service {
ServiceBase::ServiceBase(std::string_view name, std::string_view display_name)
    : m_name(name), m_display_name(display_name) {}

const char* ServiceBase::getName() const {
	return m_name.c_str();
}

const char* ServiceBase::getDisplayName() const {
	return m_display_name.c_str();
}

int ServiceBase::onDebug() {
	spdlog::info("Debugging service: {}", getDisplayName());
	if (onStart()) {
		auto debug = std::thread(&ServiceBase::doWork, this);
		debug.join();
		spdlog::info("Service {} exited: {}", getDisplayName(), m_exit_code);
		return m_exit_code;
	}
	return -1;
}

} // namespace Service
