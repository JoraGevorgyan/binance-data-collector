#include "../Service.hpp"
#include "spdlog/spdlog.h"

#include <csignal>

namespace Service {

namespace {

class Service final {
public:
	explicit Service(ServiceBase& service);

	Service(Service& other) = delete;
	void operator=(Service& other) = delete;

	static std::unique_ptr<Service>& createInstance(ServiceBase& service);
	static std::unique_ptr<Service>& getInstance();

	void start();
	void stop();
	[[maybe_unused]] void reload();
	[[maybe_unused]] void restart();
	[[maybe_unused]] Status getStatus() const;
	int debug();

	static void signalTerm(int sig_num);
	static void signalHandler(int sig_num);

private:
	static std::unique_ptr<Service> m_instance;
	ServiceBase& m_service;
	Status m_status;
};

Service::Service(ServiceBase& service)
    : m_service(service), m_status(Status::STOPPED) {}

Status Service::getStatus() const {
	return m_status;
}

void Service::start() {
	m_status = Status::STARTING;
	if (m_service.onStart()) {
		m_status = Status::RUNNING;
		spdlog::info("Service {} started successfully",
		             m_service.getDisplayName());
		return;
	}
	m_status = Status::STOPPED;
	spdlog::error("Failed to start service {}", m_service.getDisplayName());
}

void Service::stop() {
	m_status = Status::STOPPING;
	if (m_service.onStop()) {
		m_status = Status::STOPPED;
		spdlog::info("Service {} stopped successfully",
		             m_service.getDisplayName());
		return;
	}
	m_status = Status::RUNNING;
	spdlog::error("Failed to stop service {}", m_service.getDisplayName());
}

void Service::reload() {
	spdlog::info("Reloading service: {}", m_service.getDisplayName());
	if (m_service.onReload()) {
		spdlog::info("Service {} reloaded successfully",
		             m_service.getDisplayName());
	} else {
		spdlog::error("Failed to reload service {}",
		              m_service.getDisplayName());
	}
}

void Service::restart() {
	spdlog::info("Restarting service: {}", m_service.getDisplayName());
	if (m_status == Status::RUNNING) {
		Service::stop();
	}
	Service::start();
}

int Service::debug() {
	return m_service.onDebug();
}

std::unique_ptr<Service> Service::m_instance{nullptr};

std::unique_ptr<Service>& Service::createInstance(ServiceBase& service) {
	if (m_instance != nullptr) {
		spdlog::debug("returning existing instance of Service");
		return m_instance;
	}
	spdlog::debug("creating new instance of Service");
	m_instance = std::make_unique<Service>(service);
	return m_instance;
}

std::unique_ptr<Service>& Service::getInstance() {
	return m_instance;
}

void Service::signalTerm(int sig_num) {
	spdlog::info("Received termination signal: {}", sig_num);
	if (sig_num == SIGABRT || sig_num == SIGSEGV || sig_num == SIGTERM ||
	    sig_num == SIGKILL) {
		spdlog::error("Abnormal termination signal received: {}", sig_num);
		signal(sig_num, SIG_DFL);
		kill(getpid(), sig_num);
	}
	auto& service = Service::getInstance();
	if (service != nullptr) {
		service->stop();
		return;
	}
	spdlog::warn("No service instance available to stop on signal: {}",
	             sig_num);
	std::exit(EXIT_FAILURE);
}

void Service::signalHandler(int sig_num) {
	spdlog::info("Received signal: {}", sig_num);
	switch (sig_num) {
		case SIGINT:
		case SIGTERM:
			signalTerm(sig_num);
			break;
		default:
			spdlog::warn("Unhandled signal received: {}", sig_num);
			auto& service = Service::getInstance();
			if (service != nullptr) {
				service->reload();
			}
	}
}

void setupSigAction(struct sigaction& sig_action) {
	sigset_t s_set{};
	sigemptyset(&s_set);
	sigaddset(&s_set, SIGABRT);
	sigaddset(&s_set, SIGSEGV);
	sigaddset(&s_set, SIGTERM);
	sigaddset(&s_set, SIGKILL);
	sig_action.sa_handler = Service::signalTerm;
	sig_action.sa_mask = s_set;
	sig_action.sa_flags = 0;
	sigaction(SIGABRT, &sig_action, nullptr);
	sigaction(SIGSEGV, &sig_action, nullptr);
	sigaction(SIGTERM, &sig_action, nullptr);
	sigaction(SIGKILL, &sig_action, nullptr);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, Service::signalHandler);
}

} // namespace

bool runService(ServiceBase& service) {
	return debugService(service);
}

bool debugService(ServiceBase& service) {
	spdlog::info("running service: {}", service.getDisplayName());
	struct sigaction sa {};
	setupSigAction(sa);
	return Service::createInstance(service)->debug() == 0;
}

} // namespace Service
