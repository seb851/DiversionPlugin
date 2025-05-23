// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIRepositoryWorkspaceManipulationApi.h"
#include "OpenAPIRepositoryWorkspaceManipulationApiOperations.h"


using namespace CoreAPI;

using FReset = OpenAPIRepositoryWorkspaceManipulationApi;

class FSyncReset;
using FResetAPI = TSyncApiCall<
	FSyncReset,
	FReset::SrcHandlersv2WorkspaceResetRequest,
	FReset::SrcHandlersv2WorkspaceResetResponse,
	FReset::FSrcHandlersv2WorkspaceResetDelegate,
	FReset,
	&FReset::SrcHandlersv2WorkspaceReset,
	AuthorizedCall,
	// 	ResponseImplementation arguments types
	FDiversionWorkerRef, /*Worker*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncReset final : public FResetAPI {
	friend FResetAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages,
		TArray<FString>& OutErrorMessages, const FReset::SrcHandlersv2WorkspaceResetResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncReset);

bool FSyncReset::ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, const FReset::SrcHandlersv2WorkspaceResetResponse& Response) {
	if (!Response.IsSuccessful() || (Response.GetHttpResponseCode() != EHttpResponseCodes::Ok)) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}
	// TODO: compare response to the expected response?
	// TODO: print the paths that were reset
	OutInfoMessages.Add("Successfully reset path(s)");
	return true;
}


bool DiversionUtils::RunReset(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	auto Request = FReset::SrcHandlersv2WorkspaceResetRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.WorkspaceId = InCommand.WsInfo.WorkspaceID;
	Request.OpenAPISrcHandlersv2WorkspaceResetRequest.DeleteAdded = true;

	TArray<FString> RelativeFilesPaths;
	Algo::Transform(InCommand.Files, RelativeFilesPaths, [&InCommand](const FString& Path) {
		return ConvertFullPathToRelative(Path, InCommand.WsInfo.GetPath());
	});

	Request.OpenAPISrcHandlersv2WorkspaceResetRequest.Paths = RelativeFilesPaths;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncReset>();
	bool Success = ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand.Worker, OutInfoMessages, OutErrorMessages);

	return Success;
}