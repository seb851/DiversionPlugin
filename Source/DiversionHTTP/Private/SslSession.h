// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#pragma once
#include "HttpSession.h"
#include "BoostHeaders.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

using tcp_stream = beast::tcp_stream;
using ssl_stream = beast::ssl_stream<tcp_stream>;


class FHttpSSLSession final : public FHttpSession<ssl_stream>
{
public:
	explicit FHttpSSLSession(net::io_context& IoContext,
		net::ssl::context& SslContext,
		const std::string& Host,
		const std::string Port,
		const std::chrono::seconds& ConnectionTimeout,
		const std::chrono::seconds& RequestTimeout)
		: FHttpSession<ssl_stream>(IoContext, ssl_stream(net::make_strand(IoContext), SslContext), Host, Port, ConnectionTimeout, RequestTimeout)
	{}

private:

	tcp_stream& TcpStream() override;

	void Shutdown() override;

	void PerformRequest() override;

};
