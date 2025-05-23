// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

#include <string_view>

namespace DiversionHttp {
	struct HTTPCallResponse {
		HTTPCallResponse() : Contents(""), Error(TOptional<FString>()), Headers(TMap<FString, FString>()), ResponseCode(0) {}
		HTTPCallResponse(const FString& Contents, int32 ResponseCode, TMap<FString, FString> Headers) : Contents(Contents), Error(TOptional<FString>()), Headers(Headers), ResponseCode(ResponseCode) {}
		HTTPCallResponse(const FString& Error) : Contents(""), Error(Error), Headers(TMap<FString, FString>()), ResponseCode(500) {}

		FString Contents;
		TOptional<FString> Error;
		TMap<FString, FString> Headers;
		int32 ResponseCode;
	};

	enum class HttpMethod
	{
		GET,
		POST,
		PUT,
		DEL,
	};


	DIVERSIONHTTP_API FString parameterToString(const FString& value);

	DIVERSIONHTTP_API FString parameterToString(int64 value);

	DIVERSIONHTTP_API FString parameterToString(int32 value);

	DIVERSIONHTTP_API FString parameterToString(float value);

	DIVERSIONHTTP_API FString parameterToString(double value);

	DIVERSIONHTTP_API FString parameterToString(bool value);

	// URL operations

	DIVERSIONHTTP_API FString URLEncode(const FString& input);

	DIVERSIONHTTP_API FString ExtractHostFromUrl(const FString& Url);
	
	DIVERSIONHTTP_API FString ExtractPortFromUrl(const FString& Url);
	
	DIVERSIONHTTP_API bool IsEncrypted(const FString& Url);

	DIVERSIONHTTP_API FString GetPathFromUrl(const FString& Url);

	DIVERSIONHTTP_API FString ConvertToFstring(const std::string_view& InValue, std::size_t InSize = -1);

}
