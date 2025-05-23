// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"

#include "ISourceControlModule.h"
#include "Logging/StructuredLog.h"

#include "DiversionUtils.h"
#include "DiversionModule.h"
#include "DiversionCommand.h"

#include "OpenAPIRepositoryCommitManipulationApi.h"
#include "OpenAPIRepositoryCommitManipulationApiOperations.h"

using namespace CoreAPI;
using FCommit = OpenAPIRepositoryCommitManipulationApi;

class FSyncCommitWorkspace;
using FCommitWorkspaceSyncAPI = TSyncApiCall<
	FSyncCommitWorkspace,
	FCommit::SrcHandlersv2WorkspaceCommitWorkspaceRequest,
	FCommit::SrcHandlersv2WorkspaceCommitWorkspaceResponse,
	FCommit::FSrcHandlersv2WorkspaceCommitWorkspaceDelegate,
	FCommit,
	&FCommit::SrcHandlersv2WorkspaceCommitWorkspace,
	AuthorizedCall,
	// 	ResponseImplementation arguments types
	FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;
	//FDiversionCommand&,TArray<FString>& ,TArray<FDiversionState>&>;

class FSyncCommitWorkspace final : public FCommitWorkspaceSyncAPI {
	friend FCommitWorkspaceSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FCommit::SrcHandlersv2WorkspaceCommitWorkspaceResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncCommitWorkspace);


bool FSyncCommitWorkspace::ResponseImplementation(FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FCommit::SrcHandlersv2WorkspaceCommitWorkspaceResponse& Response)
{
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	InCommand.InfoMessages.Add(Response.GetResponseString());
	return true;
}

bool DiversionUtils::RunCommit(FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InDescription, bool WaitForSync)
{
	if (WaitForSync && !WaitForAgentSync(InCommand, OutInfoMessages, OutErrorMessages))
	{
		return false;
	}

	auto Request = FCommit::SrcHandlersv2WorkspaceCommitWorkspaceRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.WorkspaceId = InCommand.WsInfo.WorkspaceID;
	Request.OpenAPICommitRequest.CommitMessage = InDescription;

	TOptional<TArray<FString>> CommitPaths;
	if (InCommand.Files.Num() > 0) {
		// Note! zero files means commit all!
		// This is for internal use as using UE UI its not possible to send 0 files
		CommitPaths = TArray<FString>();
	}
	for (auto& FilePath : InCommand.Files) {
		// TODO: Convert all file paths in the command to relative paths upon command creation
		auto RelativeFilePath = FilePath.Replace(*InCommand.WsInfo.GetPath(), TEXT(""));
		CommitPaths->Add(RelativeFilePath);
	}

	Request.OpenAPICommitRequest.IncludePaths = CommitPaths;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncCommitWorkspace>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}