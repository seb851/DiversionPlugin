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
#include "ModifyTag.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void ModifyTag::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	if (mName.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("name")); WriteJsonValue(Writer, mName.GetValue());
	}
	if (mDescription.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("description")); WriteJsonValue(Writer, mDescription.GetValue());
	}
	if (mCommit_id.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("commit_id")); WriteJsonValue(Writer, mCommit_id.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool ModifyTag::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("name"), mName);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("description"), mDescription);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("commit_id"), mCommit_id);


	return ParseSuccess;
}


}
}
}

