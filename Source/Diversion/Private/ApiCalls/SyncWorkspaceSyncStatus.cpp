// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"

using namespace AgentAPI;

using FWsSync = OpenAPIDefaultApi;

class FSyncGetWorkspaceSyncStatus;
using FGetWorkspaceSyncStatusAPI = TSyncApiCall<
	FSyncGetWorkspaceSyncStatus,
	FWsSync::GetWorkspaceSyncStatusRequest,
	FWsSync::GetWorkspaceSyncStatusResponse,
	FWsSync::FGetWorkspaceSyncStatusDelegate,
	FWsSync,
	&FWsSync::GetWorkspaceSyncStatus,
	UnauthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/
	>;


class FSyncGetWorkspaceSyncStatus final : public FGetWorkspaceSyncStatusAPI {
	friend FGetWorkspaceSyncStatusAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FWsSync::GetWorkspaceSyncStatusResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncGetWorkspaceSyncStatus);


bool FSyncGetWorkspaceSyncStatus::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FWsSync::GetWorkspaceSyncStatusResponse& Response) {
	IDiversionWorker& Worker = static_cast<IDiversionWorker&>(InCommand.Worker.Get());

	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::Unknown;
		return false;
	}

	// Waiting for implementation
	if (Response.Content.IsPaused) {
		Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::Paused;
	}
	else if (Response.Content.IsSyncComplete) {
		Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::Completed;
	} else{
		Worker.SyncStatus = DiversionUtils::EDiversionWsSyncStatus::InProgress;
	}

	return true;
}


bool DiversionUtils::AgentInSync(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages)
{
	auto Request = FWsSync::GetWorkspaceSyncStatusRequest();
	Request.RepoID = InCommand.WsInfo.RepoID;
	Request.WorkspaceID = InCommand.WsInfo.WorkspaceID;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncGetWorkspaceSyncStatus>();
	bool Success = ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
	if (Success) {
		Success &= WorkspaceSyncProgress(InCommand, OutInfoMessages, OutErrorMessages);
	}
	return Success;
}