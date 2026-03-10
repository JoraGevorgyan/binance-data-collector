#pragma once
#include "Aggregator.hpp"
#include "Config/Config.hpp"
#include "common/Canceler.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <string>

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
	~WebSocketClient() { clearQueue(); }

	bool runWebSocketSession() noexcept;

private:
	const Config::Config& m_config;
	Canceler::Canceler& m_canceler;
	std::atomic<bool> m_receiver_stopped{true};
	boost::lockfree::queue<const std::string*, boost::lockfree::capacity<16384>>
	    m_blk_queue_str_items;

private:
	void runClientSession() noexcept;
	void runClientSessionImpl(const std::string& host,
	                          const std::string& port,
	                          const std::string& target) noexcept;
	void receiveAndStore(
	    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream);
	void waitForQueueDrain(std::chrono::milliseconds timeout) noexcept;
	void aggregateData(Aggregator& aggregator) noexcept;
	void clearQueue() noexcept;
};

} // namespace DataCollector
