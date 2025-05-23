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
#include "RefFileStatus.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void RefFileStatus::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	if (mWorkspace_id.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("workspace_id")); WriteJsonValue(Writer, mWorkspace_id.GetValue());
	}
	Writer->WriteIdentifierPrefix(TEXT("commit_id")); WriteJsonValue(Writer, mCommit_id);
	if (mBranch_id.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("branch_id")); WriteJsonValue(Writer, mBranch_id.GetValue());
	}
	if (mBranch_name.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("branch_name")); WriteJsonValue(Writer, mBranch_name.GetValue());
	}
	Writer->WriteIdentifierPrefix(TEXT("status")); WriteJsonValue(Writer, mStatus);
	Writer->WriteIdentifierPrefix(TEXT("author")); WriteJsonValue(Writer, mAuthor);
	if (mMtime.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("mtime")); WriteJsonValue(Writer, mMtime.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool RefFileStatus::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("workspace_id"), mWorkspace_id);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("commit_id"), mCommit_id);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("branch_id"), mBranch_id);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("branch_name"), mBranch_name);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("status"), mStatus);
	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("author"), mAuthor);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("mtime"), mMtime);


	return ParseSuccess;
}


}
}
}

