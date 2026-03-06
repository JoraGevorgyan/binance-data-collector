#pragma once
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace DataCollector {

namespace {

struct TradeStatistics;
struct TradeEvent;

} // namespace

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
