#include "../Aggregator.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <thread>
#include <unordered_map>
#include <vector>
#include "../../../thirdparty/nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace DataCollector {

namespace {

std::string formatTimestamp(
    std::chrono::system_clock::time_point t_point) noexcept {
	const auto to_time = std::chrono::system_clock::to_time_t(t_point);
	std::tm tm{};
	gmtime_r(&to_time, &tm); // thread-safe on Linux
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
	return oss.str();
}

} // namespace

Aggregator::Aggregator(Canceler::Canceler& canceler,
                       const std::weak_ptr<spdlog::logger>& out,
                       std::chrono::seconds flush_period)
    : m_canceler(canceler), m_out(out.lock()), m_flush_period(flush_period) {}

std::optional<TradeEvent> Aggregator::parseTradeEvent(
    const std::string& message) noexcept {
	try {
		spdlog::debug("Parsing message: {}", message);
		TradeEvent result;
		nlohmann::json js_obj = nlohmann::json::parse(message, nullptr, false);
		if (js_obj.is_discarded()) {
			spdlog::warn("Discarded malformed JSON");
			return std::nullopt;
		}

		if (!js_obj.contains("data") || !js_obj["data"].is_object()) {
			spdlog::warn("Missing data field");
			return std::nullopt;
		}

		const auto& d = js_obj["data"];
		if (!d.contains("s") || !d.contains("p") || !d.contains("q") ||
		    !d.contains("m")) {
			spdlog::warn("Missing required trade fields");
			return std::nullopt;
		}

		result.symbol = d.value("s", "");
		result.price = std::stod(d.value("p", "0"));
		result.quantity = std::stod(d.value("q", "0"));
		result.is_buyer_or_maker = d.value("m", true);
		return result;
	} catch (const std::exception& ex) {
		spdlog::warn("Trade parse error: {}", ex.what());
		return std::nullopt;
	}
}

void Aggregator::update(const TradeEvent& event_msg) noexcept {
	TradeStatistics initial_stats{};
	initial_stats.trades = 1;
	initial_stats.volume = event_msg.price * event_msg.quantity;
	initial_stats.min_price = event_msg.price;
	initial_stats.max_price = event_msg.price;
	if (event_msg.is_buyer_or_maker) {
		initial_stats.sell_count = 1;
	} else {
		initial_stats.buy_count = 1;
	}

	m_statistics.insert_or_visit(
	    {event_msg.symbol, initial_stats}, [&](auto& entry) {
		    TradeStatistics& stats = entry.second;
		    stats.trades += 1;
		    stats.volume += event_msg.price * event_msg.quantity;
		    stats.min_price = std::min(stats.min_price, event_msg.price);
		    stats.max_price = std::max(stats.max_price, event_msg.price);
		    if (event_msg.is_buyer_or_maker) {
			    stats.sell_count += 1;
		    } else {
			    stats.buy_count += 1;
		    }
	    });
}

bool Aggregator::writeSnapshotSync() noexcept {
	if (m_out == nullptr) {
		spdlog::critical("impossible(inited and checked in config)");
		return false;
	}
	std::unordered_map<std::string, TradeStatistics> cur_stats;
	m_statistics.visit_all([&](auto& entry) {
		cur_stats[entry.first] = entry.second;
		entry.second.reset();
	});

	const auto timestamp = formatTimestamp(std::chrono::system_clock::now());
	m_out->info("timestamp={}", timestamp);

	for (const auto& entry : cur_stats) {
		const std::string& symbol = entry.first;
		const TradeStatistics& tmp_s = entry.second;
		m_out->info(
		    "symbol={} trades={} volume={} min={} max={} buy={} sell={}",
		    symbol, tmp_s.trades, tmp_s.volume, tmp_s.min_price,
		    tmp_s.max_price, tmp_s.buy_count, tmp_s.sell_count);
	}
	m_out->flush();
	return true;
}

std::thread Aggregator::startFlushWorker() noexcept {
	return std::thread(&Aggregator::flushWorker, this);
}

void Aggregator::flushWorker() noexcept {
	auto next_flush = std::chrono::steady_clock::now();
	while (!m_canceler.isCanceled()) {
		spdlog::debug("flush worker thread in progress");
		next_flush += m_flush_period;
		std::this_thread::sleep_until(next_flush);
		if (m_canceler.isCanceled()) {
			break;
		}

		if (!writeSnapshotSync()) {
			spdlog::error("Failed to write snapshot");
		} else {
			spdlog::info("Snapshot flushed successfully");
		}
	}
	spdlog::info("flush worker thread woke up");
}

} // namespace DataCollector
