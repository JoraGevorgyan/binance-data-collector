#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include "Service/ServiceBase.hpp"

namespace {

class FakeService final : public Service::ServiceBase {
public:
	explicit FakeService(const bool should_start)
	    : ServiceBase("test-name", "Test Display Name"),
	      m_should_start(should_start) {}

	bool onStart() override { return m_should_start; }

	bool onStop() override { return true; }

	bool onPause() override { return true; }

	bool onResume() override { return true; }

	bool onShutdown() override { return true; }

	bool onReload() override { return true; }

	void doWork() override { m_exit_code = 7; }

private:
	bool m_should_start;
};

} // namespace

BOOST_AUTO_TEST_CASE(service_base_exposes_name_and_display_name) {
	FakeService service(true);

	BOOST_TEST(service.getName() == "test-name");
	BOOST_TEST(service.getDisplayName() == "Test Display Name");
}

BOOST_AUTO_TEST_CASE(service_base_on_debug_returns_worker_exit_code) {
	FakeService service(true);

	BOOST_TEST(service.onDebug() == 7);
}

BOOST_AUTO_TEST_CASE(service_base_on_debug_fails_when_start_fails) {
	FakeService service(false);

	BOOST_TEST(service.onDebug() == EXIT_FAILURE);
}
