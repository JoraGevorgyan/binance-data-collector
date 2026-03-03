#pragma once

#include "../Config/Config.hpp"
#include "../common/Canceler.hpp"
#include "Service.hpp"

namespace Service {

class ServiceDataCollector : public ServiceBase {
public:
	explicit ServiceDataCollector(Config::Config& config);
	bool onStart() override;
	bool onStop() override;
	bool onPause() override;
	bool onResume() override;
	bool onShutdown() override;
	bool onReload() override;
	void doWork() override;

private:
	Config::Config& m_config;
	Canceler::Canceler m_canceler;
};

} // namespace Service
