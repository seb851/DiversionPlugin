// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#pragma once

#include "DiversionUtils.h"
#include "DiversionModule.h"
#include "IDiversionWorker.h"
#include "ISourceControlModule.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#define AUTHORIZATION_HEADER_KEY "Authorization"
#define APP_VERSION_HEADER_KEY "X-DV-App-Version"
#define APP_NAME_HEADER_KEY "X-DV-App-Name"
#define CORRELATION_ID_HEADER_KEY "X-Sentry-Correlation-ID"

constexpr float UseDefaultSystemTimeout = -1.f;
constexpr bool AuthorizedCall = true;
constexpr bool UnauthorizedCall = false;

class DiversionState;

struct SuccessIndication {
	SuccessIndication() : Result(false), RunValidation(false), DelegateDone(false) {}
	bool Result;
	bool RunValidation;
	volatile bool DelegateDone;
};

/* Template definition for synchronous calls to API endpoints*/
template<
	typename Derived,
	typename TRequest,
	typename TResponse,
	typename TDelegate,
	typename TApi,
	FHttpRequestPtr(TApi::* Call)(const TRequest&, const TDelegate&, float)const,
	bool Authorized,
	typename... TArgs>
class TSyncApiCall {
public:
	virtual ~TSyncApiCall() = default;

	static bool CallAPI(const TRequest& Request, FString InUserID, TArgs... Args) {
		TSharedRef<SuccessIndication, ESPMode::ThreadSafe> Success = MakeShared<SuccessIndication, ESPMode::ThreadSafe>();

		// Make sure to have one API instance per call
		// Use shared pointer to make sure it's lived as long as the call is being processed
		TSharedPtr<TApi, ESPMode::ThreadSafe> MyApi = MakeShared<TApi, ESPMode::ThreadSafe>();

		auto Delegate = TDelegate::CreateLambda(
			[&Args..., Success](const TResponse& Response) {
				Success->Result = Derived::ResponseImplementation(Args..., Response);
				Success->RunValidation = true;
				Success->DelegateDone = true;
			});

		float TimeoutSeconds = 5.0f;
		if (Authorized) {
			AddAuthorization(MyApi.Get(), InUserID);
			// Authorized calls are calls to the API (not localhost calls) and might take longer
			TimeoutSeconds = UseDefaultSystemTimeout;
		}
		AddAnalyticsHeaders(MyApi.Get());

		auto RequestRes = ((*MyApi).*Call)(Request, Delegate, TimeoutSeconds);
		const double MaxWaitingTimeout = Authorized ? 121.0 : 5.0;
		double AvoidLogSynchronizationSpammig = 4.0;
		auto CallerName = TypeParseTraits<Derived>::name;
		DiversionUtils::WaitForHttpRequest(MaxWaitingTimeout, RequestRes, CallerName, AvoidLogSynchronizationSpammig);
		UE_LOG(LogSourceControl, Verbose, TEXT("API Call to %hs finished"), CallerName);

		while (!Success->DelegateDone) {
            FPlatformProcess::Sleep(0.1f);
        }

		return Success->Result;
	}

	
	static void AddErrorMessage(const FString& BaseErrMsg, TArray<FString>& OutErrorMessages) {
		auto CallerName = TypeParseTraits<Derived>::name;
		const FString ErrMsg = FString::Printf(TEXT("call to %hs Failed: %s"), CallerName, *BaseErrMsg);
		UE_LOG(LogSourceControl, Error, TEXT("%s"), *ErrMsg);
		OutErrorMessages.Add(ErrMsg);
	}

private:
	static void AddAnalyticsHeaders(TApi* InApi) {
		InApi->AddHeaderParam(APP_VERSION_HEADER_KEY, FDiversionModule::GetPluginVersion());
		InApi->AddHeaderParam(CORRELATION_ID_HEADER_KEY, FGuid::NewGuid().ToString());
		InApi->AddHeaderParam(APP_NAME_HEADER_KEY, FDiversionModule::GetAppName());
	}
	static void AddAuthorization(TApi* InApi, FString InUserID) {
		InApi->AddHeaderParam(AUTHORIZATION_HEADER_KEY, FString::Printf(TEXT("Bearer %s"), *FDiversionModule::Get().GetAccessToken(InUserID)));
	}
};