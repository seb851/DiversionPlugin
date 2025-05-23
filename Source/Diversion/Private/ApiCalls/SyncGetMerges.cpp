// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "IPAddressAsyncResolve.h"
#include "ISourceControlModule.h"
#include "DiversionCommand.h"
#include "IDiversionStatusWorker.h"

#include "OpenAPIRepositoryMergeManipulationApi.h"
#include "OpenAPIRepositoryMergeManipulationApiOperations.h"

#include "SyncApiCall.h"

using namespace CoreAPI;

using FConflictData = FDiversionResolveInfo;

#pragma region GetMerges

using FMerges = OpenAPIRepositoryMergeManipulationApi;

class FSyncGetMerges;
using FMergeListSyncAPI = TSyncApiCall<
	FSyncGetMerges,
	FMerges::SrcHandlersv2MergeListOpenMergesRequest,
	FMerges::SrcHandlersv2MergeListOpenMergesResponse,
	FMerges::FSrcHandlersv2MergeListOpenMergesDelegate,
	FMerges,
	&FMerges::SrcHandlersv2MergeListOpenMerges,
	AuthorizedCall,
	// Output parameters
	FDiversionWorkerRef, /*Worker*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/
	>;

class FSyncGetMerges final : public FMergeListSyncAPI {
	friend FMergeListSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(FDiversionWorkerRef InOutWorker,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeListOpenMergesResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncGetMerges);

bool FSyncGetMerges::ResponseImplementation(FDiversionWorkerRef InOutWorker,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeListOpenMergesResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InOutWorker.Get());
	Worker.Merges = Response.Content.Items;
	
	return true;
}

#pragma endregion

#pragma region GetSpecificMerge
class FSyncGetConflictedFiles;
using FGetSpecificMergeSyncAPI = TSyncApiCall<
	FSyncGetConflictedFiles,
	FMerges::SrcHandlersv2MergeGetOpenMergeRequest,
	FMerges::SrcHandlersv2MergeGetOpenMergeResponse,
	FMerges::FSrcHandlersv2MergeGetOpenMergeDelegate,
	FMerges,
	&FMerges::SrcHandlersv2MergeGetOpenMerge,
	AuthorizedCall,
	// 	ResponseImplementation arguments types
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/
>;


class FSyncGetConflictedFiles final : public FGetSpecificMergeSyncAPI {
	friend FGetSpecificMergeSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeGetOpenMergeResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncGetConflictedFiles);

bool FSyncGetConflictedFiles::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeGetOpenMergeResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InCommand.Worker.Get());

	FConflictData ConflictInfo;
	for (const auto& Conflict: Response.Content.Conflicts) {
		ConflictInfo.BaseFile = DiversionUtils::ConvertRelativePathToDiversionFull(Conflict.Base.Path, InCommand.WsInfo.GetPath());
		ConflictInfo.BaseRevision = Worker.Merges[0].AncestorCommit;
		ConflictInfo.RemoteRevision = Worker.Merges[0].OtherRef;
		ConflictInfo.RemoteFile = DiversionUtils::ConvertRelativePathToDiversionFull(Conflict.Other.Path, InCommand.WsInfo.GetPath());
		ConflictInfo.MergeId = Worker.Merges[0].Id;
		ConflictInfo.ConflictId = Conflict.ConflictId;
		Worker.Conflicts.Add(ConflictInfo.BaseFile, ConflictInfo);
		
	}

	OutInfoMessages.Add(FString::Printf(TEXT("Found %d conflicted files"), Worker.Conflicts.Num()));
	return true;
}

#pragma endregion 

bool DiversionUtils::RunGetMerges(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages) {
	auto Request = FMerges::SrcHandlersv2MergeListOpenMergesRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncGetMerges>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand.Worker, OutInfoMessages, OutErrorMessages);
}


bool DiversionUtils::GetConflictedFiles(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages) {
	
	// Get the list of open merges first
	if (!RunGetMerges(InCommand, OutInfoMessages, OutErrorMessages)) {
		return false;
	}

	IDiversionStatusWorker& Worker = static_cast<IDiversionStatusWorker&>(InCommand.Worker.Get());

	// We should take only the first returned merge.
	// It doesn't make sense to handle more than one merge at the same time
	if (Worker.Merges.Num() > 0)
	{
		const auto& Merge = Worker.Merges[0];
		auto Request = FMerges::SrcHandlersv2MergeGetOpenMergeRequest();
		Request.MergeId = Merge.Id;
		Request.RepoId = InCommand.WsInfo.RepoID;

		auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncGetConflictedFiles>();
		if (!ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages)) {
			OutErrorMessages.Add(FString::Printf(TEXT("Failed to get conflicted files for merge %s"), *Merge.Id));
			return false;
		}

		// Update the state of the ongoing merge
		Worker.OutOnGoingMerge = Worker.Conflicts.Num() > 0;
	}
	return true;
}