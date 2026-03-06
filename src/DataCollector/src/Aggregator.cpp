#include "../Aggregator.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace DataCollector {

namespace {

std::string formatTimestamp(
    std::chrono::system_clock::time_point t_point) noexcept {
	const auto to_time = std::chrono::system_clock::to_time_t(t_point);
	const auto tm = *std::gmtime(&to_time);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
	return oss.str();
}

} // namespace

Aggregator::Aggregator(std::chrono::seconds flush_period,
                       std::string output_dir,
                       Canceler::Canceler& canceler)
    : m_flush_period(flush_period),
      m_flush_out_dir(std::move(output_dir)),
      m_canceler(canceler),
      m_next_flush(std::chrono::steady_clock::now() + flush_period) {}

std::optional<TradeEvent> Aggregator::parseTradeEvent(
    const std::string& message) noexcept {
	{
		spdlog::debug("Parsing message: {}", message);
		std::this_thread::sleep_for(std::chrono::seconds(1));
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

		try {
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
}

void Aggregator::update(const TradeEvent& event_msg) noexcept {
	const std::lock_guard<std::mutex> lock(m_mutex);

	TradeStatistics& stats = m_statistics[event_msg.symbol];
	stats.trades += 1;
	stats.volume += event_msg.price * event_msg.quantity;
	stats.min_price = std::min(stats.min_price, event_msg.price);
	stats.max_price = std::max(stats.max_price, event_msg.price);
	if (event_msg.is_buyer_or_maker) {
		stats.sell_count += 1; // seller-initiated
	} else {
		stats.buy_count += 1; // buyer-initiated
	}
}

bool Aggregator::writeSnapshotSync() noexcept {
	if (!m_out) {
		spdlog::error("Output stream is not open");
		return false;
	}

	const auto timestamp = formatTimestamp(std::chrono::system_clock::now());
	m_out << "timestamp=" << timestamp << '\n';

	for (auto& entry : m_statistics) {
		const std::string& symbol = entry.first;
		TradeStatistics& tmp_s = entry.second;
		if (tmp_s.trades == 0) {
			continue;
		}
		m_out << "symbol=" << symbol << " trades=" << tmp_s.trades
		      << " volume=" << tmp_s.volume << " min=" << tmp_s.min_price
		      << " max=" << tmp_s.max_price << " buy=" << tmp_s.buy_count
		      << " sell=" << tmp_s.sell_count << '\n';
		tmp_s.reset();
	}
	m_out.flush();
	return true;
}

std::thread Aggregator::startFlushWorker() noexcept {
	return std::thread(&Aggregator::flushWorker, this);
}

void Aggregator::flushWorker() noexcept {
	while (!m_canceler.isCanceled()) {
		auto now = std::chrono::steady_clock::now();
		const std::unique_lock<std::mutex> lock(m_mutex);
		if (now > m_next_flush) {
			if (!writeSnapshotSync()) {
				spdlog::error("Failed to write snapshot");
			} else {
				spdlog::info("Snapshot flushed successfully");
			}
		} else {
			std::this_thread::yield();
			const auto dur =
			    std::chrono::duration_cast<std::chrono::milliseconds>(
			        m_next_flush - now);
			std::this_thread::sleep_for(dur);
		}
		m_next_flush = now + m_flush_period;
	}
}

} // namespace DataCollector
