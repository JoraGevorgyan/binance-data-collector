#include "../Aggregator.hpp"
#include <algorithm>
#include <filesystem>
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

std::optional<std::filesystem::path> getValidFullPath(
    const std::string& dir,
    const std::string& f_name) noexcept {
	namespace fs = std::filesystem;
	std::error_code err_c;
	const fs::path base_path(dir);

	if (!fs::exists(base_path, err_c)) {
		if (!fs::create_directories(base_path, err_c) && err_c) {
			spdlog::error("Failed to create directory {}: {}",
			              base_path.string(), err_c.message());
			return std::nullopt;
		}
	}
	return base_path / f_name;
}

} // namespace

Aggregator::Aggregator(std::chrono::seconds flush_period,
                       std::string output_dir,
                       Canceler::Canceler& canceler)
    : m_flush_period(flush_period),
      m_flush_out_dir(std::move(output_dir)),
      m_canceler(canceler) {}

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

std::size_t Aggregator::m_next_stats_file = 1;

bool Aggregator::isStreamAvailable() noexcept {
	if (m_out.is_open()) {
		return true;
	}
	// TODO: check currenttly open file size, and if needed create a new one
	const auto n_name = std::to_string(m_next_stats_file) + "_statistics.log";
	++m_next_stats_file;
	if (m_next_stats_file >= 1000) { // configure
		spdlog::critical("Too many stats files, cannot create new one");
		return false;
	}
	const auto full_path = getValidFullPath(m_flush_out_dir, n_name);
	m_out.open(full_path.value_or(n_name), std::ios::out | std::ios::app);
	return m_out.is_open();
}

bool Aggregator::writeSnapshotSync() noexcept {
	const std::unique_lock<std::mutex> lock(m_mutex);
	if (!isStreamAvailable()) {
		spdlog::critical("cannot dump statistics to a file");
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
	auto next_flush = std::chrono::steady_clock::now();
	while (!m_canceler.isCanceled()) {
		spdlog::info("flush worker thread in progress");
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
