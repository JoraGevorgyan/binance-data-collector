#pragma once
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include "../common/Canceler.hpp"

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
	explicit Aggregator(std::chrono::seconds flush_period,
	                    std::string output_dir,
	                    Canceler::Canceler& canceler);

	[[nodiscard]] static std::optional<TradeEvent> parseTradeEvent(
	    const std::string& message) noexcept;
	[[nodiscard]] std::thread startFlushWorker() noexcept;
	void update(const TradeEvent& event_msg) noexcept;

private:
	const std::chrono::seconds m_flush_period;
	const std::string m_flush_out_dir;
	Canceler::Canceler& m_canceler;
	std::chrono::steady_clock::time_point m_next_flush;
	std::ofstream m_out;
	std::unordered_map<std::string, TradeStatistics> m_statistics;
	std::mutex m_mutex;
	static std::size_t m_next_stats_file;

private:
	bool writeSnapshotSync() noexcept;
	void flushWorker() noexcept;
	bool isStreamAvailable() noexcept;
};

} // namespace DataCollector
