#include <boost/test/unit_test.hpp>
#include "common/Canceler.hpp"

BOOST_AUTO_TEST_CASE(canceler_default_state_is_not_canceled) {
	Canceler::Canceler canceler;

	BOOST_TEST(!canceler.isCanceled());
	BOOST_TEST(canceler.getState() == 0u);
}

BOOST_AUTO_TEST_CASE(canceler_cancel_and_reset_changes_state) {
	Canceler::Canceler canceler;

	canceler.cancel(Canceler::State::Abort);
	BOOST_TEST(canceler.isCanceled());
	BOOST_TEST(canceler.getState() == 5u);

	canceler.reset();
	BOOST_TEST(!canceler.isCanceled());
	BOOST_TEST(canceler.getState() == 0u);
}
