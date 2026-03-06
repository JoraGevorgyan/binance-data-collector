#pragma once
#include "Aggregator.hpp"
#include "common/Canceler.hpp"
#include "Config/Config.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/algorithm/string.hpp>

namespace DataCollector {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

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
