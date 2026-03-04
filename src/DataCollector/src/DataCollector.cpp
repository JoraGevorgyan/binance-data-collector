#include "../DataCollector.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <openssl/err.h>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace DataCollector {

namespace {

std::string toLowerCopy(std::string value) {
	std::transform(
	    value.begin(), value.end(), value.begin(),
	    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return value;
}

std::vector<std::string> parseStreams(const Config::Config& config) {
	std::vector<std::string> streams;
	const char* raw = std::getenv("BINANCE_STREAMS");
	if (raw != nullptr && *raw != '\0') {
		const std::string input(raw);
		std::size_t start = 0;
		while (start < input.size()) {
			const auto end = input.find(',', start);
			auto token =
			    input.substr(start, end == std::string::npos ? std::string::npos
			                                                 : end - start);

			// trim spaces
			const auto l = token.find_first_not_of(" \t\r\n");
			const auto r = token.find_last_not_of(" \t\r\n");
			if (l != std::string::npos && r != std::string::npos) {
				token = token.substr(l, r - l + 1);
				token = toLowerCopy(token);
				if (token.find('@') == std::string::npos) {
					token += "@trade";
				}
				streams.push_back(token);
			}

			if (end == std::string::npos) {
				break;
			}
			start = end + 1;
		}
	}

	if (streams.empty()) {
		for (const auto& symbol : config.getSymbols()) {
			if (symbol.empty()) {
				continue;
			}

			auto token = toLowerCopy(symbol);
			if (token.find('@') == std::string::npos) {
				token += "@trade";
			}
			streams.push_back(std::move(token));
		}
	}

	if (streams.empty()) {
		streams.push_back("btcusdt@trade");
	}
	return streams;
}

std::string buildCombinedTarget(const std::vector<std::string>& streams) {
	std::string target = "/stream?streams=";
	for (std::size_t i = 0; i < streams.size(); ++i) {
		if (i != 0) {
			target.push_back('/');
		}
		target += streams[i];
	}
	return target;
}

} // namespace

BinanceWebSocketClient::BinanceWebSocketClient(const Config::Config& config,
                                               Canceler::Canceler& canceler)
    : m_config(config), m_canceler(canceler) {}

bool BinanceWebSocketClient::runWebSocketSession() noexcept {
	namespace net = boost::asio;
	namespace ssl = net::ssl;
	namespace beast = boost::beast;
	namespace websocket = beast::websocket;
	using namespace std::chrono_literals;

	const auto streams = parseStreams(m_config);
	const std::string host = "stream.binance.com";
	const std::string port = "9443";
	const std::string target = buildCombinedTarget(streams);

	int consecutive_failures = 0;

	while (!m_canceler.isCanceled()) {
		try {
			net::io_context ioc;
			ssl::context ssl_context{ssl::context::tls_client};
			ssl_context.set_default_verify_paths();
			ssl_context.set_verify_mode(ssl::verify_peer);

			websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws{
			    ioc, ssl_context};

			if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(),
			                              host.c_str())) {
				throw beast::system_error(
				    beast::error_code{static_cast<int>(::ERR_get_error()),
				                      net::error::get_ssl_category()});
			}

			net::ip::tcp::resolver resolver{ioc};
			const auto endpoints = resolver.resolve(host, port);

			beast::get_lowest_layer(ws).expires_after(30s);
			beast::get_lowest_layer(ws).connect(endpoints);

			ws.next_layer().handshake(ssl::stream_base::client);

			ws.set_option(websocket::stream_base::timeout::suggested(
			    beast::role_type::client));
			ws.set_option(websocket::stream_base::decorator(
			    [](websocket::request_type& req) {
				    req.set(beast::http::field::user_agent,
				            "binance-data-collector");
			    }));

			// Binance requires pong payload to match ping payload.
			ws.control_callback(
			    [&ws](websocket::frame_type kind, beast::string_view payload) {
				    if (kind != websocket::frame_type::ping) {
					    return;
				    }

				    websocket::ping_data pong_payload;
				    const auto copy_size =
				        std::min(payload.size(), pong_payload.max_size());
				    pong_payload.assign(payload.data(), copy_size);
				    beast::error_code pong_error;
				    ws.pong(pong_payload, pong_error);
				    if (pong_error) {
					    spdlog::warn("Failed to send pong: {}",
					                 pong_error.message());
				    }
			    });

			ws.handshake(host, target);
			spdlog::info("WebSocket connected: {}{}", host, target);

			consecutive_failures = 0;
			const auto session_start = std::chrono::steady_clock::now();
			beast::flat_buffer buffer;

			while (!m_canceler.isCanceled()) {
				// Binance connection validity: 24h max.
				if (std::chrono::steady_clock::now() - session_start >=
				    23h + 55min) {
					spdlog::info("Reconnecting WebSocket before 24h limit");
					break;
				}

				buffer.clear();
				beast::error_code ec;
				ws.read(buffer, ec);

				if (ec == websocket::error::closed) {
					break;
				}
				if (ec) {
					throw beast::system_error(ec);
				}

				const std::string payload =
				    beast::buffers_to_string(buffer.data());

				// Keep processing compact: parse once, log protocol errors,
				// ignore ACKs.
				const auto json =
				    nlohmann::json::parse(payload, nullptr, false);
				if (json.is_discarded()) {
					spdlog::warn("Invalid JSON payload from Binance");
					continue;
				}
				if (json.is_object() && json.contains("code") &&
				    json.contains("msg")) {
					spdlog::error("Binance stream error: {}", payload);
					continue;
				}
				if (json.is_object() && json.contains("result")) {
					// subscription/property ACK
					continue;
				}

				spdlog::debug("Binance event: {}", payload);
			}

			beast::error_code close_ec;
			ws.close(websocket::close_code::normal, close_ec);
		} catch (const std::exception& ex) {
			++consecutive_failures;
			spdlog::error("WebSocket session failure ({}): {}",
			              consecutive_failures, ex.what());

			if (m_canceler.isCanceled()) {
				return true;
			}
			if (consecutive_failures >= 3) {
				return false;
			}
			std::this_thread::sleep_for(1s);
		}
	}

	return true;
}

} // namespace DataCollector
