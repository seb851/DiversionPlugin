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



#include "JsonBody.h"
#include "Repo.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void Repo::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	Writer->WriteIdentifierPrefix(TEXT("repo_name")); WriteJsonValue(Writer, mRepo_name);
	if (mDescription.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("description")); WriteJsonValue(Writer, mDescription.GetValue());
	}
	Writer->WriteIdentifierPrefix(TEXT("repo_id")); WriteJsonValue(Writer, mRepo_id);
	if (mDefault_branch_id.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("default_branch_id")); WriteJsonValue(Writer, mDefault_branch_id.GetValue());
	}
	if (mDefault_branch_name.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("default_branch_name")); WriteJsonValue(Writer, mDefault_branch_name.GetValue());
	}
	Writer->WriteIdentifierPrefix(TEXT("size_bytes")); WriteJsonValue(Writer, mSize_bytes);
	Writer->WriteIdentifierPrefix(TEXT("owner_user_id")); WriteJsonValue(Writer, mOwner_user_id);
	Writer->WriteIdentifierPrefix(TEXT("created_timestamp")); WriteJsonValue(Writer, mCreated_timestamp);
	if (mSync_git_repo_url.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("sync_git_repo_url")); WriteJsonValue(Writer, mSync_git_repo_url.GetValue());
	}
	if (mSync_git_level.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("sync_git_level")); WriteJsonValue(Writer, mSync_git_level.GetValue());
	}
	if (mOrganization_id.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("organization_id")); WriteJsonValue(Writer, mOrganization_id.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool Repo::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("repo_name"), mRepo_name);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("description"), mDescription);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("repo_id"), mRepo_id);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("default_branch_id"), mDefault_branch_id);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("default_branch_name"), mDefault_branch_name);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("size_bytes"), mSize_bytes);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("owner_user_id"), mOwner_user_id);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("created_timestamp"), mCreated_timestamp);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("sync_git_repo_url"), mSync_git_repo_url);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("sync_git_level"), mSync_git_level);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("organization_id"), mOrganization_id);


	return ParseSuccess;
}


}
}
}

