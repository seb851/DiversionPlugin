// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "IPAddressAsyncResolve.h"
#include "ISourceControlModule.h"
#include "DiversionCommand.h"

#include "OpenAPIRepositoryMergeManipulationApi.h"
#include "OpenAPIRepositoryMergeManipulationApiOperations.h"

#include "SyncApiCall.h"

using namespace CoreAPI;

using FMerges = OpenAPIRepositoryMergeManipulationApi;

class FSyncFinalizeMerge;
using FMergeFinalizeSyncAPI = TSyncApiCall<
	FSyncFinalizeMerge,
	FMerges::SrcHandlersv2MergeFinalizeRequest,
	FMerges::SrcHandlersv2MergeFinalizeResponse,
	FMerges::FSrcHandlersv2MergeFinalizeDelegate,
	FMerges,
	&FMerges::SrcHandlersv2MergeFinalize,
	AuthorizedCall,
	// Output parameters
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncFinalizeMerge final : public FMergeFinalizeSyncAPI {
	friend FMergeFinalizeSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeFinalizeResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncFinalizeMerge);

bool FSyncFinalizeMerge::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FMerges::SrcHandlersv2MergeFinalizeResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	return true;
}


bool DiversionUtils::RunFinalizeMerge(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InMergeId) {
	auto Request = FMerges::SrcHandlersv2MergeFinalizeRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.MergeId = InMergeId;
	Request.OpenAPICommitMessage = OpenAPICommitMessage();
	Request.OpenAPICommitMessage->CommitMessage = FString("Merged " + InMergeId);

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncFinalizeMerge>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}

