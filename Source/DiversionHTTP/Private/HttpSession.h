// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#pragma once
#include "BoostHeaders.h"
#include "Types.h"
#include "DiversionHttpModule.h"

#include <iostream>
#include <fstream>
#include <cstdio> 
#include <string>
#include <future>

namespace beast = boost::beast;
namespace http = beast::http;         
namespace net = boost::asio;          

using tcp_stream = beast::tcp_stream;
using ssl_stream = beast::ssl_stream<tcp_stream>;


// Helper functions
template <typename ResponseType>
TMap<FString, FString> ExtractResponseHeaders(const http::response<ResponseType>& InResponse) {
	TMap<FString, FString> Headers;
	for (auto const& header : InResponse) {
		auto HeaderNameStringView = boost::beast::http::to_string(header.name());
		FString HeaderName = DiversionHttp::ConvertToFstring(HeaderNameStringView.data());
		FString HeaderValue = DiversionHttp::ConvertToFstring(header.value().data(), header.value().size());
		HeaderName = HeaderName.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(""));
		HeaderValue = HeaderValue.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(""));
		Headers.Add(HeaderName, HeaderValue);
	}

	return Headers;
}

bool DecompressGzipWithZlib(const std::string& CompressedFilePath, const std::string& DecompressedFilePath, std::string& OutError);

void ConvertHttpResponseToTArray(const http::response<http::string_body>& HttpResponse, TArray<uint8>& OutArray);

TArray<uint8> DecompressGzipFromArray(const TArray<uint8>& CompressedData, int32 ExpectedDecompressedSize);

uint32 GetUncompressedSizeFromGzip(const TArray<uint8>& GzipData);


template <typename StreamType>
class FHttpSession : public TSharedFromThis<FHttpSession<StreamType>>
{
public:
	explicit FHttpSession(net::io_context& IoContext,
		StreamType Stream,
		const std::string& Host,
		const std::string Port,
		const std::chrono::seconds& ConnectionTimeout,
		const std::chrono::seconds& RequestTimeout)
		: ConnectionTimeout(ConnectionTimeout), RequestTimeout(RequestTimeout), Compression(),
		  Stream(std::move(Stream)), Host(Host),
		  Port(Port), IoContext(IoContext), Resolver(net::make_strand(IoContext))
	{
	}

	virtual ~FHttpSession() = default;

	DiversionHttp::HTTPCallResponse Run(const http::request<http::string_body>& InRequest, FString InOutputFilePath = "");
	
	void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

protected:

	virtual tcp_stream& TcpStream() = 0;
	virtual void Shutdown();
	virtual void PerformRequest();	

	void LogTimeoutErrorIfExists(const beast::error_code& ec) const;
	
private:

	void OnResolve(beast::error_code ec, net::ip::tcp::resolver::results_type results);
	void OnConnect(beast::error_code ec, net::ip::tcp::resolver::results_type::endpoint_type);
	void OnReadStringResponse(beast::error_code ec, std::size_t bytes_transferred);
	void OnReadFileResponseHeaders(beast::error_code ec, std::size_t bytes_transferred);
	void OnReadFileResponseBody(beast::error_code ec, std::size_t bytes_transferred);


protected:
	std::chrono::seconds ConnectionTimeout;
	std::chrono::seconds RequestTimeout;

	beast::flat_buffer Buffer;
	http::request<http::string_body> Request;
	http::response<http::string_body> Response;
	enum class CompressionType
	{
		None,
		Gzip
	} Compression;
	FString CompressedFilePath;
	FString OutputFilePath;
	http::response_parser<http::file_body> FileResponse;

	// Value retrieval mechanism
	DiversionHttp::HTTPCallResponse ResponseValue;
	std::promise<DiversionHttp::HTTPCallResponse> response_promise;

	StreamType Stream;
	const std::string Host;
	const std::string Port;

private:
	net::io_context& IoContext;
	net::ip::tcp::resolver  Resolver;
	std::chrono::time_point<std::chrono::system_clock> StartTime;
};

using namespace DiversionHttp;

template <typename StreamType>
HTTPCallResponse FHttpSession<StreamType>::Run(const http::request<http::string_body>& InRequest, FString InOutputFilePath)
{
	// Support requests to be saved to a file
	OutputFilePath = InOutputFilePath;

	Request = InRequest;
	auto response_future = response_promise.get_future();
	StartTime = std::chrono::system_clock::now();
	Resolver.async_resolve(
		Host,
		Port,
		beast::bind_front_handler(&FHttpSession<StreamType>::OnResolve, this->AsShared()));

	// Wait until the request completes
	return response_future.get();
}


template <typename StreamType>
void FHttpSession<StreamType>::OnResolve(beast::error_code ec, net::ip::tcp::resolver::results_type results)
{
	if (ec) {
		std::string HostAndPort = Host + ":" + Port;
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Resolve error: " + ec.message() + " - " + HostAndPort).c_str())));
		return;
	}

	// Set a timeout on the operation
	TcpStream().expires_after(ConnectionTimeout);
	TcpStream().async_connect(results,
		beast::bind_front_handler(&FHttpSession<StreamType>::OnConnect, this->AsShared()));
}


template <typename StreamType>
void FHttpSession<StreamType>::OnConnect(beast::error_code ec, net::ip::tcp::resolver::results_type::endpoint_type)
{
	if (ec) {
		LogTimeoutErrorIfExists(ec);
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Connect error: " + ec.message()).c_str())));
		return;
	}

	PerformRequest();
}


template <typename StreamType>
void FHttpSession<StreamType>::PerformRequest()
{
	TcpStream().expires_after(RequestTimeout);

	http::async_write(Stream, Request, beast::bind_front_handler(&FHttpSession<StreamType>::OnWrite, this->AsShared()));
}


template <typename StreamType>
void FHttpSession<StreamType>::OnWrite(beast::error_code ec, std::size_t bytes_transferred) {
	
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		LogTimeoutErrorIfExists(ec);
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Write error: " + ec.message()).c_str())));
		return;
	}

	if (OutputFilePath.IsEmpty())
	{
		// Parse the response directly into a string
		http::async_read(Stream, Buffer, Response, beast::bind_front_handler(&FHttpSession::OnReadStringResponse, this->AsShared()));
	}
	else
	{
		beast::error_code file_ec;
		CompressedFilePath = FPaths::GetPath(OutputFilePath) / TEXT("compressed_response.gz");
		FileResponse.get().body().open(TCHAR_TO_UTF8(*CompressedFilePath), beast::file_mode::write, file_ec);
		if (file_ec) {
			response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Failed opening output file: " + ec.message()).c_str())));
			return;
		}

		http::async_read_header(Stream, Buffer, FileResponse, beast::bind_front_handler(&FHttpSession<StreamType>::OnReadFileResponseHeaders, this->AsShared()));
	}
}


template <typename StreamType>
void FHttpSession<StreamType>::OnReadStringResponse(beast::error_code ec, std::size_t bytes_transferred)
{
	
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		LogTimeoutErrorIfExists(ec);
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("String read error: " + ec.message()).c_str())));
		return;
	}

	TArray<uint8> ResponseBody;
	if (Response["Content-Encoding"] == "gzip") {
		TArray<uint8> DecompressedBody;
		ConvertHttpResponseToTArray(Response, ResponseBody);
		DecompressedBody = DecompressGzipFromArray(ResponseBody, GetUncompressedSizeFromGzip(ResponseBody));
		if (DecompressedBody.IsEmpty()) {
			response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR("Failed to decompress response body")));
			return;
		}
		// If the compression succeeded return the uncompressed body
		ResponseValue = HTTPCallResponse(FString(reinterpret_cast<const TCHAR*>(DecompressedBody.GetData()), DecompressedBody.Num()),
			Response.result_int(), ExtractResponseHeaders(Response));
	}
	else {
		ResponseValue = HTTPCallResponse(UTF8_TO_TCHAR(Response.body().c_str()), Response.result_int(), ExtractResponseHeaders(Response));
	}

	this->Shutdown();
}


template <typename StreamType>
void FHttpSession<StreamType>::OnReadFileResponseHeaders(beast::error_code ec, std::size_t bytes_transferred) {
	
	boost::ignore_unused(bytes_transferred);
	
	if (ec) {
		LogTimeoutErrorIfExists(ec);
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("File headers read error: " + ec.message()).c_str())));
		return;
	}

	// Set the content encoding according to the headers values
	const boost::beast::string_view ContentEncoding = FileResponse.get()[http::field::content_encoding];
	if (ContentEncoding == "gzip") {
		Compression = CompressionType::Gzip;
	}
	else if (ContentEncoding == "") {
		Compression = CompressionType::None;
	}
	else {
		response_promise.set_value(HTTPCallResponse(FString("File headers read error: Unsupported content encodng")));
		return;
	}

	http::async_read(Stream, Buffer, FileResponse, beast::bind_front_handler(&FHttpSession<StreamType>::OnReadFileResponseBody, this->AsShared()));
}


template <typename StreamType>
void FHttpSession<StreamType>::OnReadFileResponseBody(beast::error_code ec, std::size_t bytes_transferred) {
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		LogTimeoutErrorIfExists(ec);
		response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("File read error: " + ec.message()).c_str())));
		return;
	}

	FileResponse.get().body().close();

	if (Compression == CompressionType::Gzip) {
		std::string DecompressionError;
		bool res = DecompressGzipWithZlib(TCHAR_TO_UTF8(*CompressedFilePath), TCHAR_TO_UTF8(*OutputFilePath), DecompressionError);
		if (!res) {
			response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Decompression error: " + DecompressionError).c_str())));
			return;
		}
	}
	else if (Compression == CompressionType::None) {
		// Rename the compressed file to the output file
		int result = std::rename(TCHAR_TO_UTF8(*CompressedFilePath), TCHAR_TO_UTF8(*OutputFilePath));
		if (result != 0) {
			std::error_code rename_ec(errno, std::system_category());
			std::string errorMessage = rename_ec.message();
			response_promise.set_value(HTTPCallResponse(UTF8_TO_TCHAR(("Failed to rename file: " + errorMessage).c_str())));
			return;
		}
	}
	else {
		response_promise.set_value(HTTPCallResponse(TEXT("Failed Decompression downloaded file. Unspported compression format.")));
		return;
	}
	
	// Return the result path to the output file as a validation mechanism
	ResponseValue = HTTPCallResponse(OutputFilePath, Response.result_int(), ExtractResponseHeaders(Response));

	this->Shutdown();
}

template <typename StreamType>
void FHttpSession<StreamType>::LogTimeoutErrorIfExists(const beast::error_code& ec) const
{
	if(ec == boost::beast::error::timeout){
		const auto DeltaTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now() - StartTime).count();
		
		std::string TargetUrl = Request.target().data(); 
		UE_LOG(LogDiversionHttp, Error, TEXT("Call to url: %hs timed out after %lld seconds"),
			TargetUrl.c_str(), DeltaTimeSeconds);
	}
}


template <typename StreamType>
void FHttpSession<StreamType>::Shutdown()
{
	// Gracefully close the socket
	if (TcpStream().socket().is_open())
	{
		beast::error_code ec;
		TcpStream().socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);
		// not_connected happens sometimes so don't bother reporting it.
		if (ec && ec != beast::errc::not_connected)
		{
			auto Error = beast::system_error{ ec };
			UE_LOG(LogDiversionHttp, Error, TEXT("Socket shutdown failed: %s"), UTF8_TO_TCHAR(Error.what()));
		}
	}
	else {
		UE_LOG(LogDiversionHttp, Warning, TEXT("Attempted to shutdown a closed or an uninitialized socket"));
	}

	// Release the main Run function
	response_promise.set_value(ResponseValue);
}
