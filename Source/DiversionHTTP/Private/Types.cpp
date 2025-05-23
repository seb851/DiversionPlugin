// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "Types.h"

#include "BoostHeaders.h"

// URL is a header only library, including it here separately to avoid duplicate definitions in the DLL
THIRD_PARTY_INCLUDES_START
#include <boost/url.hpp>
#include <boost/url/src.hpp>
#include <boost/url/encode.hpp>
THIRD_PARTY_INCLUDES_END
//

FString DiversionHttp::parameterToString(const FString& value)
{
	return value;
}

FString DiversionHttp::parameterToString(int64 value)
{
	return FString::Printf(TEXT("%lld"), value);
}

FString DiversionHttp::parameterToString(int32 value)
{
	return FString::Printf(TEXT("%d"), value);
}

FString DiversionHttp::parameterToString(float value)
{
	return FString::SanitizeFloat(value);
}

FString DiversionHttp::parameterToString(double value)
{
	return FString::SanitizeFloat(value);
}

FString DiversionHttp::parameterToString(bool value)
{
	return value ? TEXT("true") : TEXT("false");
}

FString DiversionHttp::URLEncode(const FString& input)
{
	std::string TempStdString(TCHAR_TO_UTF8(*input));
	// Create a percent-encoded view of the input
	std::string Encoded = boost::urls::encode( TempStdString.c_str(), boost::urls::unreserved_chars);
	return UTF8_TO_TCHAR(Encoded.c_str());
}


FString DiversionHttp::GetPathFromUrl(const FString& Url) {
	try {
		std::string StdUrl = TCHAR_TO_UTF8(*Url);
		boost::urls::url_view ParsedUrl = boost::urls::parse_uri(StdUrl).value();

		std::string FullPath = ParsedUrl.encoded_path().data();
		// Convert back to FString
		return FString(FullPath.c_str());
	}
	catch (const std::exception& e) {
		// Handle errors and return an empty string
		UE_LOG(LogTemp, Error, TEXT("Error parsing URL: %s"), *FString(e.what()));
		return TEXT("");
	}
}

FString DiversionHttp::ConvertToFstring(const std::string_view& InValue, std::size_t InSize)
{
	std::size_t size = InSize == -1 ? InValue.size() : InSize;
	return FString(UTF8_TO_TCHAR(InValue.data()), InValue.size()).Mid(0, size);
}

FString DiversionHttp::ExtractHostFromUrl(const FString& Url)
{
	std::string StdUrl = TCHAR_TO_UTF8(*Url);
	try {
		boost::urls::url_view ParsedUrl = boost::urls::parse_uri(StdUrl).value();

		return FString(ParsedUrl.host().data());
	}
	catch (const std::exception& e) {
		// Handle errors and return an empty string
		UE_LOG(LogTemp, Error, TEXT("Error parsing URL:%s - %s"), *Url, *FString(e.what()));
		return TEXT("");
	}
}

FString DiversionHttp::ExtractPortFromUrl(const FString& Url) {
	try {
		std::string StdUrl = TCHAR_TO_UTF8(*Url);
		boost::urls::url_view ParsedUrl = boost::urls::parse_uri(StdUrl).value();

		// Check if the URL explicitly specifies a port
		if (ParsedUrl.has_port()) {
			return FString(ParsedUrl.port().data());
		}
		else {
			// Use a switch-like structure for default ports
			const std::string scheme = ParsedUrl.scheme();
			if (scheme == "http") {
				return TEXT("80");
			}
			else if (scheme == "https") {
				return TEXT("443");
			}
			else if (scheme == "ftp") {
				return TEXT("21");
			}
			else if (scheme == "ws") { // WebSocket
				return TEXT("80");
			}
			else if (scheme == "wss") { // Secure WebSocket
				return TEXT("443");
			}
			else {
				return TEXT(""); // Unknown or unsupported scheme
			}
		}
	}
	catch (const std::exception& e) {
		// Handle errors and return an empty string
		UE_LOG(LogTemp, Error, TEXT("Error parsing URL: %s"), *FString(e.what()));
		return TEXT("");
	}
}

bool DiversionHttp::IsEncrypted(const FString& Url) {
	try {
		std::string StdUrl = TCHAR_TO_UTF8(*Url);
		boost::urls::url_view ParsedUrl = boost::urls::parse_uri(StdUrl).value();

		const std::string scheme = ParsedUrl.scheme();
		return scheme == "https" || scheme == "wss"; // HTTPS or secure WebSocket
	}
	catch (const std::exception& e) {
		// Handle errors (e.g., invalid URL)
		UE_LOG(LogTemp, Error, TEXT("Error parsing URL: %s"), *FString(e.what()));
		return false; // Assume not encrypted if the URL is invalid
	}
}
