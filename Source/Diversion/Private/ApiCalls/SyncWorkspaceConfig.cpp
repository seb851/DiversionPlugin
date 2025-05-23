// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"

#include "OpenAPIDefaultApi.h"
#include "OpenAPIDefaultApiOperations.h"
#include "DiversionOperations.h"

using namespace AgentAPI;

using WsConfSync = OpenAPIDefaultApi;

class FSyncFGetWorkspaceByPath;
using FGetWorkspaceByPathAPI = TSyncApiCall<
	FSyncFGetWorkspaceByPath,
	WsConfSync::GetWorkspaceByPathRequest,
	WsConfSync::GetWorkspaceByPathResponse,
	WsConfSync::FGetWorkspaceByPathDelegate,
	WsConfSync,
	&WsConfSync::GetWorkspaceByPath,
	UnauthorizedCall,
	FDiversionWorkerRef,
	TArray<FString>&, 
	TArray<FString>&>;


class FSyncFGetWorkspaceByPath: public FGetWorkspaceByPathAPI {
	friend FGetWorkspaceByPathAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages,
		TArray<FString>& OutErrorMessages, const WsConfSync::GetWorkspaceByPathResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncFGetWorkspaceByPath);


bool FSyncFGetWorkspaceByPath::ResponseImplementation(FDiversionWorkerRef InOutWorker, TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, const WsConfSync::GetWorkspaceByPathResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	if (Response.Content.Num() <= 0) {
		// TODO: pass request parameters for better error messages
		AddErrorMessage("No workspaces found in the given path", OutErrorMessages);
		return false;
	}

	// We care only about the first workspace (The return shouldn't have more than one)
	auto& ResultWsConfig = Response.Content.begin()->Value;
	auto& ResultWsId = Response.Content.begin()->Key;

	auto& Worker = static_cast<FDiversionWsInfoWorker&>(InOutWorker.Get());

	Worker.WsInfo.WorkspaceID = ResultWsId;
	// TODO: Add Ws Name in Endpoint
	Worker.WsInfo.WorkspaceName = "";
	Worker.WsInfo.RepoID = ResultWsConfig.RepoID;
	Worker.WsInfo.RepoName = ResultWsConfig.RepoName.IsSet() ? ResultWsConfig.RepoName.GetValue() : "";
	Worker.WsInfo.SetPath(ResultWsConfig.Path);
	Worker.WsInfo.AccountID = ResultWsConfig.AccountID;
	Worker.WsInfo.BranchID = ResultWsConfig.BranchID.IsSet() ? ResultWsConfig.BranchID.GetValue(): "";
	Worker.WsInfo.BranchName = ResultWsConfig.BranchName.IsSet() ? ResultWsConfig.BranchName.GetValue() : "";
	Worker.WsInfo.CommitID = ResultWsConfig.CommitID;
	
	return true;
}


bool DiversionUtils::GetWorkspaceConfigByPath(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InPath) {
	auto Request = WsConfSync::GetWorkspaceByPathRequest();
	Request.AbsPath = InPath;

	auto& Module = FDiversionModule::Get();
	if (&Module == nullptr) {
		return false;
	}

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncFGetWorkspaceByPath>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand.Worker, OutInfoMessages, OutErrorMessages);
}