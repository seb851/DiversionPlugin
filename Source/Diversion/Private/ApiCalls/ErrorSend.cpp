// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "IDiversionStatusWorker.h"

#include "DiversionState.h"
#include "DiversionModule.h"

#include "OpenAPISupportApi.h"
#include "OpenAPISupportApiOperations.h"
#include <DiversionOperations.h>

using namespace CoreAPI;

using FSupport = OpenAPISupportApi;

class FSupportSendError;
using FSupportAPI = TSyncApiCall<
	FSupportSendError,
	FSupport::SrcHandlersSupportErrorReportRequest,
	FSupport::SrcHandlersSupportErrorReportResponse,
	FSupport::FSrcHandlersSupportErrorReportDelegate,
	FSupport,
	&FSupport::SrcHandlersSupportErrorReport,
	AuthorizedCall>;


class FSupportSendError final : public FSupportAPI {
public:
	FSupportSendError() {}
	static bool ResponseImplementation(const FSupport::SrcHandlersSupportErrorReportResponse& Response);
};

REGISTER_PARSE_TYPE(FSupportSendError);


bool FSupportSendError::ResponseImplementation(const FSupport::SrcHandlersSupportErrorReportResponse& Response) {
	if (!Response.IsSuccessful() || (Response.GetHttpResponseCode() != EHttpResponseCodes::NoContent)) {
		UE_LOG(LogSourceControl, Error, TEXT("Diversion: Failed sending error message to the backend"))
		return false;
	}

	return true;
}


bool DiversionUtils::SendErrorToBE(const FString& AccountID, const FString& InErrorMessageToReport, const FString& InStackTrace)
{
	auto Request = FSupport::SrcHandlersSupportErrorReportRequest();

	Request.OpenAPIErrorReport.Source = "Unreal-Diversion-Plugin";
	Request.OpenAPIErrorReport.Error = InErrorMessageToReport;
	Request.OpenAPIErrorReport.Stack = InStackTrace;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSupportSendError>();
	return ApiCall.CallAPI(Request, AccountID);
}