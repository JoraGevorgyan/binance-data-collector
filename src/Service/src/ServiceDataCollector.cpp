#include "../ServiceDataCollector.hpp"

namespace Service {

ServiceDataCollector::ServiceDataCollector(Config::Config& config)
    : ServiceBase(config.getServiceName(), config.getServiceDisplayName()),
      m_config(config) {}

bool ServiceDataCollector::onStart() {
	spdlog::info("Starting service: {}", m_display_name.c_str());
	m_canceler.reset();
	return true;
}

bool ServiceDataCollector::onStop() {
	spdlog::info("Stopping service: {}", m_display_name.c_str());
	m_canceler.cancel();
	return true;
}

bool ServiceDataCollector::onPause() {
	if (!m_canceler.isCanceled()) {
		return onStop();
	}
	return false;
}

bool ServiceDataCollector::onResume() {
	if (m_canceler.isCanceled()) {
		return onStart();
	}
	return false;
}

bool ServiceDataCollector::onShutdown() {
	if (!m_canceler.isCanceled()) {
		return onStop();
	}
	return true;
}

bool ServiceDataCollector::onReload() {
	return true;
}

void ServiceDataCollector::doMainWork() noexcept {
	if (!m_config.updateConfig()) {
		spdlog::error("Failed to update configuration.");
		return;
	}
	// TODO: implemnt the main logic here
}

void ServiceDataCollector::doWork() {
	auto interval = m_config.getConnectPeriod();
	while (!m_canceler.isCanceled()) {
		spdlog::info("Service {} is working...", m_display_name.c_str());
		if (interval >= m_config.getConnectPeriod()) {
			doMainWork();
			interval = std::chrono::seconds(0);
		}
		std::this_thread::sleep_for(m_config.getCheckPeriod());
		interval += m_config.getCheckPeriod();
	}

	spdlog::info("Service {} is exiting...", m_display_name.c_str());
	m_exit_code = 0;
}

} // namespace Service
