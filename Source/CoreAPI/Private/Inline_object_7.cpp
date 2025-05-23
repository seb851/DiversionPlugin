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
#include "Inline_object_7.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void Inline_object_7::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	if (mCollaborators.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("collaborators")); WriteJsonValue(Writer, mCollaborators.GetValue());
	}
	if (mInvites.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("invites")); WriteJsonValue(Writer, mInvites.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool Inline_object_7::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("collaborators"), mCollaborators);
    ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("invites"), mInvites);


	return ParseSuccess;
}


}
}
}

