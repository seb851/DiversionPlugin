// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SslSession.h"
#include "DiversionHttpModule.h"
#include "Types.h"

#include "Misc/Compression.h"
#include "Misc/ScopeLock.h"
#include "Containers/UnrealString.h"

#include <fstream>


using namespace DiversionHttp;

tcp_stream& FHttpSSLSession::TcpStream() {
	return beast::get_lowest_layer(Stream);
}

void FHttpSSLSession::Shutdown() {


	TcpStream().expires_after(RequestTimeout);

	Stream.async_shutdown([this](boost::system::error_code ec) {
		LogTimeoutErrorIfExists(ec);
		if (ec == boost::asio::error::eof)
		{
			// Rationale:
			// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
			ec.assign(0, ec.category());
		}

		// ssl::error::stream_truncated, also known as an SSL "short read",
		// indicates the peer closed the connection without performing the
		// required closing handshake (for example, Google does this to
		// improve performance). Generally this can be a security issue,
		// but if your communication protocol is self-terminated (as
		// it is with both HTTP and WebSocket) then you may simply
		// ignore the lack of close_notify.
		//
		// https://github.com/boostorg/beast/issues/38
		//
		// https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
		//
		// When a short read would cut off the end of an HTTP message,
		// Beast returns the error beast::http::error::partial_message.
		// Therefore, if we see a short read here, it has occurred
		// after the message has been completed, so it is safe to ignore it.
		if (ec != net::ssl::error::stream_truncated)
		{
			UE_LOG(LogDiversionHttp, Error, TEXT("SSL Socket shutdown failed: %hs"), ec.message().c_str());
		}

		// Release the main Run function
		response_promise.set_value(ResponseValue);
	});
}


void FHttpSSLSession::PerformRequest() {
	
	
	if (!SSL_set_tlsext_host_name(Stream.native_handle(), Host.c_str()))
	{
		boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("SNI Handshake error: " + ec.message()).c_str())));
		return;
	}

	Stream.async_handshake(net::ssl::stream_base::client, [&, this](beast::error_code ec) {
		if (ec) {
			response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Handshake error: " + ec.message()).c_str())));
			return;
		}

		TcpStream().expires_after(RequestTimeout);

		http::async_write(Stream, Request, beast::bind_front_handler(&FHttpSession::OnWrite, AsShared()));
	});
}
