#pragma once
#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include "../common/Canceler.hpp"
#include "spdlog/spdlog.h"

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
	explicit Aggregator(Canceler::Canceler& canceler,
	                    const std::weak_ptr<spdlog::logger>& out,
	                    std::chrono::seconds flush_period);
	~Aggregator() = default;

	[[nodiscard]] static std::optional<TradeEvent> parseTradeEvent(
	    const std::string& message) noexcept;
	[[nodiscard]] std::thread startFlushWorker() noexcept;
	void update(const TradeEvent& event_msg) noexcept;

private:
	Canceler::Canceler& m_canceler;
	std::shared_ptr<spdlog::logger> m_out;
	const std::chrono::seconds m_flush_period;
	std::unordered_map<std::string, TradeStatistics> m_statistics;
	std::mutex m_mutex;

private:
	bool writeSnapshotSync() noexcept;
	void flushWorker() noexcept;
};

} // namespace DataCollector
