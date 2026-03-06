#pragma once
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace DataCollector {

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

struct TradeEvent {
	std::string symbol;
	double price{0.0};
	double quantity{0.0};
	bool is_buyer_or_maker{true};
};

class Aggregator {
public:
	explicit Aggregator(std::chrono::seconds flush_period);

	[[nodiscard]] static std::optional<TradeEvent> parseTradeEvent(
	    const std::string& message) noexcept;
	void update(const TradeEvent& event_msg) noexcept;
	[[nodiscard]] bool flushIf(std::ofstream& out) noexcept;
	[[nodiscard]] bool forceFlush(std::ofstream& out) noexcept;

private:
	std::mutex m_mutex;
	std::map<std::string, TradeStatistics> m_statistics;
	const std::chrono::seconds m_flush_period;
	std::chrono::steady_clock::time_point m_next_flush;

private:
	bool writeSnapshotSync(
	    std::ofstream& out,
	    std::chrono::system_clock::time_point wall_clock) noexcept;
};

} // namespace DataCollector
