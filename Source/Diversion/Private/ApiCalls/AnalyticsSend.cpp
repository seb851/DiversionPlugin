// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIAnalyticsApi.h"
#include "OpenAPIAnalyticsApiOperations.h"
#include "DiversionOperations.h"
#include "Internationalization/Culture.h"

#include "Internationalization/Internationalization.h"

using namespace CoreAPI;

class FAnalyticsIngest;
using DerivedClass = FAnalyticsIngest;
using TApi = OpenAPIAnalyticsApi;
using ApiRequest = OpenAPIAnalyticsApi::SrcHandlersAnalyticsIngestRequest;
using ApiResponse = OpenAPIAnalyticsApi::SrcHandlersAnalyticsIngestResponse;
using ApiDelegate = OpenAPIAnalyticsApi::FSrcHandlersAnalyticsIngestDelegate;


using FAnalyticsAPI = TSyncApiCall<
	DerivedClass, ApiRequest, ApiResponse, ApiDelegate,
	TApi, &TApi::SrcHandlersAnalyticsIngest,
	AuthorizedCall,
	// Output parameters
	FDiversionWorkerRef, /*Worker*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/
	>;


class FAnalyticsIngest final : public FAnalyticsAPI {
	friend FAnalyticsAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionWorkerRef InOutWorker,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const ApiResponse& Response);
};
REGISTER_PARSE_TYPE(FAnalyticsIngest);


bool FAnalyticsIngest::ResponseImplementation(FDiversionWorkerRef InOutWorker, 
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const ApiResponse& Response) {
	if (!Response.IsSuccessful() || (Response.GetHttpResponseCode() != EHttpResponseCodes::NoContent)) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	OutInfoMessages.Add("Successfuly sent anayltics event");
	return true;
}


bool DiversionUtils::SendAnalyticsEvent(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const TMap<
                                        FString, FString>& InProperties)
{
	TSharedRef<FSendAnalytics, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FSendAnalytics>(InCommand.Operation);
	ApiRequest Request = ApiRequest();
	CoreAPI::OpenAPIAnalyticsEvent Event = OpenAPIAnalyticsEvent();
	
	Event.Event = Operation->GetEventName().ToString();
	Event.Source = "UE";
	for (auto& Elem : InProperties) {
		Event.Properties.Add(Elem.Key, Elem.Value);
	}
	Event.Properties.Add("ue_ver_major", FString::Printf(TEXT("%d"), ENGINE_MAJOR_VERSION));
	Event.Properties.Add("ue_ver_minor", FString::Printf(TEXT("%d"), ENGINE_MINOR_VERSION));
	Event.Properties.Add("ue_version", FString::Printf(TEXT("%d.%d.%d"), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION));
	Event.Properties.Add("ue_language", FInternationalization::Get().GetCurrentCulture()->GetName());

	Request.OpenAPIAnalyticsEvents.Events.Add(Event);

	auto& Module = FDiversionModule::Get();
	if (&Module == nullptr) {
		return false;
	}

	FAnalyticsIngest& ApiCall = FDiversionModule::Get().GetApiCall<FAnalyticsIngest>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand.Worker, OutInfoMessages, OutErrorMessages);
}