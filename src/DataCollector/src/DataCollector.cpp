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
		spdlog::warn("Max threads num {} is too low.");
		return 1;
	}
	return num - busy_workers;
}

} // namespace

WebSocketClient::WebSocketClient(const Config::Config& config,
                                 Canceler::Canceler& canceler)
    : m_config(config), m_canceler(canceler) {}

bool WebSocketClient::runWebSocketSession() noexcept {
	spdlog::info("Trying to create a session");
	std::thread client_session_thread(&WebSocketClient::runClientSession, this);

	DataCollector::Aggregator aggregator(m_config.getStatsFlushPeriod(),
	                                     m_config.getStatsOutputPath(),
	                                     m_canceler);

	auto flush_worker_thread = aggregator.startFlushWorker();
	const auto workers_num = validateWorkersNum(m_config.getMaxThreadsNum());
	std::vector<std::thread> aggregator_threads;
	aggregator_threads.reserve(workers_num);
	spdlog::info("Starting {} aggregator worker threads", workers_num);
	for (std::size_t i = 0; i < workers_num; ++i) {
		aggregator_threads.emplace_back(
		    [&aggregator, this]() { aggregateData(aggregator); });
	}
	spdlog::debug("join other threads");
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
			return;
		}

		if (!SSL_set_tlsext_host_name(native, host.c_str())) {
			const beast::error_code ec{static_cast<int>(::ERR_get_error()),
			                           net::error::get_ssl_category()};
			spdlog::error("SNI setup failed {}: {}", host, ec.message());
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
		beast::error_code ec_close;
		ws_stream.close(websocket::close_code::normal, ec_close);
	} catch (const std::exception& ex) {
		spdlog::error("Connection loop exception: {}", ex.what());
	}
}

void WebSocketClient::receiveAndStore(
    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream) {
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
			spdlog::error("WebSocket read error: {}", ec.message());
			return;
		}
		const auto message = beast::buffers_to_string(buffer.data());
		const std::string* msg_ptr = new (std::nothrow) std::string(message);
		if (msg_ptr == nullptr) {
			spdlog::error("Failed to allocate memory for message copy");
			return;
		}
		spdlog::debug("Received message: {}", message);
		if (!m_blk_queue_str_items.bounded_push(msg_ptr)) {
			delete msg_ptr;
			spdlog::critical("loss of data, queue is full...");
			return;
		}
	}
}

void WebSocketClient::aggregateData(Aggregator& aggregator) noexcept {
	spdlog::debug("Aggregator worker thread started");

	while (!m_canceler.isCanceled()) {
		spdlog::debug("Aggregator worker thread in loop");
		const std::string* cur_message_ptr = nullptr;
		if (!m_blk_queue_str_items.pop(cur_message_ptr) ||
		    cur_message_ptr == nullptr) {
			spdlog::debug("No message to process, sleeping...");
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		const auto trade_event_opt =
		    Aggregator::parseTradeEvent(*cur_message_ptr);

		if (trade_event_opt.has_value()) {
			spdlog::debug("update stats");
			aggregator.update(trade_event_opt.value());
		} else {
			spdlog::warn("Failed to parse message: {}", *cur_message_ptr);
		}
		delete cur_message_ptr;
	}
}

void WebSocketClient::clearQueue() noexcept {
	spdlog::debug("Clearing message queue");
	const std::string* cur_message_ptr = nullptr;
	while (m_blk_queue_str_items.pop(cur_message_ptr)) {
		delete cur_message_ptr;
		cur_message_ptr = nullptr;
	}
}

} // namespace DataCollector
