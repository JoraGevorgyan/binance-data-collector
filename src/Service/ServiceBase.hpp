#pragma once

#include <cstdlib>
#include <string>
#include <string_view>

namespace Service {

class ServiceBase {
public:
	explicit ServiceBase(std::string_view name, std::string_view display_name);
	virtual ~ServiceBase() = default;

	std::string_view getName() const;
	std::string_view getDisplayName() const;

	virtual bool onStart() = 0;
	virtual bool onStop() = 0;
	virtual bool onPause() = 0;
	virtual bool onResume() = 0;
	virtual bool onShutdown() = 0;
	virtual bool onReload() = 0;
	virtual void doWork() = 0;

	int onDebug();

protected:
	const std::string m_name;
	const std::string m_display_name;
	int m_exit_code = EXIT_FAILURE;
};

} // namespace Service
