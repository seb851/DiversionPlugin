// Copyright 2024 Diversion Company, Inc. All Rights Reserved.


#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "IDiversionStatusWorker.h"

#include "DiversionState.h"
#include "DiversionModule.h"

#include "OpenAPIRepositoryWorkspaceManipulationApi.h"
#include "OpenAPIRepositoryWorkspaceManipulationApiOperations.h"


constexpr int StatusItemsLimit = 1500;

using namespace CoreAPI;

using FOutStates = TArray<FDiversionState>;
using FOutErrorMessages = TArray<FString>;

using FStatus = OpenAPIRepositoryWorkspaceManipulationApi;

class FSyncUpdateStatus;
using FStatusSyncAPI = TSyncApiCall<
	FSyncUpdateStatus,
	FStatus::SrcHandlersv2WorkspaceGetStatusRequest,
	FStatus::SrcHandlersv2WorkspaceGetStatusResponse,
	FStatus::FSrcHandlersv2WorkspaceGetStatusDelegate,
	FStatus,
	&FStatus::SrcHandlersv2WorkspaceGetStatus,
	AuthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncUpdateStatus final : public FStatusSyncAPI {
	friend FStatusSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FStatus::SrcHandlersv2WorkspaceGetStatusResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncUpdateStatus);


bool IsPathContained(const FString& InPath, const TArray<FString>& InCommandPaths) {
	for (auto& CommandPath : InCommandPaths)
	{
		if (FPaths::IsUnderDirectory(InPath, CommandPath))
		{
			return true;
		}
	}
	return false;
}

void ParseStateFromList(const TArray<CoreAPI::OpenAPIFileEntry>& InItems,
	const FString& InWsPath, const TArray<FString>& InCommandPaths,
	const EWorkingCopyState::Type& InState, const FDateTime& InDefaultMtime, 
	const int InLocalRevNumber, TMap<FString, FDiversionState>& OutStates, const TMap<FString, FDiversionResolveInfo>& ConflictedFiles) {
	for (const auto& File : InItems)
	{
		FString FullItemPath = DiversionUtils::ConvertRelativePathToDiversionFull(File.Path, InWsPath);
		// (Optimization): Only add state if FullItemPath is a subpath(or equal) of at least one of the InCommandPaths
		if (!IsPathContained(FullItemPath, InCommandPaths)) continue;

		// Don't override conflicted files states if we have ongoing merge
		if (ConflictedFiles.Contains(FullItemPath)) continue;

		FDiversionState FileState(FullItemPath);
		FileState.WorkingCopyState = InState;
		FileState.TimeStamp = File.Mtime.IsSet() ? File.Mtime.GetValue() : InDefaultMtime;
		FileState.LocalRevNumber = InLocalRevNumber;
		FileState.Hash = File.Hash.IsSet() ? File.Hash.GetValue() : TEXT("");
		OutStates.Add(FullItemPath, FileState);
	}
}

bool FSyncUpdateStatus::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FStatus::SrcHandlersv2WorkspaceGetStatusResponse& Response) {
	
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}
	
	if (!Response.Content.Items.IsSet()) {
		FString BaseErr = FString::Printf(TEXT("Failed parsing status response"));
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	// Since the command paths might be a superset of the paths in the response, we need to 
	// traverse the response and update the states accordingly
	auto& Items = Response.Content.Items.GetValue();

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InCommand.Worker.Get());

	const FDateTime Now = FDateTime::Now();
	const int LocalRevNumber = DiversionUtils::GetWorkspaceRevisionByCommit(InCommand.WsInfo.CommitID);
	const FString WsPath = InCommand.WsInfo.GetPath();

	// Handle controlled files
	ParseStateFromList(Items._New, WsPath, InCommand.Files, EWorkingCopyState::Added, Now, LocalRevNumber, Worker.States, InCommand.ConflictedFiles);
	ParseStateFromList(Items.Modified, WsPath, InCommand.Files, EWorkingCopyState::Modified, Now, LocalRevNumber, Worker.States, InCommand.ConflictedFiles);
	ParseStateFromList(Items.Deleted, WsPath, InCommand.Files, EWorkingCopyState::Deleted, Now, LocalRevNumber, Worker.States, InCommand.ConflictedFiles);

	// Handle non-controlled files
	for (auto& Path : InCommand.Files) {
		// The assumption is UE provides us absolute path
		if(!FPaths::IsUnderDirectory(Path, WsPath))
		{
			UE_LOG(LogSourceControl, Log, TEXT("Path: %s is not contained in the repo, skipping."), *Path);
			continue;
		}

		// Skip if the path was already handled
		// TODO: handle readding a deleted file before saving it
		if(Worker.States.Contains(Path)) continue;
		
		FDiversionState FileState(Path);
		if (InCommand.ConflictedFiles.Contains(Path)) {
			// Don't override conflicted files states if we have ongoing merge
			FileState.WorkingCopyState = EWorkingCopyState::Conflicted;
			FileState.PendingResolveInfo = InCommand.ConflictedFiles[Path];
		}
		else if (FPaths::FileExists(Path) || FPaths::DirectoryExists(Path)) {
			FileState.WorkingCopyState = EWorkingCopyState::Unchanged;
			// Set the timestamp to the last write time
			FileState.TimeStamp = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*Path);
		}
		else {
			FileState.WorkingCopyState = EWorkingCopyState::NotControlled;
		}
		Worker.States.Add(Path, FileState);
	}

	// Handle pagination
	Worker.ItemsFetchedNum = Items._New.Num() + Items.Modified.Num() + Items.Deleted.Num();
	Worker.bIncompleteResult = Response.Content.IncompleteResult.IsSet() && Response.Content.IncompleteResult.GetValue();

	return true;
}

bool AllPathsAreFiles(const TArray<FString>& InPaths) {
	for (auto& Path : InPaths) {
		if (FPaths::DirectoryExists(Path)) {
			return false;
		}
	}
	return true;
}

bool DiversionUtils::RunUpdateStatus(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, 
	TArray<FString>& OutErrorMessages, bool WaitForSync){

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InCommand.Worker.Get());

	auto Request = FStatus::SrcHandlersv2WorkspaceGetStatusRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.WorkspaceId = InCommand.WsInfo.WorkspaceID;
	Request.Limit = StatusItemsLimit;
	Request.AllowTrim = false;

	TArray<FString> CommonPrefixes = GetPathsCommonPrefixes(InCommand.Files, InCommand.WsInfo.GetPath()).Array();

	// Avoid recursive status requests if all paths are files and on the same directory
	bool bRequestStatusOfOnlyOneDirectory = AllPathsAreFiles(InCommand.Files) && CommonPrefixes.Num() == 1;
	Worker.bRecursiveRequest = !bRequestStatusOfOnlyOneDirectory;
	
	FString AncestorPrefix = DiversionUtils::ConvertFullPathToRelative(FindCommonAncestorDirectory(InCommand.Files),
		InCommand.WsInfo.GetPath());
	Request.PathPrefix = AncestorPrefix;
	Request.Recurse = !bRequestStatusOfOnlyOneDirectory;

	int RequestOffset = 0;
	bool Success = true;
	while (true) {
		Request.Skip = RequestOffset;
		auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncUpdateStatus>();
		Success &= ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
		
		if (!Worker.bIncompleteResult) {
			break;
		}
		RequestOffset += Worker.ItemsFetchedNum;
	}

	// Show Conflicting files
	if (Success)
	{
		// Get actual conflicted files (Open active merges)
		GetConflictedFiles(InCommand, OutInfoMessages, OutErrorMessages);
	}

	return Success;
}