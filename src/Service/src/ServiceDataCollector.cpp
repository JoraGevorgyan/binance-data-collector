#include "../ServiceDataCollector.hpp"
#include "../../DataCollector/DataCollector.hpp"

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

void ServiceDataCollector::doWork() {
	doMainWork();
}

void ServiceDataCollector::doMainWork() noexcept {
	auto max_retries = m_config.getMaxRetriesNum();
	auto retries = 0U;
	while (!m_canceler.isCanceled()) {
		spdlog::info("Service {} is working...", m_display_name.c_str());

		DataCollector::WebSocketClient client(m_config, m_canceler);
		if (!client.runWebSocketSession()) {
			m_exit_code = EXIT_FAILURE;
			if (++retries >= max_retries) {
				spdlog::error("Reached max retries num({}). Exiting...",
				              max_retries);
				break;
			}
			spdlog::warn("session failed. Retrying...");
		} else {
			m_exit_code = EXIT_SUCCESS;
			spdlog::info("BinanceWebSocket client session ended successfully.");
		}
		m_config.updateConfig();
		max_retries = m_config.getMaxRetriesNum();
	}

	spdlog::info("Service {} is exiting...", m_display_name.c_str());
}

} // namespace Service
