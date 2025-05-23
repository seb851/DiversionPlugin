// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "ISourceControlModule.h"
#include "DiversionOperations.h"

#include "OpenAPIRepositoryMergeManipulationApi.h"
#include "OpenAPIRepositoryMergeManipulationApiOperations.h"


using namespace CoreAPI;

using Resolve = OpenAPIRepositoryMergeManipulationApi;

class FSyncResolve;
using FResolveAPI = TSyncApiCall<
	FSyncResolve,
	Resolve::SrcHandlersv2MergeSetResultRequest,
	Resolve::SrcHandlersv2MergeSetResultResponse,
	Resolve::FSrcHandlersv2MergeSetResultDelegate,
	Resolve,
	&Resolve::SrcHandlersv2MergeSetResult,
	AuthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncResolve final : public FResolveAPI {
	friend FResolveAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const Resolve::SrcHandlersv2MergeSetResultResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncResolve);


bool FSyncResolve::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const Resolve::SrcHandlersv2MergeSetResultResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	OutInfoMessages.Add("File resolved");
	return true;
}


bool DiversionUtils::RunResolvePath(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, 
	const FString& InMergeId, const FString& InConflictId, bool WaitForSync)
{
	if (WaitForSync && !WaitForAgentSync(InCommand, OutInfoMessages, OutErrorMessages))
	{
		return false;
	}

	FString FilePath;
	if (InCommand.Files.Num() <= 0) {
		FString ErrMsg = "RunResolvePath: No file path provided";
		UE_LOG(LogSourceControl, Error, TEXT("%s"), *ErrMsg);
		OutErrorMessages.Add(ErrMsg);
		return false;
	} 
	FilePath = InCommand.Files[0];

	// Get info of the specific blob 
	if(!GetWsBlobInfo(InCommand, OutInfoMessages, OutInfoMessages, FilePath))
	{
		UE_LOG(LogSourceControl, Error, TEXT("Diversion: RunResolvePath: Failed getting blob info"));
		return false;
	}

	FDiversionResolveFileWorker& Worker = static_cast<FDiversionResolveFileWorker&>(InCommand.Worker.Get());

	auto Request = Resolve::SrcHandlersv2MergeSetResultRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.MergeId = InMergeId;
	Request.ConflictId = InConflictId;
	Request.Path = DiversionUtils::ConvertFullPathToRelative(FilePath, InCommand.WsInfo.GetPath());
	Request.Mode = Worker.FileEntry.Mode;
	Request.Body = "";
	Request.Size = Worker.FileEntry.Blob->Size;
	Request.Sha1 = Worker.FileEntry.Blob->Sha;
	Request.StorageBackend = Worker.FileEntry.Blob->StorageBackend;
	Request.StorageUri = Worker.FileEntry.Blob->StorageUri;

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncResolve>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutInfoMessages);
}