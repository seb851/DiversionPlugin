// Copyright 2024 Diversion Company, Inc. All Rights Reserved.
/**
 * Diversion Core API
 * Definition of the Core API used to access low-level functionality of Diversion
 *
 * The version of the OpenAPI document: 0.2.0
 *
 * NOTE: This class is auto generated by OpenAPI-Generator 7.10.0.
 * https://openapi-generator.tech
 * Do not edit the class manually.
 */

#pragma once

#include "CoreMinimal.h"
#include "HTTPResult.h"
#include "DiversionHttpManager.h"
#include "Misc/TVariant.h"


#include "Error.h"

#include "FileEntry.h"
#include "HttpContent.h"
#include "Src_handlersv2_commit_get_object_history_200_response.h"

namespace Diversion {
namespace CoreAPI {

using namespace Diversion::CoreAPI::Model;

class COREAPI_API RepositoryManipulationApi
{
public:
    explicit RepositoryManipulationApi(TSharedPtr<DiversionHttp::FHttpRequestManager> ApiClient);
    virtual ~RepositoryManipulationApi();


    /**
    * Get object history in a ref by its latest path
    * @param repoId The repo ID of the repository. Repo _name_ can be used instead of the ID, but usage of ID for permanent linking and API requests is preferred.@param refId An ID of a workspace, branch or commit.@param path A path to an item inside the repository.@param limit Limit the number or items returned from a listing api@param skip Skip a number of items returned from a listing api
    * @return 
    */
    THTTPResult<TVariant<TSharedPtr<Src_handlersv2_commit_get_object_history_200_response>, TSharedPtr<Error>>> SrcHandlersv2CommitGetObjectHistory(
        FString repoId,
        FString refId,
        FString path,
        TOptional<int32_t> limit,
        TOptional<int32_t> skip,
        const FString& Token,
        const TMap<FString, FString>& Headers,
		int ConnectionTimeoutSeconds, int RequestTimeoutSeconds) const;

    /**
    * Get blob contents snapshot. Either one of workspace, branch or commit ID needs to be specified.
    * @param repoId The repo ID of the repository. Repo _name_ can be used instead of the ID, but usage of ID for permanent linking and API requests is preferred.@param refId An ID of a workspace, branch or commit.@param path A path to an item inside the repository.
    * @return 
    */
    THTTPResult<TVariant<TSharedPtr<HttpContent>, void*>> SrcHandlersv2FilesGetBlob(
        FString repoId,
        FString refId,
        FString path,
        const FString& Token,
        const TMap<FString, FString>& Headers,
		int ConnectionTimeoutSeconds, int RequestTimeoutSeconds) const;

    /**
    * Get file entry (either tree or blob). Either one of workspace, branch or commit ID needs to be specified.
    * @param repoId The repo ID of the repository. Repo _name_ can be used instead of the ID, but usage of ID for permanent linking and API requests is preferred.@param refId An ID of a workspace, branch or commit.@param path A path to an item inside the repository.
    * @return 
    */
    THTTPResult<TVariant<TSharedPtr<FileEntry>, TSharedPtr<Error>>> SrcHandlersv2FilesGetFileEntry(
        FString repoId,
        FString refId,
        FString path,
        const FString& Token,
        const TMap<FString, FString>& Headers,
		int ConnectionTimeoutSeconds, int RequestTimeoutSeconds) const;

    typedef FApiResponseDelegate<TVariant<TSharedPtr<Src_handlersv2_commit_get_object_history_200_response>, TSharedPtr<Error>>> Fsrc_handlersv2_commit_getObjectHistoryDelegate;
    typedef FApiResponseDelegate<TVariant<TSharedPtr<HttpContent>, void*>> Fsrc_handlersv2_files_getBlobDelegate;
    typedef FApiResponseDelegate<TVariant<TSharedPtr<FileEntry>, TSharedPtr<Error>>> Fsrc_handlersv2_files_getFileEntryDelegate;

protected:
    TSharedPtr<DiversionHttp::FHttpRequestManager> ApiClient;
};

}
}

