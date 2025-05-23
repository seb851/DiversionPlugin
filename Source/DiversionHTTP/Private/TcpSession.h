// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#pragma once
#include "HttpSession.h"
#include "BoostHeaders.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

using tcp_stream = beast::tcp_stream;
using ssl_stream = beast::ssl_stream<tcp_stream>;

class FHttpTcpSession final : public FHttpSession<tcp_stream>
{
public:
	explicit FHttpTcpSession(net::io_context& IoContext,
		const std::string& Host,
		const std::string Port,
		const std::chrono::seconds& ConnectionTimeout,
		const std::chrono::seconds& RequestTimeout)
		: FHttpSession<tcp_stream>(IoContext, tcp_stream(net::make_strand(IoContext)), Host, Port, ConnectionTimeout, RequestTimeout)
	{}

private:

	beast::tcp_stream& TcpStream() override {
		return Stream;
	}

};