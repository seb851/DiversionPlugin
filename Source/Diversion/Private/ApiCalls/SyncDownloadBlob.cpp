// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

#include "ISourceControlModule.h"

#include "SyncApiCall.h"
#include "DiversionUtils.h"
#include "DiversionCommand.h"
#include "DiversionOperations.h"

#include "OpenAPIRepositoryManipulationApi.h"
#include "OpenAPIRepositoryManipulationApiOperations.h"


using namespace CoreAPI;

using Download = OpenAPIRepositoryManipulationApi;

class FSyncDownloadBlob;
using FDownloadBlobSyncAPI = TSyncApiCall<
	FSyncDownloadBlob,
	Download::SrcHandlersv2FilesGetBlobRequest,
	Download::SrcHandlersv2FilesGetBlobResponse,
	Download::FSrcHandlersv2FilesGetBlobDelegate,
	Download,
	&Download::SrcHandlersv2FilesGetBlob,
	AuthorizedCall,
	// Output parameters
	TArray<FString>&, /*ErrorMessages*/
	TArray<FString>&, /*InfoMessages*/
	FString& /*OutRedirectUrl*/>;


class FSyncDownloadBlob final : public FDownloadBlobSyncAPI {
	friend FDownloadBlobSyncAPI; 	// To support Static Polymorphism and keep encapsulation

	static bool ResponseImplementation(TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages, 
		FString& OutRedirectUrl, const Download::SrcHandlersv2FilesGetBlobResponse& Response);
};
REGISTER_PARSE_TYPE(FSyncDownloadBlob);

bool FSyncDownloadBlob::ResponseImplementation(TArray<FString>& OutInfoMessages, TArray<FString>& OutErrorMessages,
	FString& OutRedirectUrl, const Download::SrcHandlersv2FilesGetBlobResponse& Response) {
	if (!Response.IsSuccessful()) {
		FString BaseErr = FString::Printf(TEXT("%d:%s"), Response.GetHttpResponseCode(), *Response.GetResponseString());
		AddErrorMessage(BaseErr, OutErrorMessages);
		return false;
	}
if (Response.GetHttpResponseCode() == EHttpResponseCodes::NoContent)
	{
		OutRedirectUrl = Response.GetHttpResponse()->GetHeader("location");
		OutInfoMessages.Add("Received redirection URL for file");
		return false;
	}
	
	OutInfoMessages.Add("File was succesfully downloaded");
	return true;
}


bool DiversionUtils::DownloadBlob(TArray<FString>& OutInfoMessages,
	TArray<FString>& OutErrorMessages, const FString& InRefId, const FString& InOutputFilePath, 
	const FString& InFilePath, WorkspaceInfo InWsInfo)
{
	auto Request = Download::SrcHandlersv2FilesGetBlobRequest();
	Request.RepoId = InWsInfo.RepoID;
	Request.RefId = InRefId;
	Request.Path = ConvertFullPathToRelative(InFilePath, InWsInfo.GetPath());
	Request.ResponseFilePath = InOutputFilePath;

	FString RedirectUrl;
	auto& DownloadApiCall = FDiversionModule::Get().GetApiCall<FSyncDownloadBlob>();
	auto Success = DownloadApiCall.CallAPI(Request, InWsInfo.AccountID, OutInfoMessages, OutErrorMessages, RedirectUrl);
	
	if (!Success && !RedirectUrl.IsEmpty())
	{
		Success = DownloadFileFromURL(RedirectUrl, InOutputFilePath);
	}
	return Success;
}

