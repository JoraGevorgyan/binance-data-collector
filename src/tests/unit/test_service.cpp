#include <boost/test/unit_test.hpp>
#include "Service/Service.hpp"

namespace {

class FakeService final : public Service::ServiceBase {
public:
	FakeService() : ServiceBase("service-name", "Service Display") {}

	bool onStart() override {
		++start_calls;
		return true;
	}

	bool onStop() override { return true; }

	bool onPause() override { return true; }

	bool onResume() override { return true; }

	bool onShutdown() override { return true; }

	bool onReload() override { return true; }

	void doWork() override {
		++work_calls;
		m_exit_code = EXIT_SUCCESS;
	}

	int start_calls{0};
	int work_calls{0};
};

} // namespace

BOOST_AUTO_TEST_CASE(service_run_and_debug_return_success) {
	FakeService service;

	BOOST_TEST(Service::debugService(service));
	BOOST_TEST(Service::runService(service));
	BOOST_TEST(service.start_calls == 2);
	BOOST_TEST(service.work_calls == 2);
}
