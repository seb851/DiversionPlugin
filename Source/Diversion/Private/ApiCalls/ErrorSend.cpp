// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionModule.h"

using namespace Diversion::CoreAPI;

bool DiversionUtils::SendErrorToBE(const FString& AccountID, const FString& InErrorMessageToReport, const FString& InStackTrace)
{
	auto ErrorResponse = SupportApi::Fsrc_handlers_support_errorReportDelegate::Bind([&]() {
		return false;
	});

	auto VariantResponse = SupportApi::Fsrc_handlers_support_errorReportDelegate::Bind([&]() {
		return true;
	});
	
	TSharedPtr<ErrorReport> Request = MakeShared<ErrorReport>();
	Request->mSource = "Unreal-Diversion-Plugin";
	Request->mError = InErrorMessageToReport;
	Request->mStack = InStackTrace;

	TArray<FString> ErrorMessages;
	return FDiversionModule::Get().SupportAPIRequestManager->SrcHandlersSupportErrorReport(Request, 
		FDiversionModule::Get().GetAccessToken(AccountID), {}, 5, 120).HandleApiResponse(ErrorResponse, VariantResponse, ErrorMessages);
}