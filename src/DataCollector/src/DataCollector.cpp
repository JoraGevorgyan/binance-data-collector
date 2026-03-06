#include "../DataCollector.hpp"
#include "../Aggregator.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lockfree/queue.hpp>

#include <openssl/err.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace DataCollector {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

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

} // namespace

WebSocketClient::WebSocketClient(const Config::Config& config,
                                 Canceler::Canceler& canceler)
    : m_config(config), m_canceler(canceler) {}

bool WebSocketClient::runWebSocketSession() noexcept {
	spdlog::info("Trying to create a session");
	if (!runClient()) {
		spdlog::error("Failed to run WebSocket client");
		return false;
	}

	DataCollector::Aggregator aggregator(m_config.getStatsFlushPeriod());
	std::ofstream outfile("binance_stats.log", std::ios::app);
}

bool WebSocketClient::runClient() noexcept {
	const auto host = m_config.getHostName();
	const auto port = m_config.getPort();
	spdlog::info("Connecting to WebSocket at {}:{}", host, port);

	const auto target = makeStreamPath(m_config.getStreamsList());
	while (!m_canceler.isCanceled()) {
		try {
			net::io_context ioc;
			ssl::context ctx{ssl::context::tls_client};
			ctx.set_default_verify_paths();

			tcp::resolver resolver{ioc};
			auto const results = resolver.resolve(host, port);

			beast::ssl_stream<beast::tcp_stream> ssl_stream{ioc, ctx};
			ssl_stream.set_verify_mode(ssl::verify_peer);
			auto* native = ssl_stream.native_handle();
			if (!native) {
				spdlog::error("Missing native SSL handle for {}", host);
				return false;
			}

			if (!SSL_set_tlsext_host_name(native, host.c_str())) {
				beast::error_code ec{static_cast<int>(::ERR_get_error()),
				                     net::error::get_ssl_category()};
				spdlog::error("SNI setup failed {}: {}", host, ec.message());
				return false;
			}

			beast::get_lowest_layer(ssl_stream).connect(results);
			ssl_stream.handshake(ssl::stream_base::client);

			websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_stream{
			    std::move(ssl_stream)};
			ws_stream.set_option(websocket::stream_base::timeout::suggested(
			    boost::beast::role_type::client));
			ws_stream.set_option(websocket::stream_base::decorator(
			    [](websocket::request_type& req) {
				    req.set(http::field::user_agent,
				            std::string("binance-ws-sample"));
			    }));

			ws_stream.handshake(host, target);
			spdlog::info("Connected to {}{}", host, target);

			if (!receiveAndStore(ws_stream)) {
				spdlog::error("WebSocket session ended with errors");
				return false;
			}

			beast::error_code ec_close;
			ws_stream.close(websocket::close_code::normal, ec_close);
			if (ec_close) {
				spdlog::warn("WebSocket close error: {}", ec_close.message());
				return false;
			} else {
				spdlog::info("WebSocket closed successfully");
				return true;
			}
		} catch (const std::exception& ex) {
			spdlog::error("Connection loop exception: {}", ex.what());
		}
	}
	spdlog::info("WebSocket session ended successfully");
	return true;
}

bool WebSocketClient::receiveAndStore(
    websocket::stream<beast::ssl_stream<beast::tcp_stream>>& ws_stream) {
	const auto reconnection_delay = m_config.getReconnectionDelay();
	const auto timer = std::chrono::steady_clock::now();
	while (!m_canceler.isCanceled()) {
		if (std::chrono::steady_clock::now() - timer >= reconnection_delay) {
			spdlog::warn("Reconnection delay of {} minutes exceeded.",
			             reconnection_delay.count());
			spdlog::warn("Stopping session to reconnect...");
			return true;
		}
		beast::flat_buffer buffer;
		beast::error_code ec;
		ws_stream.read(buffer, ec);
		if (ec) {
			spdlog::error("WebSocket read error: {}", ec.message());
			return false;
		}
		std::string message = beast::buffers_to_string(buffer.data());
		buffer.consume(buffer.size());
		m_str_items.bounded_push(std::move(message));
	}
	return true;
}

} // namespace DataCollector
