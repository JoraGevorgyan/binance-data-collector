#pragma once
#include "Config/Config.hpp"
#include "common/Canceler.hpp"

namespace DataCollector {

class BinanceWebSocketClient {
public:
	explicit BinanceWebSocketClient(const Config::Config& config,
	                                Canceler::Canceler& canceler);
	BinanceWebSocketClient(const BinanceWebSocketClient&) = delete;
	BinanceWebSocketClient& operator=(const BinanceWebSocketClient&) = delete;
	~BinanceWebSocketClient() = default;

	bool runWebSocketSession() noexcept;

private:
	const Config::Config& m_config;
	Canceler::Canceler& m_canceler;
};

} // namespace DataCollector
