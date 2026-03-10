#include "../DataCollector.hpp"
#include "../Aggregator.hpp"
#include "spdlog/spdlog.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

#include <openssl/err.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace DataCollector {

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

namespace {

uint16_t g_i_store{0};     // value inited after pop from stack
uint16_t g_i_aggregate{0}; // value inited after pop from queue

std::string makeStreamPath(const std::vector<std::string>& streams) noexcept {
	std::string path = "/stream?streams=";
	for (const auto& stream : streams) {
		path += stream + '/';
	}
	if (!streams.empty()) {
		path.pop_back();
	}
	return path;
}

std::size_t validateWorkersNum(std::size_t num) noexcept {
	constexpr int busy_workers = 3; // main, flush worker, client session
	if (num <= busy_workers) {
		spdlog::warn("Max threads num {} is too low.", num);
		return 1;
	}
	return num - busy_workers;
}

bool isExpectedShutdownError(const beast::error_code& ec) noexcept {
	return ec == websocket::error::closed ||
	       ec == net::error::operation_aborted || ec == net::error::eof ||
	       ec == ssl::error::stream_truncated;
}

template <typename b_lockfree_stack_t>
void putIndicesToStack(b_lockfree_stack_t& dest_stack, uint16_t num_indices) {
	for (auto i = num_indices - 1; i >= 0; --i) {
		if (!dest_stack.push(i)) {
			return; // this should never happen, but it's safe this way
		}
	}
}

template <typename b_lockfree_queue_t>
void waitForQueueDrain(b_lockfree_queue_t& target_queue,
                       Canceler::Canceler& canceler,
                       const std::chrono::milliseconds timeout) noexcept {
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	while (!canceler.isCanceled() &&
	       std::chrono::steady_clock::now() < deadline) {
		if (target_queue.empty()) {
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

} // namespace

WebSocketClient::WebSocketClient(const Config::Config& config,
                                 Canceler::Canceler& canceler)
    : m_config(config),
      m_canceler(canceler),
      m_receiver_stopped{true},
      m_msg_list_arr(m_perfect_size) {}

bool WebSocketClient::runWebSocketSession() noexcept {
	putIndicesToStack(m_free_indices, m_msg_list_arr.size());

	spdlog::info("Starting WebSocket session");
	std::thread client_session_thread(&WebSocketClient::runClientSession, this);
	DataCollector::Aggregator aggregator(m_canceler, m_config.getStatsLogger(),
	                                     m_config.getStatsFlushPeriod());

	auto flush_worker_thread = aggregator.startFlushWorker();
	const auto workers_num = validateWorkersNum(m_config.getMaxThreadsNum());
	std::vector<std::thread> aggregator_threads;
	aggregator_threads.reserve(workers_num);
	spdlog::info("Starting {} aggregator worker threads", workers_num);
	for (std::size_t i = 0; i < workers_num; ++i) {
		aggregator_threads.emplace_back(
		    [&aggregator, this]() { aggregateData(aggregator); });
	}

	spdlog::debug("join other threads to exit");
	client_session_thread.join();
	for (auto& aggregator_thread : aggregator_threads) {
		aggregator_thread.join();
	}
	flush_worker_thread.join();

	spdlog::info("Shutdown complete");
	return true;
}

void WebSocketClient::runClientSession() noexcept {
	const auto host = m_config.getHostName();
	const auto port = m_config.getPort();
	const auto target = makeStreamPath(m_config.getStreamsList());

	spdlog::info("Connecting to WebSocket at {}:{}", host, port);
	auto interval = m_config.getConnectPeriod();
	while (!m_canceler.isCanceled()) {
		if (interval >= m_config.getConnectPeriod()) {
			runClientSessionImpl(host, port, target);
			interval = std::chrono::seconds(0);
		}
		std::this_thread::sleep_for(m_config.getCheckPeriod());
		interval += m_config.getCheckPeriod();
	}
	spdlog::info("WebSocket session ended");
}

void WebSocketClient::runClientSessionImpl(const std::string& host,
                                           const std::string& port,
                                           const std::string& target) noexcept {
	m_receiver_stopped.store(false, std::memory_order_release);
	try {
		net::io_context ioc;
		ssl::context ctx{ssl::context::tls_client};
		ctx.set_default_verify_paths();

		net::ip::tcp::resolver resolver{ioc};
		auto const results = resolver.resolve(host, port);

		beast::ssl_stream<beast::tcp_stream> ssl_stream{ioc, ctx};
		ssl_stream.set_verify_mode(ssl::verify_peer);
		auto native = ssl_stream.native_handle();
		if (native == nullptr) {
			spdlog::error("Missing native SSL handle for {}", host);
			m_receiver_stopped.store(true, std::memory_order_release);
			return;
		}

		if (!SSL_set_tlsext_host_name(native, host.c_str())) {
			const beast::error_code ec{static_cast<int>(::ERR_get_error()),
			                           net::error::get_ssl_category()};
			spdlog::error("SNI setup failed {}: {}", host, ec.message());
			m_receiver_stopped.store(true, std::memory_order_release);
			return;
		}

		beast::get_lowest_layer(ssl_stream).connect(results);
		ssl_stream.handshake(ssl::stream_base::client);

		websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_stream{
		    std::move(ssl_stream)};
		ws_stream.set_option(websocket::stream_base::timeout::suggested(
		    boost::beast::role_type::client));
		ws_stream.set_option(
		    websocket::stream_base::decorator([](websocket::request_type& req) {
			    req.set(http::field::user_agent,
			            std::string("binance-ws-sample"));
		    }));

		ws_stream.handshake(host, target);
		spdlog::info("Connected to {}{}", host, target);

		receiveAndStore(ws_stream);
		m_receiver_stopped.store(true, std::memory_order_release);
		waitForQueueDrain(m_to_process_indices, m_canceler,
		                  std::chrono::milliseconds(1500));

		spdlog::info("Closing WebSocket connection to {}{}", host, target);
		if (!ws_stream.is_open()) {
			spdlog::debug("WebSocket already closed for {}{}", host, target);
			return;
		}
		beast::error_code ec_close;
		ws_stream.close(websocket::close_code::normal, ec_close);
		if (!ec_close) {
			spdlog::info("WebSocket connection closed for {}{}", host, target);
			return;
		}
		if (isExpectedShutdownError(ec_close)) {
			spdlog::info(
			    "WebSocket close completed with expected shutdown state: {}",
			    ec_close.message());
			return;
		}
		spdlog::error("WebSocket close failed ({}): {}", ec_close.value(),
		              ec_close.message());
	} catch (const std::exception& ex) {
		m_receiver_stopped.store(true, std::memory_order_release);
		spdlog::error("Connection loop exception: {}", ex.what());
	}
}

void WebSocketClient::receiveAndStore(
    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream) {
	if (!m_free_indices.pop(g_i_store)) {
		spdlog::critical("No free index available to store incoming message");
		return; // shouldn't happen actually
	}
	const auto reconnection_delay = m_config.getReconnectionDelay();
	const auto timer = std::chrono::steady_clock::now();
	while (!m_canceler.isCanceled()) {
		if (std::chrono::steady_clock::now() - timer >= reconnection_delay) {
			spdlog::warn("Reconnection delay of {} minutes exceeded.",
			             reconnection_delay.count());
			spdlog::warn("Stopping session to reconnect...");
			return;
		}
		beast::flat_buffer buffer;
		beast::error_code ec;
		ws_stream.read(buffer, ec);
		if (ec) {
			if (isExpectedShutdownError(ec)) {
				spdlog::info(
				    "WebSocket read ended due to expected shutdown: {}",
				    ec.message());
				return;
			}
			spdlog::error("WebSocket read error: {}", ec.message());
			return;
		}
		auto message = beast::buffers_to_string(buffer.data());
		spdlog::debug("Received message: {}", message);
		auto& msg_list = m_msg_list_arr[g_i_store];
		msg_list.emplace_back(std::move(message));

		bool is_limit_low = false;
		if (msg_list.size() > m_cur_list_limit) {
			is_limit_low = m_to_process_indices.bounded_push(g_i_store);
		}
		if (is_limit_low) {
			m_cur_list_limit = m_cur_list_limit * 2 + 1;
		}
	}
}

void WebSocketClient::aggregateData(Aggregator& aggregator) noexcept {
	spdlog::debug("Aggregator worker thread started");

	while (true) {
		if (!m_to_process_indices.pop(g_i_aggregate)) {
			if (m_canceler.isCanceled() &&
			    m_receiver_stopped.load(std::memory_order_acquire)) {
				spdlog::debug("Aggregator worker thread break");
				break;
			}
			spdlog::debug("No message to process, sleeping...");
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		for (auto& msg : m_msg_list_arr[g_i_aggregate]) {
			const auto trade_event_opt = Aggregator::parseTradeEvent(msg);

			if (trade_event_opt.has_value()) {
				spdlog::debug("update stats");
				aggregator.update(trade_event_opt.value());
			} else {
				spdlog::warn("Failed to parse message: {}", msg);
			}
		}
		m_msg_list_arr[g_i_aggregate].clear();
		m_free_indices.bounded_push(g_i_aggregate);
	}
	spdlog::debug("Aggregator worker thread exiting after queue drain");
}

} // namespace DataCollector
