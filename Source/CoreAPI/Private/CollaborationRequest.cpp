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
#include "CollaborationRequest.h"

namespace Diversion {
namespace CoreAPI {
namespace Model {



void CollaborationRequest::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	Writer->WriteIdentifierPrefix(TEXT("mode")); WriteJsonValue(Writer, mMode);
	Writer->WriteObjectEnd();
}

bool CollaborationRequest::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{

	const TSharedPtr<FJsonObject>* InnerGeneratorOpenAPIObject;
	if (!JsonValue->TryGetObject(InnerGeneratorOpenAPIObject))
		return false;

	bool ParseSuccess = true;

    

	ParseSuccess &= TryGetJsonValue(*InnerGeneratorOpenAPIObject, TEXT("mode"), mMode);


	return ParseSuccess;
}


}
}
}

