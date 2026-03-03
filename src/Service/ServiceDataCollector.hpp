#pragma once

#include "Service.hpp"

namespace Service {

class DataCollector : public ServiceBase {
public:
	explicit DataCollector() = default;
	bool onStart() override;
	bool onStop() override;
	bool onPause() override;
	bool onResume() override;
	bool onShutdown() override;
	bool onReload() override;
	void doWork() override;

private:
};

} // namespace Service
