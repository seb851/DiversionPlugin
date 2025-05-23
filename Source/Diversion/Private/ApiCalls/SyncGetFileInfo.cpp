// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionOperations.h"

#include "OpenAPIRepositoryManipulationApi.h"
#include "OpenAPIRepositoryManipulationApiOperations.h"

using namespace CoreAPI;

using FRepoFileInfo = OpenAPIRepositoryManipulationApi;

class FSyncGetFileInfo;
using FFileInfoAPI = TSyncApiCall<
	FSyncGetFileInfo,
	FRepoFileInfo::SrcHandlersv2FilesGetFileEntryRequest,
	FRepoFileInfo::SrcHandlersv2FilesGetFileEntryResponse,
	FRepoFileInfo::FSrcHandlersv2FilesGetFileEntryDelegate,
	FRepoFileInfo,
	&FRepoFileInfo::SrcHandlersv2FilesGetFileEntry,
	AuthorizedCall,
	const FDiversionCommand&, /*InCommand*/
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>& /*InfoMessages*/>;


class FSyncGetFileInfo final : public FFileInfoAPI {
	friend FFileInfoAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(const FDiversionCommand& InCommand,
		TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FRepoFileInfo::SrcHandlersv2FilesGetFileEntryResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncGetFileInfo);

bool FSyncGetFileInfo::ResponseImplementation(const FDiversionCommand& InCommand,
	TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FRepoFileInfo::SrcHandlersv2FilesGetFileEntryResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}

	FDiversionResolveFileWorker& Worker = static_cast<FDiversionResolveFileWorker&>(InCommand.Worker.Get());
	Worker.FileEntry = Response.Content;

	OutInfoMessages.Add("Successfuly retrieved file entry");
	return true;
}


bool DiversionUtils::GetWsBlobInfo(const FDiversionCommand& InCommand, TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, const FString& InFile)
{
	auto Request = FRepoFileInfo::SrcHandlersv2FilesGetFileEntryRequest();
	Request.RepoId = InCommand.WsInfo.RepoID;
	Request.RefId = InCommand.WsInfo.WorkspaceID;
	Request.Path = ConvertFullPathToRelative(InFile, InCommand.WsInfo.GetPath());

	auto& ApiCall = FDiversionModule::Get().GetApiCall<FSyncGetFileInfo>();
	return ApiCall.CallAPI(Request, InCommand.WsInfo.AccountID, InCommand, OutInfoMessages, OutErrorMessages);
}