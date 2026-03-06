#pragma once
#include "Config/Config.hpp"
#include "common/Canceler.hpp"

namespace DataCollector {

class WebSocketClient {
public:
	explicit WebSocketClient(const Config::Config& config,
	                         Canceler::Canceler& canceler);
	WebSocketClient(const WebSocketClient&) = delete;
	WebSocketClient& operator=(const WebSocketClient&) = delete;
	~WebSocketClient() = default;

	bool runWebSocketSession() noexcept;

private:
	const Config::Config& m_config;
	Canceler::Canceler& m_canceler;
	boost::lockfree::queue<std::string> m_str_items{1024}; // can be configured

private:
	void runClientSession() noexcept;
	bool receiveAndStore(
	    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream);
	void aggregateData(std::ofstream& out,
	                   const Aggregator& aggregator) noexcept;
};

} // namespace DataCollector
