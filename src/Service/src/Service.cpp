#include "../Service.hpp"
#include "spdlog/spdlog.h"

namespace Service
{

namespace
{

class Service final
{
public:
    Service(Service& other) = delete;
    void operator=(Service& other) = delete;
    static std::unique_ptr<Service>& CreateInstance(ServiceBase& service);
	static std::unique_ptr<Service>& GetInstance();

	void start();
	void stop();
	[[maybe_unused]] void reload();
	[[maybe_unused]] Status getStatus() const;
	int debug();

	static void signal_term(int signal);
	static void signal_handler(int signal);

private:
	explicit Service(ServiceBase& service);

	static std::unique_ptr<Service> m_instance;
	ServiceBase& m_service;
	Status m_status;
};

Service::Service(ServiceBase& service)
	: m_service(service), m_status(Status::STOPPED) {}

std::unique_ptr<Service> Service::m_instance{nullptr};

std::unique_ptr<Service>& Service::CreateInstance(ServiceBase& service)
{
	if (m_instance != nullptr)
	{
		spdlog::debug("returning existing instance of Service");
		return m_instance;
	}
	spdlog::debug("creating new instance of Service");
	m_instance = std::make_unique<Service>(service);
	return m_instance;
}

std::unique_ptr<Service>& Service::GetInstance()
{
	return m_instance;
}

} // namespace for Service final class(unnamed)

bool runService(ServiceBase& service)
{
   return debugService(service);
}

bool debugService(ServiceBase& service)
{
    spdlog::info("Running service: {}", service.getDisplayName());
    if (service.onStart())
    {
        while (true)
        {
            service.doWork();
        }
        return true;
    }
    return false;
}

} // namespace Service
