#pragma once
#include "Service/ServiceBase.hpp"

namespace Service {

enum Status {
	STOPPED = 1,
	STOPPING,
	RUNNING,
	STARTING,
	RESUMING,
	PAUSING,
	PAUSED
};

bool runService(ServiceBase& service);
bool debugService(ServiceBase& service);

} // namespace Service
