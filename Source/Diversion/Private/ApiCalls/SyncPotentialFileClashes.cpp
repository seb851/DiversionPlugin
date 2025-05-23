// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "IDiversionStatusWorker.h"

#include "OpenAPIRepositoryWorkspaceManipulationApi.h"
#include "OpenAPIRepositoryWorkspaceManipulationApiOperations.h"


using namespace CoreAPI;

using FPotentialFileClashes = OpenAPIRepositoryWorkspaceManipulationApi;

class FSyncPotentialFileClashes;
using FPotentialFileClashesAPI = TSyncApiCall<
	FSyncPotentialFileClashes,
	FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatusesRequest,
	FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatusesResponse,
	FPotentialFileClashes::FSrcHandlersv2WorkspaceGetOtherStatusesDelegate,
	FPotentialFileClashes,
	&FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatuses,
	AuthorizedCall,
	// 	ResponseImplementation arguments types
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>& /*InfoMessages*/,
	TArray<FString>&, /*ErrorMessages*/
	TMap<FString, TArray<EDiversionPotentialClashInfo>>& /*OutPotentialClashes*/>;


class FSyncPotentialFileClashes final : public FPotentialFileClashesAPI {
	friend FPotentialFileClashesAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages,
		TMap<FString, TArray<EDiversionPotentialClashInfo>>& OutPotentialClashes,
		const FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatusesResponse& Response);
};

bool FSyncPotentialFileClashes::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages,
	TMap<FString, TArray<EDiversionPotentialClashInfo>>& OutPotentialClashes,
	const FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatusesResponse& Response) {
	if (!Response.IsSuccessful() || (Response.GetHttpResponseCode() != EHttpResponseCodes::Ok)) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	const FString WsPath = InCommand.WsInfo.GetPath();

	for (auto& status : Response.Content.Statuses) {
		auto FullStatusFilePath = DiversionUtils::ConvertRelativePathToDiversionFull(status.Path, WsPath);
		TArray<EDiversionPotentialClashInfo> PotentialClashes;
		for(auto& FileStatus : status.FileStatuses){
			PotentialClashes.Add(EDiversionPotentialClashInfo(
				FileStatus.CommitId,
				FileStatus.WorkspaceId.GetPtrOrNull() ? *FileStatus.WorkspaceId : "",
				FileStatus.BranchName.GetPtrOrNull() ? *FileStatus.BranchName : "N/a",
				FileStatus.Author.Email.GetPtrOrNull() ? *FileStatus.Author.Email : "",
				FileStatus.Author.FullName.GetPtrOrNull() ? *FileStatus.Author.FullName : "",
				FileStatus.Mtime.GetPtrOrNull() ? *FileStatus.Mtime : -1
			));
		}
		OutPotentialClashes.Add(FullStatusFilePath, PotentialClashes);
	}
	
	// Remove outdated potential clash data
	for (auto& Path : InCommand.Files)
	{
		if(!FPaths::IsUnderDirectory(Path, WsPath))
		{
			UE_LOG(LogSourceControl, Log, TEXT("Path: %s is not contained in the repo, skipping."), *Path);
			continue;
		}
		auto FullStatusFilePath = DiversionUtils::ConvertRelativePathToDiversionFull(Path, WsPath);
		if(OutPotentialClashes.Contains(FullStatusFilePath)) continue;
		// Indicate that there are no potential clashes for the file we queried
		OutPotentialClashes.Add(FullStatusFilePath, TArray<EDiversionPotentialClashInfo>());
	}
	
	return true;
}

REGISTER_PARSE_TYPE(FSyncPotentialFileClashes);

bool DiversionUtils::GetPotentialFileClashes(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, 
TArray<FString>& OutErrorMessages, TMap<FString, TArray<EDiversionPotentialClashInfo>>& OutPotentialClashes, bool& OutRecurseCall)
{
	const int PrefixesLimit = 20;
	bool Success = true;
	auto Request = FPotentialFileClashes::SrcHandlersv2WorkspaceGetOtherStatusesRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.WorkspaceId = InCommand.WsInfo.WorkspaceID;

	// Recurse request if the full repo is being updated
	bool FullRepoUpdateRequested = (InCommand.Files.Num() == 1) && (InCommand.Files[0] == InCommand.WsInfo.GetPath());
	Request.Recurse = FullRepoUpdateRequested;
	OutRecurseCall = FullRepoUpdateRequested;
	
	TArray<FString> FullPrefixesArray = GetPathsCommonPrefixes(InCommand.Files, InCommand.WsInfo.GetPath()).Array();
	int PrefixesRequestOffset = 0;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncPotentialFileClashes>();

	while (PrefixesRequestOffset < FullPrefixesArray.Num()) {
		// Get the relevant prefixes to sent to the BE in the current request
		
		if(!DiversionUtils::DiversionValidityCheck(PrefixesRequestOffset < FullPrefixesArray.Num(), 
			"PrefixesRequestOffset is greater than FullPrefixesArray.Num()!", InCommand.WsInfo.AccountID)) 
		{
			// Should never happen!
			break;
		}

		// Try taking as much elements as possible left in the FullPrefixesArray, but not passing the limit
		int ElemensToTake = FullPrefixesArray.Num() - PrefixesRequestOffset < PrefixesLimit ? (FullPrefixesArray.Num() - PrefixesRequestOffset) : PrefixesLimit;
		if (!DiversionUtils::DiversionValidityCheck(PrefixesRequestOffset + ElemensToTake <= FullPrefixesArray.Num(),
			"Tried slicing the FullPrefixesArray for more than its contents...", InCommand.WsInfo.AccountID))
		{
			// Should never happen!
			break;
		}

		auto PartialPrefixesView = TArrayView<FString>(&FullPrefixesArray[PrefixesRequestOffset], ElemensToTake);

		// Convert view to array with relative paths
		TArray<FString> RelativePartialPrefixesArray;
		for (auto& Prefix : PartialPrefixesView) {
			RelativePartialPrefixesArray.Add(DiversionUtils::ConvertFullPathToRelative(Prefix, InCommand.WsInfo.GetPath()));
		}

		// Send request of the current offset
		Request.PathPrefixes = RelativePartialPrefixesArray;
		Success &= ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, 
									OutErrorMessages, OutPotentialClashes);
		if (!Success) {
			break;
		}

		PrefixesRequestOffset += ElemensToTake;
	}

	return Success;
}