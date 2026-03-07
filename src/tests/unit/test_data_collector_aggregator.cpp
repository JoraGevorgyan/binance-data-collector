#include <boost/test/unit_test.hpp>
#include "DataCollector/Aggregator.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

BOOST_AUTO_TEST_CASE(aggregator_parse_trade_event_success) {
	const std::string msg =
	    R"({"stream":"btcusdt@trade","data":{"s":"BTCUSDT","p":"100.5","q":"2.0","m":false}})";

	const auto parsed = DataCollector::Aggregator::parseTradeEvent(msg);
	BOOST_REQUIRE(parsed.has_value());
	BOOST_TEST(parsed->symbol == "BTCUSDT");
	BOOST_TEST(parsed->price == 100.5);
	BOOST_TEST(parsed->quantity == 2.0);
	BOOST_TEST(parsed->is_buyer_or_maker == false);
}

BOOST_AUTO_TEST_CASE(aggregator_parse_trade_event_rejects_invalid_messages) {
	const auto malformed =
	    DataCollector::Aggregator::parseTradeEvent("{not valid json");
	BOOST_TEST(!malformed.has_value());

	const std::string missing_fields =
	    R"({"stream":"btcusdt@trade","data":{"s":"BTCUSDT","p":"100.5"}})";
	const auto incomplete =
	    DataCollector::Aggregator::parseTradeEvent(missing_fields);
	BOOST_TEST(!incomplete.has_value());
}

BOOST_AUTO_TEST_CASE(aggregator_flush_worker_writes_snapshot) {
	Canceler::Canceler canceler;
	const auto tmp_path = std::filesystem::temp_directory_path() /
	                      ("aggregator_ut_" + std::to_string(getpid()) +
	                       "_" +
	                       std::to_string(std::chrono::steady_clock::now()
	                                          .time_since_epoch()
	                                          .count()) +
	                       ".log");

	DataCollector::Aggregator aggregator(
	    std::chrono::seconds(1), tmp_path.string(), canceler);

	DataCollector::TradeEvent first_event;
	first_event.symbol = "BTCUSDT";
	first_event.price = 100.0;
	first_event.quantity = 1.0;
	first_event.is_buyer_or_maker = false;
	aggregator.update(first_event);

	DataCollector::TradeEvent second_event;
	second_event.symbol = "BTCUSDT";
	second_event.price = 120.0;
	second_event.quantity = 2.0;
	second_event.is_buyer_or_maker = true;
	aggregator.update(second_event);

	auto worker = aggregator.startFlushWorker();
	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	canceler.cancel();
	worker.join();

	std::ifstream in(tmp_path);
	BOOST_REQUIRE(in.is_open());
	std::stringstream buffer;
	buffer << in.rdbuf();
	const auto snapshot = buffer.str();

	BOOST_TEST(snapshot.find("timestamp=") != std::string::npos);
	BOOST_TEST(snapshot.find("symbol=BTCUSDT trades=2") != std::string::npos);
	BOOST_TEST(snapshot.find("buy=1 sell=1") != std::string::npos);

	std::filesystem::remove(tmp_path);
}
