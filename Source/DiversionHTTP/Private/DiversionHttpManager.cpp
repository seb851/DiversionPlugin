// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#include "DiversionHttpManager.h"
#include "DiversionHttpModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#include "HttpSession.h"
#include "SslSession.h"
#include "TcpSession.h"
#include "BoostHeaders.h"

#include <string>
#include <future>
#include <functional>
#include <atomic>
#include <stdexcept>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>

using namespace DiversionHttp;


class IoContextManager {
public:
	IoContextManager() :
		IoContext(boost::asio::io_context()),
		IsRunning(false)
	{}

	~IoContextManager()
	{
		if(IsRunning) {
			Stop();
			Join();
		}
	}

	void Start(std::size_t InThreadCount = 1) {
		if(IsRunning) {
			return;
		}

		IsRunning.store(true);
		WorkGuard = MakeUnique<WorkGuardType>(boost::asio::make_work_guard(IoContext));

		Threads.Reserve(InThreadCount);
		for(std::size_t i = 0; i < InThreadCount; ++i) {
			Threads.Emplace([this]() { this->RunLoop(); });
		}
	}

	void Stop() {
		if(!IsRunning) {
			return;
		}

		IsRunning.store(false);

		if(WorkGuard) {
			WorkGuard->reset();
		}
		WorkGuard.Reset();
		IoContext.stop();
	}

	void Join() {
		for(auto& Thread : Threads) {
			if(Thread.joinable()) {
				Thread.join();
			}
		}
		Threads.Empty();
	}

	boost::asio::io_context& GetIoContext() {
		return IoContext;
	}

private:
	using WorkGuardType = boost::asio::executor_work_guard<
		boost::asio::io_context::executor_type>;

	TUniquePtr<WorkGuardType> WorkGuard;
	TArray<std::thread> Threads;
	boost::asio::io_context IoContext;
	std::atomic<bool> IsRunning;

private:
	void RunLoop() {
		while (IsRunning.load()) {
			try {
				IoContext.run();
				// If the context finished running due to no more work, we should restart it and continue
				if (IoContext.stopped() && IsRunning) {
					UE_LOG(LogDiversionHttp, Warning, TEXT("IoContext stopped due to no more work, restarting"));
					IoContext.restart();
				}
			}
			catch (const std::exception& Ex) {
				UE_LOG(LogDiversionHttp, Error, TEXT("Error in HTTP io context: %s"), UTF8_TO_TCHAR(Ex.what()));
			}
			catch(...) {
				UE_LOG(LogDiversionHttp, Error, TEXT("Unknown error in HTTP io context"));
				IoContext.stop();
				break;
			}
		}
	}
};


// Internal implementation class
class FHttpRequestManagerImpl
{
	typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> WorkGuardType;

public:
	FHttpRequestManagerImpl(const FString& InHost, const FString& InPort, bool UseSSL, int HttpVersion)
		: SslContext(MakeUnique<net::ssl::context>(net::ssl::context::tlsv12_client)),
		Host(TCHAR_TO_UTF8(*InHost)),
		Port(TCHAR_TO_UTF8(*InPort)),
		UseSSL(UseSSL),
		httpVersion(HttpVersion)
	{
		// Configure context anyway in case ssl config is activated later
		ConfigureSslContext();
		IoContextManager.Start();
	}

	~FHttpRequestManagerImpl()
	{
		IoContextManager.Stop();
		IoContextManager.Join();
	}

	HTTPCallResponse SendRequest(
		const FString& Url,
		DiversionHttp::HttpMethod Method,
		const FString& Token,
		const FString& ContentType,
		const FString& Content,
		const TMap<FString, FString>& Headers,
		const int ConnectionTimeoutSeconds,
		const int RequestTimeoutSeconds,
		const FString& OutputFilePath = TEXT(""))
	{
		try {
			http::request<http::string_body> Request; 
			BuildRequset(Request, Url, Method, Token, ContentType, Content, Headers);

			// Sessions are created and destroyed for each request
			HTTPCallResponse Response;
			if (UseSSL) {
				auto Session =
					MakeShared<FHttpSSLSession>(IoContextManager.GetIoContext(), *SslContext, Host, Port,
						std::chrono::seconds(ConnectionTimeoutSeconds),
						std::chrono::seconds(RequestTimeoutSeconds));
				Response = Session->Run(Request, OutputFilePath);
			}
			else {
				auto Session =
					MakeShared<FHttpTcpSession>(IoContextManager.GetIoContext(), Host, Port,
						std::chrono::seconds(ConnectionTimeoutSeconds),
						std::chrono::seconds(RequestTimeoutSeconds));
				Response = Session->Run(Request, OutputFilePath);
			}

			return Response;
		}
		catch (const std::exception& Ex) {
			return HTTPCallResponse(UTF8_TO_TCHAR(Ex.what()));
		}
	}

	void SetPort(const FString& InPort)
	{
		Port = TCHAR_TO_UTF8(*InPort);
	}

	void SetHost(const FString& InHost)
	{
		Host = TCHAR_TO_UTF8(*InHost);
	}

	void SetUseSSL(bool InUseSSL)
	{
		UseSSL = InUseSSL;
	}

private:
	void ConfigureSslContext() const
	{
		SslContext->set_default_verify_paths();
		SslContext->set_verify_mode(net::ssl::verify_peer);

		// Disable older protocols.
		SslContext->set_options(
			net::ssl::context::default_workarounds
			| net::ssl::context::no_sslv2
			| net::ssl::context::no_sslv3
			| net::ssl::context::single_dh_use);

#if PLATFORM_WINDOWS
		// OpenSSL doesn't work with Windows Certificate Store, so we need to provide the certificate manually
		FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("Diversion"))->GetBaseDir();
		FString CertPath = FPaths::Combine(PluginDir, TEXT("Resources"), TEXT("cacert.pem"));
		SslContext->load_verify_file(TCHAR_TO_UTF8(*CertPath));
#endif
	}
	
	boost::beast::http::verb ExtractHttpVerb(DiversionHttp::HttpMethod Method) const
	{
		switch (Method)
		{
		case DiversionHttp::HttpMethod::GET:
			return http::verb::get;
		case DiversionHttp::HttpMethod::POST:
			return http::verb::post;
		case DiversionHttp::HttpMethod::PUT:
			return http::verb::put;
		case DiversionHttp::HttpMethod::DEL:
			return http::verb::delete_;
		default:
			throw std::runtime_error("Invalid or unsupported HTTP method");
		}
	}

	void BuildRequset(http::request<http::string_body>& OutRequest,
		const FString& Url, DiversionHttp::HttpMethod Method, const FString& Token, 
		const FString& ContentType, const FString& Content, 
		const TMap<FString, FString>& Headers) const {
		OutRequest.version(httpVersion);
		OutRequest.method(ExtractHttpVerb(Method));
		OutRequest.target(TCHAR_TO_UTF8(*Url));
		OutRequest.set(http::field::host, Host);
		OutRequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		OutRequest.set(http::field::content_type, TCHAR_TO_UTF8(*ContentType));

		if (!Token.IsEmpty()) {
			OutRequest.set(http::field::authorization, "Bearer " + std::string(TCHAR_TO_UTF8(*Token)));
		}

		// Set the extra headers
		for (const auto& Header : Headers) {
			OutRequest.set(TCHAR_TO_UTF8(*Header.Key), TCHAR_TO_UTF8(*Header.Value));
		}

		if (Method == DiversionHttp::HttpMethod::POST || Method == DiversionHttp::HttpMethod::PUT) {
			OutRequest.body() = TCHAR_TO_UTF8(*Content);
			OutRequest.prepare_payload();
		}
	}

private:
	IoContextManager IoContextManager;
	TUniquePtr<boost::asio::ssl::context> SslContext;

	std::string Host;
	std::string Port;
	bool UseSSL;

	int httpVersion;
};


namespace DiversionHttp {
	
	FHttpRequestManager::FHttpRequestManager(const FString& HostUrl, const TMap<FString, FString>& DefaultHeaders, int HttpVersion) :
		Impl(MakeUnique<FHttpRequestManagerImpl>(ExtractHostFromUrl(HostUrl), ExtractPortFromUrl(HostUrl),
			IsEncrypted(HostUrl), HttpVersion)),
		DefaultHeaders(DefaultHeaders)
	{}

	FHttpRequestManager::FHttpRequestManager(const FString& Host, const FString& Port, const TMap<FString, FString>& DefaultHeaders,
		bool UseSSL, int HttpVersion) :
		Impl(MakeUnique<FHttpRequestManagerImpl>(Host, Port, UseSSL, HttpVersion)),
		 DefaultHeaders(DefaultHeaders)
	{}

	FHttpRequestManager::~FHttpRequestManager() = default;

	HTTPCallResponse FHttpRequestManager::SendRequest(const FString& Url, DiversionHttp::HttpMethod Method, const FString& Token,
		const FString& ContentType, const FString& Content, const TMap<FString, FString>& Headers,
		int ConnectionTimeoutSeconds, int RequestTimeoutSeconds) const 
	{
		TMap<FString, FString> RequestHeaders;
		RequestHeaders.Append(DefaultHeaders);
		RequestHeaders.Append(Headers);
		return Impl->SendRequest(Url, Method, Token, ContentType, Content,
			RequestHeaders, ConnectionTimeoutSeconds, RequestTimeoutSeconds);
	}
	HTTPCallResponse FHttpRequestManager::DownloadFileFromUrl(const FString& OutputFilePath, const FString& Url, const FString& Token, 
		const TMap<FString, FString>& Headers, int ConnectionTimeoutSeconds, int RequestTimeoutSeconds) const
	{
		TMap<FString, FString> FileHeaders = {
			{TEXT("Accept"), TEXT("text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7")},
			{TEXT("Accept-Encoding"), TEXT("gzip, deflate, br, zstd")}
		};

		TMap<FString, FString> RequestHeaders;
		RequestHeaders.Append(FileHeaders);
		RequestHeaders.Append(DefaultHeaders);
		RequestHeaders.Append(Headers);
		
		return Impl->SendRequest(Url, HttpMethod::GET, Token, TEXT("application/octet-stream"), "",
			RequestHeaders, ConnectionTimeoutSeconds, RequestTimeoutSeconds, OutputFilePath);
	}
	void FHttpRequestManager::SetHost(const FString& Host) const 
	{
		Impl->SetHost(Host);
	}
	void FHttpRequestManager::SetPort(const FString& Port) const 
	{
		Impl->SetPort(Port);
	}
	void FHttpRequestManager::SetUseSSL(const bool UseSSL) const 
	{
		Impl->SetUseSSL(UseSSL);
	}

	void FHttpRequestManager::SetDefaultHeaders(const TMap<FString, FString>& Headers)
	{
		DefaultHeaders = Headers;
	}
}
