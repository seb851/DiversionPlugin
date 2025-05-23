// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "ISourceControlModule.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"
#include "Logging/StructuredLog.h"

using namespace AgentAPI;

using InitRepoSync = OpenAPIDefaultApi;

class FSyncFRepoInit;
using FRepoInitAPI = TSyncApiCall<
	FSyncFRepoInit,
	InitRepoSync::RepoInitRequest,
	InitRepoSync::RepoInitResponse,
	InitRepoSync::FRepoInitDelegate,
	InitRepoSync,
	&InitRepoSync::RepoInit,
	UnauthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncFRepoInit final : public FRepoInitAPI {
	friend FRepoInitAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const InitRepoSync::RepoInitResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncFRepoInit);

bool FSyncFRepoInit::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const InitRepoSync::RepoInitResponse& Response) {
	if (!Response.IsSuccessful() || Response.GetHttpResponseCode() != EHttpResponseCodes::Ok) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	OutInfoMessages.Add("Repo initialized successfully");
	return true;
}

bool DiversionUtils::RunRepoInit(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InRepoRootPath, const FString& InRepoName)
{
	auto Request = InitRepoSync::RepoInitRequest();
	Request.OpenAPIInitRepo.Path = InRepoRootPath;
	Request.OpenAPIInitRepo.Name = InRepoName;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncFRepoInit>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}