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
#include <boost/lockfree/stack.hpp>

#include <atomic>
#include <string>
#include <vector>

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
	std::atomic<bool> m_receiver_stopped;
	std::vector<std::vector<std::string>> m_msg_list_arr;

private:
	void runClientSession() noexcept;
	void runClientSessionImpl(const std::string& host,
	                          const std::string& port,
	                          const std::string& target) noexcept;
	void receiveAndStore(
	    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream);
	void aggregateData(Aggregator& aggregator) noexcept;
	bool putMsgToProcess(std::string& msg,
	                     std::optional<uint16_t>& put_index) noexcept;

	static constexpr uint16_t m_perfect_size = 1024;
	boost::lockfree::queue<uint16_t, boost::lockfree::capacity<m_perfect_size>>
	    m_to_process_indices;
	boost::lockfree::stack<uint16_t, boost::lockfree::capacity<m_perfect_size>>
	    m_free_indices;
	uint16_t m_cur_list_limit{200}; // don't let the aggregator wait more
};

} // namespace DataCollector
