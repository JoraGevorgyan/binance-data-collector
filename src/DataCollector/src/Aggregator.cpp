#include "../Aggregator.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>
#include "spdlog/spdlog.h"

namespace DataCollector {

namespace {

struct TradeStatistics {
	uint64_t trades{0};
	double volume{0.0};
	double min_price{std::numeric_limits<double>::max()};
	double max_price{std::numeric_limits<double>::lowest()};
	uint64_t buy_count{0};
	uint64_t sell_count{0};

	void reset() {
		trades = 0;
		volume = 0.0;
		min_price = std::numeric_limits<double>::max();
		max_price = std::numeric_limits<double>::lowest();
		buy_count = 0;
		sell_count = 0;
	}
};

std::string formatTimestamp(std::chrono::system_clock::time_point tp) noexcept {
	std::time_t tt = std::chrono::system_clock::to_time_t(tp);
	std::tm tm = *std::gmtime(&tt);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
	return oss.str();
}

} // namespace

Aggregator::Aggregator(std::chrono::seconds flush_period)
    : m_flush_period(flush_period),
      m_next_flush(std::chrono::steady_clock::now() + flush_period) {}

bool Aggregator::update(const std::string& symbol,
                        double price,
                        double quantity,
                        bool is_buyer_maker) {
	std::lock_guard<std::mutex> lock(m_mutex);
	TradeStatistics& stats = m_statistics[symbol];
	stats.trades += 1;
	stats.volume += price * quantity;
	stats.min_price = std::min(stats.min_price, price);
	stats.max_price = std::max(stats.max_price, price);
	if (is_buyer_maker) {
		stats.sell_count += 1; // seller-initiated
	} else {
		stats.buy_count += 1; // buyer-initiated
	}
}

bool Aggregator::writeSnapshotSync(
    std::ofstream& out,
    std::chrono::system_clock::time_point wall_clock) noexcept {
	if (!out) {
		spdlog::error("Output stream is not open");
		return false;
	}

	std::string timestamp = formatTimestamp(wall_clock);
	out << "timestamp=" << timestamp << '\n';

	for (auto& entry : m_statistics) {
		const std::string& symbol = entry.first;
		TradeStatistics& tmp_s = entry.second;
		if (tmp_s.trades == 0) {
			continue;
		}
		out << "symbol=" << symbol << " trades=" << tmp_s.trades
		    << " volume=" << tmp_s.volume << " min=" << tmp_s.min_price
		    << " max=" << tmp_s.max_price << " buy=" << tmp_s.buy_count
		    << " sell=" << tmp_s.sell_count << '\n';
		tmp_s.reset();
	}
	out.flush();
	return true;
}

bool Aggregator::flushIf(std::ofstream& out) {
	const auto now = std::chrono::steady_clock::now();
	std::unique_lock<std::mutex> lock(m_mutex);
	if (now < m_next_flush) {
		return false;
	}
	m_next_flush = now + m_flush_period;

	const auto wall_clock = std::chrono::system_clock::now();
	return writeSnapshotSync(out, wall_clock);
}

bool Aggregator::forceFlush(std::ofstream& out) {
	std::lock_guard<std::mutex> lock(m_mutex);
	return writeSnapshotSync(out, std::chrono::system_clock::now());
}

} // namespace DataCollector
